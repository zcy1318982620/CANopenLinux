#include "mainwindow.h"
#include "commworker.h"

#include <QThread>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QSlider>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

/* cmd_vel 滑条标度：滑条是整数，除以 1000 得到物理量。
 * v ∈ [-0.5, 0.5] m/s，ω ∈ [-1.0, 1.0] rad/s（与 S11 分册的验证量级一致）。 */
static const int CMD_SCALE = 1000;

/* ============================ TrajectoryView ============================ */

TrajectoryView::TrajectoryView(QWidget *parent)
    : QWidget(parent), m_th(0.0), m_scale(100.0)
{
    setMinimumSize(400, 400);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);
}

void TrajectoryView::addPose(double x, double y, double th)
{
    m_pts.append(QPointF(x, y));
    if (m_pts.size() > 20000)
        m_pts.removeFirst();
    m_th = th;
    update();
}

void TrajectoryView::clearPath()
{
    m_pts.clear();
    m_th = 0.0;
    update();
}

QPointF TrajectoryView::toPx(const QPointF &world) const
{
    return QPointF(width()  / 2.0 + world.x() * m_scale,
                   height() / 2.0 - world.y() * m_scale);
}

void TrajectoryView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    /* 自动缩放：让最远的轨迹点落在画布 40% 半径内 */
    double maxAbs = 1.0;
    for (const QPointF &pt : m_pts) {
        maxAbs = qMax(maxAbs, qMax(qAbs(pt.x()), qAbs(pt.y())));
    }
    m_scale = qMin(width(), height()) * 0.4 / maxAbs;

    /* 网格 + 原点 */
    p.setPen(QPen(QColor(230, 230, 230)));
    const QPointF o = toPx(QPointF(0, 0));
    p.drawLine(0, int(o.y()), width(), int(o.y()));
    p.drawLine(int(o.x()), 0, int(o.x()), height());

    /* 轨迹 */
    if (m_pts.size() > 1) {
        QPainterPath path;
        path.moveTo(toPx(m_pts.first()));
        for (int i = 1; i < m_pts.size(); ++i)
            path.lineTo(toPx(m_pts.at(i)));
        p.setPen(QPen(QColor(0, 120, 215), 2));
        p.drawPath(path);
    }

    /* 当前位姿：小车 + 朝向箭头 */
    if (!m_pts.isEmpty()) {
        const QPointF c = toPx(m_pts.last());
        p.setBrush(QColor(215, 60, 40));
        p.setPen(Qt::NoPen);
        p.drawEllipse(c, 5, 5);

        const double L = 22.0;                       /* 箭头像素长 */
        QPointF head(c.x() + L * qCos(m_th), c.y() - L * qSin(m_th));
        p.setPen(QPen(QColor(215, 60, 40), 2));
        p.drawLine(c, head);
    }
}

/* ============================== MainWindow ============================== */

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , m_worker(new CommWorker)
    , m_thread(new QThread(this))
{
    m_worker->moveToThread(m_thread);
    m_thread->start();

    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    /* 界面 -> 通信线程(队列连接) */
    connect(this, &MainWindow::requestConnect,    m_worker, &CommWorker::connectBoard);
    connect(this, &MainWindow::requestDisconnect, m_worker, &CommWorker::disconnectBoard);
    connect(this, &MainWindow::requestNode,       m_worker, &CommWorker::setNode);
    connect(this, &MainWindow::requestPreOp,      m_worker, &CommWorker::nmtPreOp);
    connect(this, &MainWindow::requestStart,      m_worker, &CommWorker::nmtStart);
    connect(this, &MainWindow::requestCmdVel,     m_worker, &CommWorker::setCmdVel);

    /* 通信线程 -> 界面 */
    connect(m_worker, &CommWorker::poseUpdated, this, &MainWindow::onPose);
    connect(m_worker, &CommWorker::connected,    this, [this]{ onConnState(true);  });
    connect(m_worker, &CommWorker::disconnected, this, [this]{ onConnState(false); });
    connect(m_worker, &CommWorker::logLine,      this, [this](const QString &s){
        m_log->appendPlainText(s);
    });

    buildUi();
    setWindowTitle(QStringLiteral("AGV 上位机 (CiA-309 / 读 0x6FFF · 写 0x7000)"));
    resize(900, 520);
}

MainWindow::~MainWindow()
{
    if (m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait(1000);
    }
}

void MainWindow::buildUi()
{
    m_view = new TrajectoryView(this);

    m_labX  = new QLabel("x = 0.000 m", this);
    m_labY  = new QLabel("y = 0.000 m", this);
    m_labTh = new QLabel("th = 0.0 deg", this);
    QFont f = m_labX->font();
    f.setPointSize(12);
    m_labX->setFont(f); m_labY->setFont(f); m_labTh->setFont(f);

    m_edHost = new QLineEdit("169.254.86.72", this);
    m_edPort = new QLineEdit("60000", this);
    m_edNode = new QLineEdit("1", this);
    m_edPort->setMaximumWidth(80);
    m_edNode->setMaximumWidth(60);

    m_btnConnect    = new QPushButton(QStringLiteral("连接"), this);
    m_btnDisconnect = new QPushButton(QStringLiteral("断开"), this);
    m_btnDisconnect->setEnabled(false);
    QPushButton *btnPreOp = new QPushButton(QStringLiteral("NMT Pre-Op"), this);
    QPushButton *btnStart = new QPushButton(QStringLiteral("NMT Start"), this);
    QPushButton *btnClear = new QPushButton(QStringLiteral("清空轨迹"), this);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(500);
    m_log->setFixedHeight(90);

    QGroupBox *boxCtl = new QGroupBox(QStringLiteral("连接"), this);
    QGridLayout *gc = new QGridLayout(boxCtl);
    gc->addWidget(new QLabel("板卡 IP"), 0, 0); gc->addWidget(m_edHost, 0, 1, 1, 2);
    gc->addWidget(new QLabel("端口"),    1, 0); gc->addWidget(m_edPort, 1, 1, 1, 2);
    gc->addWidget(new QLabel("节点号"),  2, 0); gc->addWidget(m_edNode, 2, 1, 1, 2);
    gc->addWidget(m_btnConnect,    3, 0, 1, 2);
    gc->addWidget(m_btnDisconnect, 4, 0, 1, 2);

    QGroupBox *boxPose = new QGroupBox(QStringLiteral("位姿 (OD 0x6FFF)"), this);
    QVBoxLayout *vp = new QVBoxLayout(boxPose);
    vp->addWidget(m_labX); vp->addWidget(m_labY); vp->addWidget(m_labTh);

    /* --- 运动控制：把 cmd_vel 经网关 SDO 写进主控 OD 0x7000(下行打通) --- */
    m_boxCmd = new QGroupBox(QStringLiteral("运动控制 (cmd_vel → 0x7000)"), this);
    QVBoxLayout *vc = new QVBoxLayout(m_boxCmd);

    m_sldV = new QSlider(Qt::Horizontal, this);
    m_sldV->setRange(-500, 500);            /* ±0.5 m/s */
    m_sldV->setValue(0);
    m_sldW = new QSlider(Qt::Horizontal, this);
    m_sldW->setRange(-1000, 1000);          /* ±1.0 rad/s */
    m_sldW->setValue(0);

    m_labCmd = new QLabel(QStringLiteral("v = 0.000 m/s   ω = 0.000 rad/s"), this);

    QPushButton *btnStop = new QPushButton(QStringLiteral("停车 (发零)"), this);

    vc->addWidget(new QLabel(QStringLiteral("线速度 v (±0.5 m/s)"), this));
    vc->addWidget(m_sldV);
    vc->addWidget(new QLabel(QStringLiteral("角速度 ω (±1.0 rad/s)"), this));
    vc->addWidget(m_sldW);
    vc->addWidget(m_labCmd);
    vc->addWidget(btnStop);
    vc->addWidget(new QLabel(QStringLiteral("提示：需从站已使能；松手不停，\n请按「停车」或断开连接。"), this));

    QGroupBox *boxNmt = new QGroupBox(QStringLiteral("控制"), this);
    QVBoxLayout *vn = new QVBoxLayout(boxNmt);
    vn->addWidget(btnPreOp); vn->addWidget(btnStart); vn->addWidget(btnClear);
    vn->addStretch();

    QVBoxLayout *right = new QVBoxLayout;
    right->addWidget(boxCtl);
    right->addWidget(boxPose);
    right->addWidget(m_boxCmd);
    right->addWidget(boxNmt);
    right->addWidget(new QLabel(QStringLiteral("日志"), this));
    right->addWidget(m_log);

    QHBoxLayout *root = new QHBoxLayout(this);
    root->addWidget(m_view, 3);
    root->addLayout(right, 1);

    connect(m_btnConnect,    &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_btnDisconnect, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(btnPreOp, &QPushButton::clicked, this, [this]{ emit requestPreOp(); });
    connect(btnStart, &QPushButton::clicked, this, [this]{ emit requestStart(); });
    connect(btnClear, &QPushButton::clicked, this, [this]{ m_view->clearPath(); });

    /* 滑条任意一路变动 → 立即把 (v,ω) 交给通信线程存值；周期重发由 worker 定时器负责 */
    connect(m_sldV, &QSlider::valueChanged, this, [this](int){ onCmdChanged(); });
    connect(m_sldW, &QSlider::valueChanged, this, [this](int){ onCmdChanged(); });
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);

    m_boxCmd->setEnabled(false);    /* 未连接前禁止发指令 */
}

void MainWindow::onConnectClicked()
{
    const quint16 port = quint16(m_edPort->text().toUShort());
    emit requestNode(m_edNode->text().toInt());
    emit requestConnect(m_edHost->text().trimmed(), port);
}

void MainWindow::onDisconnectClicked()
{
    emit requestDisconnect();
}

void MainWindow::onConnState(bool up)
{
    m_btnConnect->setEnabled(!up);
    m_btnDisconnect->setEnabled(up);
    m_edHost->setEnabled(!up);
    m_edPort->setEnabled(!up);
    m_edNode->setEnabled(!up);
    m_boxCmd->setEnabled(up);       /* 连上才允许发运动指令 */

    if (!up) {
        /* 断开即回零：滑条归零会经 valueChanged → onCmdChanged 发一次零速 */
        m_sldV->setValue(0);
        m_sldW->setValue(0);
    }
}

/* 滑条变动：更新数值显示，并把 (v,ω) 交给通信线程。真正的周期重发由
 * CommWorker::cmdTimeout 负责，这里只在变动瞬间推一次，让车及时响应。 */
void MainWindow::onCmdChanged()
{
    const double v = m_sldV->value() / double(CMD_SCALE);
    const double w = m_sldW->value() / double(CMD_SCALE);
    m_labCmd->setText(QString("v = %1 m/s   ω = %2 rad/s")
                      .arg(v, 0, 'f', 3).arg(w, 0, 'f', 3));
    emit requestCmdVel(v, w);
}

void MainWindow::onStopClicked()
{
    m_sldV->setValue(0);            /* 触发 onCmdChanged → 下发 (0,0) */
    m_sldW->setValue(0);
    emit requestCmdVel(0.0, 0.0);   /* 兜底：滑条本就在 0 时也显式发一次零 */
}

void MainWindow::onPose(double x, double y, double th)
{
    m_labX->setText(QString("x = %1 m").arg(x, 0, 'f', 3));
    m_labY->setText(QString("y = %1 m").arg(y, 0, 'f', 3));
    m_labTh->setText(QString("th = %1 deg").arg(th * 180.0 / M_PI, 0, 'f', 1));
    m_view->addPose(x, y, th);
}
