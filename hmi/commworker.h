#ifndef COMMWORKER_H
#define COMMWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QHash>

/* 通信线程对象：与板端 canopend 的 CiA-309 ASCII 网关 (tcp-60000) 通信。
 * 上行：轮询读取自定义 OD 数组 0x6FFF 的三个 REAL32 子索引 (1=x, 2=y, 3=th)。
 * 下行：把上位机期望速度 cmd_vel 经 SDO 写进主控 OD 数组 0x7000
 *       (子 1=v m/s, 子 2=ω rad/s)。主控有"失联停车"deadman(500ms 无新写入即
 *       清零)，故这里不能只在滑条变动时写一次，必须由 m_cmd 定时器周期重发。
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
    void setCmdVel(double v, double w);   /* 存下期望速度，交由 m_cmd 周期重发 */

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
    void cmdTimeout();                    /* cmd_vel 周期重发(deadman 保活) */

private:
    void sendLine(const QString &s);
    void sendRead(int subIndex);
    void sendWrite(int subIndex, double value);

    QTcpSocket  *m_sock;
    QTimer      *m_poll;
    QTimer      *m_cmd;
    QByteArray   m_rx;
    QHash<int, int>    m_seq2sub;   /* 请求序列号 -> 子索引 1..3 */
    QHash<int, double> m_val;       /* 子索引 -> 最近一次读到的值 */
    int m_seq    = 0;               /* 网关要求命令以 [n] 开头，自增 */
    int m_node   = 1;               /* 目标节点号(1..127)，必须显式给出 */
    int m_rdSub  = 1;               /* 轮询游标: 1->2->3->1 */
    double m_v   = 0.0;             /* 期望线速度 v (m/s) */
    double m_w   = 0.0;             /* 期望角速度 ω (rad/s) */
};

#endif // COMMWORKER_H
