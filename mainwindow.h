#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QSignalMapper>
#include <QTimer>

#include <QDebug>
#include "visa.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_CONNECTLAN_clicked();

    void handleOUTP(int id);
    void handleSetParam(int id);
    void handleGetParam(int id);
    void handleVoltprotLabel(int id);
    void handleCurrprotLabel(int id);
    void handleMeas(int id);

private:
    Ui::MainWindow *ui;


    char instrDescriptor[VI_FIND_BUFLEN];
    ViUInt32 numInstrs;
    ViFindList findList;

    ViSession defaultRM, instr;
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


};
#endif // MAINWINDOW_H
