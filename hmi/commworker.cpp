#include "commworker.h"

#include <QRegularExpression>

CommWorker::CommWorker(QObject *parent)
    : QObject(parent)
    , m_sock(new QTcpSocket(this))
    , m_poll(new QTimer(this))
{
    m_poll->setInterval(100);   /* 100ms 轮询一个子索引，300ms 刷新一次完整位姿 */

    connect(m_sock, &QTcpSocket::connected,    this, &CommWorker::onConnected);
    connect(m_sock, &QTcpSocket::disconnected, this, &CommWorker::onDisconnected);
    connect(m_sock, &QTcpSocket::readyRead,    this, &CommWorker::onReadyRead);
    connect(m_sock, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &CommWorker::onSocketError);
    connect(m_poll, &QTimer::timeout, this, &CommWorker::pollTimeout);
}

CommWorker::~CommWorker() = default;

void CommWorker::connectBoard(const QString &host, quint16 port)
{
    if (m_sock->state() != QAbstractSocket::UnconnectedState)
        m_sock->abort();
    m_rx.clear();
    m_seq2sub.clear();
    m_val.clear();
    emit logLine(QString("connecting %1:%2 ...").arg(host).arg(port));
    m_sock->connectToHost(host, port);
}

void CommWorker::disconnectBoard()
{
    m_poll->stop();
    if (m_sock->state() != QAbstractSocket::UnconnectedState)
        m_sock->disconnectFromHost();
}

void CommWorker::setNode(int node)
{
    if (node >= 1 && node <= 127)
        m_node = node;
}

void CommWorker::onConnected()
{
    emit logLine("connected");
    m_rdSub = 1;
    m_poll->start();
    emit connected();
}

void CommWorker::onDisconnected()
{
    m_poll->stop();
    emit logLine("disconnected");
    emit disconnected();
}

void CommWorker::onSocketError(QAbstractSocket::SocketError)
{
    emit logLine("socket error: " + m_sock->errorString());
}

/* 网关要求每个命令以 "[<seq>]" 开头，"<node>" 必须显式给出(默认 node 为 -1 会报错)。
 * 读命令语法: [<seq>] <node> r <index> <subindex> <datatype> */
void CommWorker::sendRead(int subIndex)
{
    int seq = ++m_seq;
    m_seq2sub.insert(seq, subIndex);
    sendLine(QString("[%1] %2 r 0x6FFF %3 r32").arg(seq).arg(m_node).arg(subIndex));
}

void CommWorker::nmtPreOp()
{
    sendLine(QString("[%1] %2 preop").arg(++m_seq).arg(m_node));
}

void CommWorker::nmtStart()
{
    sendLine(QString("[%1] %2 start").arg(++m_seq).arg(m_node));
}

void CommWorker::sendLine(const QString &s)
{
    m_sock->write(s.toLatin1() + "\n");
}

void CommWorker::pollTimeout()
{
    sendRead(m_rdSub);
    m_rdSub = (m_rdSub >= 3) ? 1 : (m_rdSub + 1);
}

/* 网关响应逐字格式: 读 => "[<seq>] <value>\r\n" ; 写/NMT => "[<seq>] OK\r\n" ;
 * 错误 => "[<seq>] ERROR:<code>\r\n"。按 \r\n 切行后解析。 */
void CommWorker::onReadyRead()
{
    static const QRegularExpression re(QStringLiteral("^\\[(\\d+)\\]\\s*(.*)$"));

    m_rx += m_sock->readAll();
    int idx;
    while ((idx = m_rx.indexOf("\r\n")) >= 0) {
        const QByteArray raw = m_rx.left(idx);
        m_rx.remove(0, idx + 2);

        const QString line = QString::fromLatin1(raw).trimmed();
        if (line.isEmpty())
            continue;

        const QRegularExpressionMatch m = re.match(line);
        if (!m.hasMatch()) {
            emit logLine("<< " + line);   /* 非预期行(如交互提示)，只记录 */
            continue;
        }

        const int     seq  = m.captured(1).toInt();
        const QString rest = m.captured(2).trimmed();

        if (rest.startsWith("ERROR")) {
            emit logLine(QString("seq %1: %2").arg(seq).arg(rest));
            m_seq2sub.remove(seq);
        } else if (rest == "OK") {
            emit logLine(QString("seq %1: OK").arg(seq));
        } else {
            bool ok = false;
            const double v = rest.toDouble(&ok);
            if (ok && m_seq2sub.contains(seq)) {
                m_val[m_seq2sub.take(seq)] = v;
                emit poseUpdated(m_val.value(1, 0.0),
                                 m_val.value(2, 0.0),
                                 m_val.value(3, 0.0));
            } else {
                emit logLine("<< " + line);
            }
        }
    }
}
