#ifndef COMMWORKER_H
#define COMMWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QHash>

/* 通信线程对象：与板端 canopend 的 CiA-309 ASCII 网关 (tcp-60000) 通信。
 * 轮询读取自定义 OD 数组 0x6FFF 的三个 REAL32 子索引 (1=x, 2=y, 3=th)。
 * 本对象 moveToThread(QThread) 后，全部槽函数都在通信线程内执行，
 * 与界面线程通过信号槽(队列连接)交互。 */
class CommWorker : public QObject
{
    Q_OBJECT
public:
    explicit CommWorker(QObject *parent = nullptr);
    ~CommWorker() override;

public slots:
    void connectBoard(const QString &host, quint16 port);
    void disconnectBoard();
    void setNode(int node);
    void nmtPreOp();
    void nmtStart();

signals:
    void connected();
    void disconnected();
    void poseUpdated(double x, double y, double th);
    void logLine(const QString &msg);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError err);
    void pollTimeout();

private:
    void sendLine(const QString &s);
    void sendRead(int subIndex);

    QTcpSocket  *m_sock;
    QTimer      *m_poll;
    QByteArray   m_rx;
    QHash<int, int>    m_seq2sub;   /* 请求序列号 -> 子索引 1..3 */
    QHash<int, double> m_val;       /* 子索引 -> 最近一次读到的值 */
    int m_seq    = 0;               /* 网关要求命令以 [n] 开头，自增 */
    int m_node   = 1;               /* 目标节点号(1..127)，必须显式给出 */
    int m_rdSub  = 1;               /* 轮询游标: 1->2->3->1 */
};

#endif // COMMWORKER_H
