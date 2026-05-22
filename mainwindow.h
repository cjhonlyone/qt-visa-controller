#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QTimer>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QTcpSocket>
#include <QVector>
#include <QQueue>
#include <QSet>
#include <QDebug>
#include <stdint.h>

#include "scpiclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 一个稳压源通道在 UI 上的全部部件 + 周期测量定时器。
// 全部使用 parent = MainWindow 的裸指针，PowerChannel 本身可安全拷贝/搬移，
// 配合 channels_.reserve() 防止隐式重分配丢失槽连接。
struct PowerChannel
{
    int idx = 0;
    QPushButton *outp = nullptr;
    QPushButton *setParam = nullptr;
    QPushButton *getParam = nullptr;
    QPushButton *voltLabel = nullptr;
    QPushButton *currLabel = nullptr;
    QPushButton *voltProtLabel = nullptr;
    QPushButton *currProtLabel = nullptr;
    QLineEdit *volt = nullptr;
    QLineEdit *curr = nullptr;
    QLineEdit *voltProt = nullptr;
    QLineEdit *currProt = nullptr;
    QPushButton *measVoltLabel = nullptr;
    QPushButton *measCurrLabel = nullptr;
    QPushButton *measPwrLabel = nullptr;
    QLineEdit *measVolt = nullptr;
    QLineEdit *measCurr = nullptr;
    QLineEdit *measPwr = nullptr;
    QTimer *timer = nullptr;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleOUTP(int id);
    void handleSetParam(int id);
    void handleGetParam(int id);
    void handleVoltprotLabel(int id);
    void handleCurrprotLabel(int id);
    void handleMeas(int id);

    void on_SCANLAN_clicked();
    void on_CONNECTLAN_DP_clicked();
    void processData();
    void probeNext();
    void onScpiDisconnected();

private:
    void setupChannels();
    void writeChannelCommand(int channelIdx, const char *fmt, double value);
    void writeChannelBool(int channelIdx, const char *fmt, bool on);
    QByteArray queryChannel(int channelIdx, const char *fmt);
    bool parseBool(const QByteArray &raw) const;

    Ui::MainWindow *ui;

    QPalette p_ON;
    QPalette p_OFF;

    QVector<PowerChannel> channels_;

    QUdpSocket *udpSend_ = nullptr;
    QUdpSocket *udpRecv_ = nullptr;

    ScpiClient *scpi_ = nullptr;

    // 异步发现：portmap 广播收到的 IP 入队，由 probeNext 状态机逐个用 *IDN? 探测。
    QQueue<QString> probeQueue_;
    QSet<QString>   probedIps_;
    bool probing_ = false;
};

#endif // MAINWINDOW_H
