#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QVector>
#include <QPointF>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

/* 轨迹画布：把里程计给出的世界坐标(米)画成小车行驶轨迹。 */
class TrajectoryView : public QWidget
{
    Q_OBJECT
public:
    explicit TrajectoryView(QWidget *parent = nullptr);
    void addPose(double x, double y, double th);
    void clearPath();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPointF toPx(const QPointF &world) const;

    QVector<QPointF> m_pts;    /* 世界坐标轨迹点 (m) */
    double m_th;               /* 最近姿态角 (rad) */
    double m_scale;            /* 像素/米，随轨迹自动缩放 */
};

class CommWorker;

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void requestConnect(const QString &host, quint16 port);
    void requestDisconnect();
    void requestNode(int node);
    void requestPreOp();
    void requestStart();

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onPose(double x, double y, double th);
    void onConnState(bool up);

private:
    void buildUi();

    CommWorker     *m_worker;
    class QThread  *m_thread;

    TrajectoryView *m_view;
    QLabel         *m_labX;
    QLabel         *m_labY;
    QLabel         *m_labTh;
    QLineEdit      *m_edHost;
    QLineEdit      *m_edPort;
    QLineEdit      *m_edNode;
    QPlainTextEdit *m_log;
    QPushButton    *m_btnConnect;
    QPushButton    *m_btnDisconnect;
};

#endif // MAINWINDOW_H
