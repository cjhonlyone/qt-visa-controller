#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "qmessagebox.h"

#define MAX_CNT 256

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    p_ON = QPalette();
    p_OFF = QPalette();
    p_ON.setColor(QPalette::Button,QColor("green"));
    p_OFF.setColor(QPalette::Button,QColor("lightgray"));

    AllQPushButton_list = ui->tab->findChildren<QPushButton *>();
    AllQLineEdit_list = ui->tab->findChildren<QLineEdit *>();

    OUTP_list = ui->tab->findChildren<QPushButton *>(QRegularExpression("^OUTP_+"));
    SetParam_list = ui->tab->findChildren<QPushButton *>(QRegularExpression("^SetParam_+"));
    GetParam_list = ui->tab->findChildren<QPushButton *>(QRegularExpression("^GetParam_+"));
    VoltLabel_list = ui->tab->findChildren<QPushButton *>(QRegularExpression("^VoltLabel_+"));
    CurrLabel_list = ui->tab->findChildren<QPushButton *>(QRegularExpression("^CurrLabel_+"));
    VoltprotLabel_list = ui->tab->findChildren<QPushButton *>(QRegularExpression("^VoltprotLabel_+"));
    CurrprotLabel_list = ui->tab->findChildren<QPushButton *>(QRegularExpression("^CurrprotLabel_+"));

    MEASVoltLabel_list = ui->tab->findChildren<QPushButton *>(QRegularExpression("^MEASVoltLabel_+"));
    MEASCurrLabel_list = ui->tab->findChildren<QPushButton *>(QRegularExpression("^MEASCurrLabel_+"));
    MEASPwrrLabel_list = ui->tab->findChildren<QPushButton *>(QRegularExpression("^MEASPwrrLabel_+"));

    Volt_list = ui->tab->findChildren<QLineEdit *>(QRegularExpression("^Volt_+"));
    Curr_list = ui->tab->findChildren<QLineEdit *>(QRegularExpression("^Curr_+"));
    Voltprot_list = ui->tab->findChildren<QLineEdit *>(QRegularExpression("^Voltprot_+"));
    Currprot_list = ui->tab->findChildren<QLineEdit *>(QRegularExpression("^Currprot_+"));

    MEASVolt_list = ui->tab->findChildren<QLineEdit *>(QRegularExpression("^MEASVolt_+"));
    MEASCurr_list = ui->tab->findChildren<QLineEdit *>(QRegularExpression("^MEASCurr_+"));
    MEASPwrr_list = ui->tab->findChildren<QLineEdit *>(QRegularExpression("^MEASPwrr_+"));

    for (int i = 0; i < OUTP_list.size(); i++){
        OUTP_list[i]->setAutoFillBackground(true);
        OUTP_list[i]->setFlat(true);
        OUTP_list[i]->setPalette(p_OFF);

        VoltprotLabel_list[i]->setAutoFillBackground(true);
        VoltprotLabel_list[i]->setFlat(true);
        VoltprotLabel_list[i]->setPalette(p_OFF);

        CurrprotLabel_list[i]->setAutoFillBackground(true);
        CurrprotLabel_list[i]->setFlat(true);
        CurrprotLabel_list[i]->setPalette(p_OFF);

        VoltLabel_list[i]->setDisabled(true);
        CurrLabel_list[i]->setDisabled(true);

        OUTP_list[i]->setText(QStringLiteral("输出"));
        SetParam_list[i]->setText(QStringLiteral("设置参数"));
        GetParam_list[i]->setText(QStringLiteral("读取参数"));

        VoltLabel_list[i]->setText(QStringLiteral("电压(V)"));
        CurrLabel_list[i]->setText(QStringLiteral("电流(A)"));
        VoltprotLabel_list[i]->setText(QStringLiteral("限压(V)"));
        CurrprotLabel_list[i]->setText(QStringLiteral("限流(A)"));

        OUTP_list[i]->setMinimumWidth(80);
        SetParam_list[i]->setMinimumWidth(80);
        GetParam_list[i]->setMinimumWidth(80);

        VoltLabel_list[i]->setMinimumWidth(80);
        CurrLabel_list[i]->setMinimumWidth(80);
        VoltprotLabel_list[i]->setMinimumWidth(80);
        CurrprotLabel_list[i]->setMinimumWidth(80);

        Volt_list[i]->setMinimumWidth(80);
        Curr_list[i]->setMinimumWidth(80);
        Voltprot_list[i]->setMinimumWidth(80);
        Currprot_list[i]->setMinimumWidth(80);

        Volt_list[i]->setText("5");
        Curr_list[i]->setText("1.5");

        Voltprot_list[i]->setText("6");
        Currprot_list[i]->setText("2");


        MEASVoltLabel_list[i]->setDisabled(true);
        MEASCurrLabel_list[i]->setDisabled(true);
        MEASPwrrLabel_list[i]->setDisabled(true);

        MEASVolt_list[i]->setDisabled(true);
        MEASCurr_list[i]->setDisabled(true);
        MEASPwrr_list[i]->setDisabled(true);

        MEASVoltLabel_list[i]->setText(QStringLiteral("电压(V)"));
        MEASCurrLabel_list[i]->setText(QStringLiteral("电流(A)"));
        MEASPwrrLabel_list[i]->setText(QStringLiteral("功率(W)"));

        MEASPwrr_list[i]->setMinimumWidth(80);
        MEASCurr_list[i]->setMinimumWidth(80);
        MEASVolt_list[i]->setMinimumWidth(80);

        AllQTimer_list.append(new QTimer(this));
    }


    OUTP_Mapper= new QSignalMapper(this);
    SetParam_Mapper= new QSignalMapper(this);
    GetParam_Mapper= new QSignalMapper(this);
    VoltprotLabel_Mapper= new QSignalMapper(this);
    CurrprotLabel_Mapper= new QSignalMapper(this);
    QTimer_Mapper= new QSignalMapper(this);

    for (int i = 0; i < OUTP_list.size(); ++i) {
        connect(OUTP_list[i], SIGNAL(clicked()), OUTP_Mapper, SLOT(map()));
        connect(SetParam_list[i], SIGNAL(clicked()), SetParam_Mapper, SLOT(map()));
        connect(GetParam_list[i], SIGNAL(clicked()), GetParam_Mapper, SLOT(map()));
        connect(VoltprotLabel_list[i], SIGNAL(clicked()), VoltprotLabel_Mapper, SLOT(map()));
        connect(CurrprotLabel_list[i], SIGNAL(clicked()), CurrprotLabel_Mapper, SLOT(map()));

        OUTP_Mapper->setMapping(OUTP_list[i], i);
        SetParam_Mapper->setMapping(SetParam_list[i], i);
        GetParam_Mapper->setMapping(GetParam_list[i], i);
        VoltprotLabel_Mapper->setMapping(VoltprotLabel_list[i], i);
        CurrprotLabel_Mapper->setMapping(CurrprotLabel_list[i], i);

        connect(AllQTimer_list[i], SIGNAL(timeout()), QTimer_Mapper, SLOT(map()));
        QTimer_Mapper->setMapping(AllQTimer_list[i], i);
    }

    connect(OUTP_Mapper, SIGNAL(mapped(int)), this, SLOT(handleOUTP(int)));
    connect(SetParam_Mapper, SIGNAL(mapped(int)), this, SLOT(handleSetParam(int)));
    connect(GetParam_Mapper, SIGNAL(mapped(int)), this, SLOT(handleGetParam(int)));
    connect(VoltprotLabel_Mapper, SIGNAL(mapped(int)), this, SLOT(handleVoltprotLabel(int)));
    connect(CurrprotLabel_Mapper, SIGNAL(mapped(int)), this, SLOT(handleCurrprotLabel(int)));

    connect(QTimer_Mapper, SIGNAL(mapped(int)), this, SLOT(handleMeas(int)));

    defaultRM = NULL;
    instr = NULL;

//    qDebug() << this->height() << this->width();
}


MainWindow::~MainWindow()
{
    delete ui;
}

#define INST_s ":INST CH%d\n"

#define OUTP_s ":OUTP CH%d,%s\n"
#define VOLT_s ":VOLT %.2f\n"
#define CURR_s ":CURR %.2f\n"
#define VOLTPROT_s ":VOLT:PROT %.2f\n"
#define CURRPROT_s ":CURR:PROT %.2f\n"
#define VOLTPROTSTAT_s ":VOLT:PROT:STAT %s\n"
#define CURRPROTSTAT_s ":CURR:PROT:STAT %s\n"

#define INST_g ":INST?\n"
#define OUTP_g ":OUTP? CH%d\n"
#define VOLT_g ":VOLT?\n"
#define CURR_g ":CURR?\n"
#define VOLTPROT_g ":VOLT:PROT?\n"
#define CURRPROT_g ":CURR:PROT?\n"
#define VOLTPROTSTAT_g ":VOLT:PROT:STAT?\n"
#define CURRPROTSTAT_g ":CURR:PROT:STAT?\n"

#define MEASALL_g ":MEAS:ALL? CH%d\n"

ViStatus VISA_POWER_SETValueBool(ViSession instr, uint32_t ch, const char* Value)
{
    ViStatus status;
    ViUInt32 retCount;
    ViChar	buffer[MAX_CNT];	/* For checking errors */

    ViUInt32 bufferCount;

    bufferCount = sprintf_s(buffer, sizeof(buffer), INST_s, ch+1);
    status = viWrite(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);
    if (status < VI_SUCCESS) {
        return status;
    }

    bufferCount = sprintf_s(buffer, sizeof(buffer), OUTP_s, ch+1, Value);
    status = viWrite(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);

    return status;
}
ViStatus VISA_POWER_SETValue(ViSession instr, uint32_t ch, const char * format, float Value)
{
    ViStatus status;
    ViUInt32 retCount;
    ViChar	buffer[MAX_CNT];	/* For checking errors */

    ViUInt32 bufferCount;

    bufferCount = sprintf_s(buffer, sizeof(buffer), INST_s, ch+1);
    status = viWrite(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);
    if (status < VI_SUCCESS) {
        return status;
    }
    bufferCount = sprintf_s(buffer, sizeof(buffer), format, Value);
    status = viWrite(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);
    return status;
}
ViStatus VISA_POWER_SETBool(ViSession instr, uint32_t ch, const char * format, const char* Value)
{
    ViStatus status;
    ViUInt32 retCount;
    ViChar	buffer[MAX_CNT];	/* For checking errors */

    ViUInt32 bufferCount;

    bufferCount = sprintf_s(buffer, sizeof(buffer), INST_s, ch+1);
    status = viWrite(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);
    if (status < VI_SUCCESS) {
        return status;
    }
    bufferCount = sprintf_s(buffer, sizeof(buffer), format, Value);
    status = viWrite(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);
    return status;
}
ViStatus VISA_POWER_GETValue(ViSession instr, uint32_t ch, const char * format, float *Valueptr)
{
    ViStatus status;
    ViUInt32 retCount;
    ViChar	buffer[MAX_CNT];	/* For checking errors */

    ViUInt32 bufferCount;

    bufferCount = sprintf_s(buffer, sizeof(buffer), INST_s, ch+1);
    status = viWrite(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);
    if (status < VI_SUCCESS) {
        return status;
    }
    bufferCount = sprintf_s(buffer, sizeof(buffer), format);
    status = viWrite(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);
    status = viRead(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);
    if (status < VI_SUCCESS) {
        return status;
    }
    buffer[retCount] = '\0';
    *Valueptr = QString(buffer).toFloat();
    return status;
}
ViStatus VISA_POWER_GETBool(ViSession instr, uint32_t ch, const char * format, ViBuf bufferptr, ViUInt32 bufferptrCnt, ViPUInt32 retCountptr)
{
    ViStatus status;
    ViChar	buffer[MAX_CNT];	/* For checking errors */

    ViUInt32 bufferCount;

    bufferCount = sprintf_s(buffer, sizeof(buffer), INST_s, ch+1);
    status = viWrite(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)retCountptr);
    if (status < VI_SUCCESS) {
        return status;
    }
    bufferCount = sprintf_s(buffer, sizeof(buffer), format, ch+1);
    status = viWrite(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)retCountptr);
    status = viRead(instr, (ViBuf)bufferptr, bufferptrCnt, (ViPUInt32)retCountptr);
    if (status < VI_SUCCESS) {
        return status;
    }
    bufferptr[*retCountptr] = '\0';
    return status;
}
ViStatus VISA_MEAS_GETBool(ViSession instr, uint32_t ch, const char * format, ViBuf bufferptr, ViUInt32 bufferptrCnt, ViPUInt32 retCountptr)
{
    ViStatus status;
    ViChar	buffer[MAX_CNT];	/* For checking errors */

    ViUInt32 bufferCount;
    bufferCount = sprintf_s(buffer, sizeof(buffer), format, ch+1);
    status = viWrite(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)retCountptr);
    status = viRead(instr, (ViBuf)bufferptr, bufferptrCnt, (ViPUInt32)retCountptr);
    if (status < VI_SUCCESS) {
        return status;
    }
    bufferptr[*retCountptr] = '\0';
    return status;
}
void MainWindow::handleOUTP(int id)
{
    if (instr != NULL)
    {
        if (OUTP_list[id]->palette() == p_OFF){
            OUTP_list[id]->setPalette(p_ON);
            OUTP_list[id]->setText(QStringLiteral("输出"));
            VISA_POWER_SETValueBool(instr, id, "ON");
        }
        else{
            OUTP_list[id]->setPalette(p_OFF);
            OUTP_list[id]->setText(QStringLiteral("输出"));
            VISA_POWER_SETValueBool(instr, id, "OFF");
        }
    }
    else
    {
        QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("未连接仪器"));
    }

}
void MainWindow::handleSetParam(int id)
{
    if (instr != NULL)
    {
        VISA_POWER_SETValue(instr, id, VOLT_s, Volt_list[id]->text().toFloat());
        VISA_POWER_SETValue(instr, id, CURR_s, Curr_list[id]->text().toFloat());
        VISA_POWER_SETValue(instr, id, VOLTPROT_s, Voltprot_list[id]->text().toFloat());
        VISA_POWER_SETValue(instr, id, CURRPROT_s, Currprot_list[id]->text().toFloat());
    }
    else
    {
        QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("未连接仪器"));
    }
}
void MainWindow::handleGetParam(int id)
{
    if (instr != NULL)
    {
        ViChar	buffer[MAX_CNT];	/* For checking errors */
        ViUInt32 retCount;
        float Value;

        VISA_POWER_GETBool(instr, id, OUTP_g, (ViBuf)buffer, MAX_CNT, &retCount);
        if (QString::compare(buffer, "ON\n") == 0){
            OUTP_list[id]->setPalette(p_ON);
            OUTP_list[id]->setText(QStringLiteral("输出"));
            VISA_POWER_SETValueBool(instr, id, "ON");
        }
        else{
            OUTP_list[id]->setPalette(p_OFF);
            OUTP_list[id]->setText(QStringLiteral("输出"));
            VISA_POWER_SETValueBool(instr, id, "OFF");
        }

        VISA_POWER_GETValue(instr, id, VOLT_g, &Value);
        Volt_list[id]->setText(QString("%1").arg(Value, 0, 'g',4));
        VISA_POWER_GETValue(instr, id, CURR_g, &Value);
        Curr_list[id]->setText(QString("%1").arg(Value, 0, 'g',4));
        VISA_POWER_GETValue(instr, id, VOLTPROT_g, &Value);
        Voltprot_list[id]->setText(QString("%1").arg(Value, 0, 'g',4));
        VISA_POWER_GETValue(instr, id, CURRPROT_g, &Value);
        Currprot_list[id]->setText(QString("%1").arg(Value, 0, 'g',4));


        VISA_POWER_GETBool(instr, id, VOLTPROTSTAT_g, (ViBuf)buffer, MAX_CNT, &retCount);
        if (QString::compare(buffer, "ON\n") == 0){
            VoltprotLabel_list[id]->setPalette(p_ON);
            VoltprotLabel_list[id]->setText(QStringLiteral("限压(V)"));
            VISA_POWER_SETBool(instr, id, VOLTPROTSTAT_s, "ON");
        }
        else{
            VoltprotLabel_list[id]->setPalette(p_OFF);
            VoltprotLabel_list[id]->setText(QStringLiteral("限压(V)"));
            VISA_POWER_SETBool(instr, id, VOLTPROTSTAT_s, "OFF");
        }
        VISA_POWER_GETBool(instr, id, CURRPROTSTAT_g, (ViBuf)buffer, MAX_CNT, &retCount);
        if (QString::compare(buffer, "ON\n") == 0){
            CurrprotLabel_list[id]->setPalette(p_ON);
            CurrprotLabel_list[id]->setText(QStringLiteral("限流(A)"));
            VISA_POWER_SETBool(instr, id, CURRPROTSTAT_s, "ON");
        }
        else{
            CurrprotLabel_list[id]->setPalette(p_OFF);
            CurrprotLabel_list[id]->setText(QStringLiteral("限流(A)"));
            VISA_POWER_SETBool(instr, id, CURRPROTSTAT_s, "OFF");
        }

    }
    else
    {
        QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("未连接仪器"));
    }


}
void MainWindow::handleVoltprotLabel(int id)
{
    if (instr != NULL)
    {
        if (VoltprotLabel_list[id]->palette() == p_OFF){
            VoltprotLabel_list[id]->setPalette(p_ON);
            VoltprotLabel_list[id]->setText(QStringLiteral("限压(V)"));
            VISA_POWER_SETBool(instr, id, VOLTPROTSTAT_s, "ON");
        }
        else{
            VoltprotLabel_list[id]->setPalette(p_OFF);
            VoltprotLabel_list[id]->setText(QStringLiteral("限压(V)"));
            VISA_POWER_SETBool(instr, id, VOLTPROTSTAT_s, "OFF");
        }
    }
    else
    {
        QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("未连接仪器"));
    }
}
void MainWindow::handleCurrprotLabel(int id)
{
    if (instr != NULL)
    {
        if (CurrprotLabel_list[id]->palette() == p_OFF){
            CurrprotLabel_list[id]->setPalette(p_ON);
            CurrprotLabel_list[id]->setText(QStringLiteral("限流(A)"));
            VISA_POWER_SETBool(instr, id, CURRPROTSTAT_s, "ON");
        }
        else{
            CurrprotLabel_list[id]->setPalette(p_OFF);
            CurrprotLabel_list[id]->setText(QStringLiteral("限流(A)"));
            VISA_POWER_SETBool(instr, id, CURRPROTSTAT_s, "OFF");
        }
    }
    else
    {
        QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("未连接仪器"));
    }
}
void MainWindow::handleMeas(int id)
{
    ViChar	buffer[MAX_CNT];	/* For checking errors */
    ViUInt32 retCount;

    VISA_MEAS_GETBool(instr, id, MEASALL_g, (ViBuf)buffer, MAX_CNT, &retCount);
    QString line(buffer);
    QStringList list = line.split(",");
    MEASVolt_list[id]->setText(QString("%1").arg(list.at(0).toFloat(), 0, 'g',4));
    MEASCurr_list[id]->setText(QString("%1").arg(list.at(1).toFloat(), 0, 'g',4));
    MEASPwrr_list[id]->setText(QString("%1").arg(list.at(2).toFloat(), 0, 'g',4));
}
void MainWindow::on_CONNECTLAN_clicked()
{
    ViUInt32 retCount;
    ViChar	buffer[MAX_CNT];	/* For checking errors */

    if (ui->CONNECTLAN->text() == QStringLiteral("连接")){
        ui->CONNECTLAN->setText(QStringLiteral("断开"));

        /* Communication channels */
        /* Return count from string I/O */
        /* Buffer for string I/O */
        /* Begin by initializing the system */
        status = viOpenDefaultRM(&defaultRM);
        if (status < VI_SUCCESS) {
            QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("Error Initializing VISA..."));
        }
        /* NOTE: For simplicity, we will not show error checking */
        status = viOpen(defaultRM, (ViRsrc)"TCPIP0::192.168.25.3::INSTR", VI_NULL, VI_NULL, &instr);
        if (status < VI_SUCCESS) {
            QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("Error Opening Resource..."));
        }
        /* Set the timeout for message-based communication */
        status = viSetAttribute(instr, VI_ATTR_TMO_VALUE, 5000);
        if (status < VI_SUCCESS) {
            QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("Error Setting Attribute..."));
        }

        for (int i = 0; i < OUTP_list.size(); ++i) {
            handleGetParam(i);
            AllQTimer_list[i]->start(250);
        }

    }
    else{
        ui->CONNECTLAN->setText(QStringLiteral("连接"));

        for (int i = 0; i < OUTP_list.size(); ++i) {
            AllQTimer_list[i]->stop();
        }
        status = viClose(instr);
        status = viClose(defaultRM);
        defaultRM = NULL;
        instr = NULL;
    }

}
