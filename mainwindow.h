#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QSignalMapper>
#include <QTimer>
#include <QtNetwork/QUdpSocket>
#include <QFileDialog>

#include <QDebug>
#include "visa.h"
#include "stdint.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QUdpSocket *UDP_send;
    QUdpSocket *UDP_recv;

private slots:
    void handleOUTP(int id);
    void handleSetParam(int id);
    void handleGetParam(int id);
    void handleVoltprotLabel(int id);
    void handleCurrprotLabel(int id);
    void handleMeas(int id);

    void handleSDGOUTP(int id);
    void handleSDGSendValue(int id);
    void handleSDGChooseFile(int id);

    void on_SCANLAN_clicked();
    void processData();
    void on_CONNECTLAN_DP_clicked();
    void on_CONNECTLAN_SDG_clicked();

private:
    Ui::MainWindow *ui;

    char instrDescriptor[VI_FIND_BUFLEN];
    ViUInt32 numInstrs;
    ViFindList findList;

    ViSession defaultRM, instr, defaultRM_SDG, instr_SDG;
    ViStatus status;

    QPalette p_ON;
    QPalette p_OFF;

    QList<QPushButton *> OUTP_list;
    QList<QPushButton *> SetParam_list;
    QList<QPushButton *> GetParam_list;
    QList<QPushButton *> VoltLabel_list;
    QList<QPushButton *> CurrLabel_list;
    QList<QPushButton *> VoltprotLabel_list;
    QList<QPushButton *> CurrprotLabel_list;
    QList<QLineEdit *> Volt_list;
    QList<QLineEdit *> Curr_list;
    QList<QLineEdit *> Voltprot_list;
    QList<QLineEdit *> Currprot_list;

    QList<QPushButton *> MEASVoltLabel_list;
    QList<QPushButton *> MEASCurrLabel_list;
    QList<QPushButton *> MEASPwrrLabel_list;
    QList<QLineEdit *> MEASVolt_list;
    QList<QLineEdit *> MEASCurr_list;
    QList<QLineEdit *> MEASPwrr_list;

    QList<QPushButton *> AllQPushButton_list;
    QList<QLineEdit *> AllQLineEdit_list;

    QList<QTimer *> AllQTimer_list;

    QSignalMapper * OUTP_Mapper;
    QSignalMapper * SetParam_Mapper;
    QSignalMapper * GetParam_Mapper;
    QSignalMapper * VoltprotLabel_Mapper;
    QSignalMapper * CurrprotLabel_Mapper;

    QSignalMapper * QTimer_Mapper;

/*********************************************************************/
    QList<QPushButton *> SDG_OUTP_list;
    QList<QPushButton *> SDG_SendValue_list;
    QList<QPushButton *> SDG_ChooseFile_list;
    QList<QLineEdit *> SDG_FilePath_list;
    QList<QLineEdit *> SDG_SampleRate_list;
    QList<QLineEdit *> SDG_Amplitude_list;
    QList<QLineEdit *> SDG_Offset_list;
    QList<QLineEdit *> SDG_Phase_list;

    QSignalMapper * SDG_OUTP_Mapper;
    QSignalMapper * SDG_SendValue_Mapper;
    QSignalMapper * SDG_ChooseFile_Mapper;
/*********************************************************************/

    QUdpSocket *Udp_Send;
    QUdpSocket *Udp_Recv;
};
#endif // MAINWINDOW_H
