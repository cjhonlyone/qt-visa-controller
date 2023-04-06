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

    for (int i=1;i<=3;i++)
    {
        OUTP_list.append(ui->tab->findChild<QPushButton *>(QString("OUTP_%1").arg(i)));
        SetParam_list.append(ui->tab->findChild<QPushButton *>(QString("SetParam_%1").arg(i)));
        GetParam_list.append(ui->tab->findChild<QPushButton *>(QString("GetParam_%1").arg(i)));
        VoltLabel_list.append(ui->tab->findChild<QPushButton *>(QString("VoltLabel_%1").arg(i)));
        CurrLabel_list.append(ui->tab->findChild<QPushButton *>(QString("CurrLabel_%1").arg(i)));
        VoltprotLabel_list.append(ui->tab->findChild<QPushButton *>(QString("VoltprotLabel_%1").arg(i)));
        CurrprotLabel_list.append(ui->tab->findChild<QPushButton *>(QString("CurrprotLabel_%1").arg(i)));

        MEASVoltLabel_list.append(ui->tab->findChild<QPushButton *>(QString("MEASVoltLabel_%1").arg(i)));
        MEASCurrLabel_list.append(ui->tab->findChild<QPushButton *>(QString("MEASCurrLabel_%1").arg(i)));
        MEASPwrrLabel_list.append(ui->tab->findChild<QPushButton *>(QString("MEASPwrrLabel_%1").arg(i)));

        Volt_list.append(ui->tab->findChild<QLineEdit *>(QString("Volt_%1").arg(i)));
        Curr_list.append(ui->tab->findChild<QLineEdit *>(QString("Curr_%1").arg(i)));
        Voltprot_list.append(ui->tab->findChild<QLineEdit *>(QString("Voltprot_%1").arg(i)));
        Currprot_list.append(ui->tab->findChild<QLineEdit *>(QString("Currprot_%1").arg(i)));

        MEASVolt_list.append(ui->tab->findChild<QLineEdit *>(QString("MEASVolt_%1").arg(i)));
        MEASCurr_list.append(ui->tab->findChild<QLineEdit *>(QString("MEASCurr_%1").arg(i)));
        MEASPwrr_list.append(ui->tab->findChild<QLineEdit *>(QString("MEASPwrr_%1").arg(i)));
    }

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

    Udp_Send = new QUdpSocket(this);
    Udp_Send->bind(QHostAddress::Any,6111,QUdpSocket::ReuseAddressHint);
    Udp_Recv = new QUdpSocket(this);
    Udp_Recv->bind(QHostAddress::AnyIPv4,6111);
    connect(Udp_Recv,&QUdpSocket::readyRead,this,&MainWindow::processData);

/*********************************************************************/
    for (int i=1;i<=2;i++)
    {
        SDG_OUTP_list.append(ui->tab_2->findChild<QPushButton *>(QString("SDG_OUTP_%1").arg(i)));
        SDG_SendValue_list.append(ui->tab_2->findChild<QPushButton *>(QString("SendValue_%1").arg(i)));
        SDG_ChooseFile_list.append(ui->tab_2->findChild<QPushButton *>(QString("ChooseFile_%1").arg(i)));

        SDG_FilePath_list.append(ui->tab_2->findChild<QLineEdit *>(QString("FilePath_%1").arg(i)));
        SDG_SampleRate_list.append(ui->tab_2->findChild<QLineEdit *>(QString("SampleRate_%1").arg(i)));
        SDG_Amplitude_list.append(ui->tab_2->findChild<QLineEdit *>(QString("Amplitude_%1").arg(i)));
        SDG_Offset_list.append(ui->tab_2->findChild<QLineEdit *>(QString("Offset_%1").arg(i)));
        SDG_Phase_list.append(ui->tab_2->findChild<QLineEdit *>(QString("Phase_%1").arg(i)));
    }

    for (int i = 0; i < SDG_OUTP_list.size(); i++){
        SDG_OUTP_list[i]->setAutoFillBackground(true);
        SDG_OUTP_list[i]->setFlat(true);
        SDG_OUTP_list[i]->setPalette(p_OFF);
        SDG_OUTP_list[i]->setText(QStringLiteral("输出"));
        SDG_OUTP_list[i]->setMinimumWidth(80);

        SDG_SendValue_list[i]->setAutoFillBackground(true);
        SDG_SendValue_list[i]->setFlat(true);
        SDG_SendValue_list[i]->setPalette(p_OFF);
        SDG_SendValue_list[i]->setText(QStringLiteral("设置参数"));
        SDG_SendValue_list[i]->setMinimumWidth(80);

        SDG_ChooseFile_list[i]->setAutoFillBackground(true);
        SDG_ChooseFile_list[i]->setFlat(true);
        SDG_ChooseFile_list[i]->setPalette(p_OFF);
        SDG_ChooseFile_list[i]->setText(QStringLiteral("选择文件"));
        SDG_ChooseFile_list[i]->setMinimumWidth(80);

        SDG_FilePath_list[i]->setMinimumWidth(80);
        SDG_SampleRate_list[i]->setMinimumWidth(80);
        SDG_Amplitude_list[i]->setMinimumWidth(80);
        SDG_Offset_list[i]->setMinimumWidth(80);
        SDG_Phase_list[i]->setMinimumWidth(80);

        SDG_SampleRate_list[i]->setText("2000");
        SDG_Amplitude_list[i]->setText("4");
        SDG_Offset_list[i]->setText("0");
        SDG_Phase_list[i]->setText("0");
    }

    SDG_OUTP_Mapper= new QSignalMapper(this);
    SDG_SendValue_Mapper= new QSignalMapper(this);
    SDG_ChooseFile_Mapper= new QSignalMapper(this);

    for (int i = 0; i < SDG_OUTP_list.size(); i++) {
        connect(SDG_OUTP_list[i], SIGNAL(clicked()), SDG_OUTP_Mapper, SLOT(map()));
        connect(SDG_SendValue_list[i], SIGNAL(clicked()), SDG_SendValue_Mapper, SLOT(map()));
        connect(SDG_ChooseFile_list[i], SIGNAL(clicked()), SDG_ChooseFile_Mapper, SLOT(map()));

        SDG_OUTP_Mapper->setMapping(SDG_OUTP_list[i], i);
        SDG_SendValue_Mapper->setMapping(SDG_SendValue_list[i], i);
        SDG_ChooseFile_Mapper->setMapping(SDG_ChooseFile_list[i], i);
    }

    connect(SDG_OUTP_Mapper, SIGNAL(mapped(int)), this, SLOT(handleSDGOUTP(int)));
    connect(SDG_SendValue_Mapper, SIGNAL(mapped(int)), this, SLOT(handleSDGSendValue(int)));
    connect(SDG_ChooseFile_Mapper, SIGNAL(mapped(int)), this, SLOT(handleSDGChooseFile(int)));
/*********************************************************************/

    defaultRM = NULL;
    instr = NULL;
    defaultRM_SDG = NULL;
    instr_SDG = NULL;

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
#define INSTID_g "*IDN?"
#define send_bin "C%d:WVDT WVNM,testwave,AMPL,%.3f,OFST,%.3f,PHASE,%.4f,WAVEDATA,"
#define set_bin "C%d:ARWV NAME,testwave"
#define SetMode "C%d:SRATE MODE,%s"
#define SetSample "C%d:SRATE VALUE,%.6f"

ViStatus VISA_INST_GETID(ViSession instr, ViSession defaultRM, const char * IPaddr, QString *INSTID)
{
    ViStatus status;
    ViUInt32 retCount;
    ViChar   buffer[MAX_CNT];    /* For checking errors */
    ViUInt32 bufferCount;

    status = viOpenDefaultRM(&defaultRM);
    if (status < VI_SUCCESS) {
        QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("Error Initializing VISA..."));
    }
    char head[256] ="TCPIP0::";
    char tail[] ="::INSTR";
    strcat(head,IPaddr);
    strcat(head,tail);
    status = viOpen(defaultRM, (ViRsrc)head, VI_NULL, VI_NULL, &instr);
    if (status < VI_SUCCESS) {
        QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("Error Opening Resource..."));
    }
    status = viSetAttribute(instr, VI_ATTR_TMO_VALUE, 5000);
    if (status < VI_SUCCESS) {
        QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("Error Setting Attribute..."));
    }
    bufferCount = sprintf_s(buffer, sizeof(buffer), INSTID_g);
    status = viWrite(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);
    status = viRead(instr, (ViBuf)buffer, MAX_CNT, (ViPUInt32)&retCount);
    if (status < VI_SUCCESS) {
        return status;
    }
    buffer[retCount] = '\0';

    *INSTID = QString(buffer);

    status = viClose(instr);
    status = viClose(defaultRM);
    defaultRM = NULL;
    instr = NULL;

    return status;
}
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
void MainWindow::on_SCANLAN_clicked()
{
    ui->comboBox_DP->clear();
    ui->comboBox_SDG->clear();
    uint8_t WlanScanPM[56]={0x00,0x01,0x23,0x45,0x00,0x00,0x00,0x00,
                         0x00,0x00,0x00,0x02,0x00,0x01,0x86,0xa0,
                         0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x03,
                         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                         0x00,0x06,0x07,0xaf,0x00,0x00,0x00,0x01,
                         0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x00};

    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    for (int i = 0; i < ifaces.size(); i++)
    {
        // Now get all IP addresses for the current interface
        QList<QNetworkAddressEntry> addrs = ifaces[i].addressEntries();

        // And for any IP address, if it is IPv4 and the interface is active, send the packet
        for (int j = 0; j < addrs.size(); j++)
            if ((addrs[j].ip().protocol() == QAbstractSocket::IPv4Protocol) && (addrs[j].broadcast().toString() != ""))
                Udp_Send->writeDatagram((const char*)WlanScanPM, 56, addrs[j].broadcast(), 111);
    }

}
void MainWindow::processData()
{
    while(Udp_Recv->hasPendingDatagrams()){
        QHostAddress TargetIP;
        quint16 TargetPort;
        QByteArray dataGram;
        QString INSTID;
        dataGram.resize(Udp_Recv->pendingDatagramSize());
        Udp_Recv->readDatagram(dataGram.data(),dataGram.size(),&TargetIP,&TargetPort);

        QString  TargetIPstr = TargetIP.toString();
        QByteArray TargetIPchba = TargetIPstr.toLatin1();
        char*  TargetIPch = TargetIPchba.data();

        VISA_INST_GETID(instr, defaultRM, TargetIPch, &INSTID);
        QStringList list = INSTID.split(",");
        QString ModelNum = list[1];

        if(ModelNum.contains("DP",Qt::CaseSensitive)) {
            ui->comboBox_DP->addItem(TargetIP.toString() + "--" + ModelNum);
        }
        else if(ModelNum.contains("SDG",Qt::CaseSensitive)) {
            ui->comboBox_SDG->addItem(TargetIP.toString() + "--" + ModelNum);
        }
    }
}
void MainWindow::on_CONNECTLAN_DP_clicked()
{
    ViUInt32 retCount;
    ViChar	buffer[MAX_CNT];	/* For checking errors */

    if (ui->CONNECTLAN_DP->text() == QStringLiteral("连接")){

        if (ui->comboBox_DP->count()==0)
        {
            QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("没有可用仪器"));
            return;
        }

        ui->CONNECTLAN_DP->setText(QStringLiteral("断开"));
        ui->comboBox_DP->setDisabled(true);

        /* Communication channels */
        /* Return count from string I/O */
        /* Buffer for string I/O */
        /* Begin by initializing the system */
        status = viOpenDefaultRM(&defaultRM);
        if (status < VI_SUCCESS) {
            QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("Error Initializing VISA..."));
        }
        QString comboBoxDP = ui->comboBox_DP->currentText();
        QStringList list = comboBoxDP.split("--");
        QString IPaddrstr = list[0];
        QByteArray IPaddrba = IPaddrstr.toLatin1();
        char* IPaddrch;
        IPaddrch = IPaddrba.data();
        char head[256] ="TCPIP0::";
        char tail[] ="::INSTR";
        strcat(head,IPaddrch);
        strcat(head,tail);
        /* NOTE: For simplicity, we will not show error checking */
        status = viOpen(defaultRM, (ViRsrc)head, VI_NULL, VI_NULL, &instr);
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
        ui->CONNECTLAN_DP->setText(QStringLiteral("连接"));
        ui->comboBox_DP->setEnabled(true);

        for (int i = 0; i < OUTP_list.size(); ++i) {
            AllQTimer_list[i]->stop();
        }
        status = viClose(instr);
        status = viClose(defaultRM);
        defaultRM = NULL;
        instr = NULL;
    }
}
void MainWindow::on_CONNECTLAN_SDG_clicked()
{
    ViUInt32 retCount;
    ViChar	buffer[MAX_CNT];	/* For checking errors */

    if (ui->CONNECTLAN_SDG->text() == QStringLiteral("连接")){
        if (ui->comboBox_SDG->count()==0)
        {
            QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("没有可用仪器"));
            return;
        }

        ui->CONNECTLAN_SDG->setText(QStringLiteral("断开"));
        ui->comboBox_SDG->setDisabled(true);

        /* Communication channels */
        /* Return count from string I/O */
        /* Buffer for string I/O */
        /* Begin by initializing the system */
        status = viOpenDefaultRM(&defaultRM_SDG);
        if (status < VI_SUCCESS) {
            QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("Error Initializing VISA..."));
        }
        QString comboBoxSDG = ui->comboBox_SDG->currentText();
        QStringList list = comboBoxSDG.split("--");
        QString IPaddrstr = list[0];
        QByteArray IPaddrba = IPaddrstr.toLatin1();
        char* IPaddrch;
        IPaddrch = IPaddrba.data();
        char head[256] ="TCPIP0::";
        char tail[] ="::INSTR";
        strcat(head,IPaddrch);
        strcat(head,tail);
        /* NOTE: For simplicity, we will not show error checking */
        status = viOpen(defaultRM_SDG, (ViRsrc)head, VI_NULL, VI_NULL, &instr_SDG);
        if (status < VI_SUCCESS) {
            QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("Error Opening Resource..."));
        }
        /* Set the timeout for message-based communication */
        status = viSetAttribute(instr_SDG, VI_ATTR_TMO_VALUE, 5000);
        if (status < VI_SUCCESS) {
            QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("Error Setting Attribute..."));
        }

    }
    else{
        ui->CONNECTLAN_SDG->setText(QStringLiteral("连接"));
        ui->comboBox_SDG->setEnabled(true);

        status = viClose(instr_SDG);
        status = viClose(defaultRM_SDG);
        defaultRM_SDG = NULL;
        instr_SDG = NULL;
    }
}

#define SDG_OUTP_s "C%d:OUTP %s"
ViStatus VISA_SDG_CtrlOUTP(ViSession instr, uint32_t ch, const char* Value)
{
    ViStatus status;
    ViUInt32 retCount;
    ViChar	buffer[MAX_CNT];	/* For checking errors */
    ViUInt32 bufferCount;

    bufferCount = sprintf_s(buffer, sizeof(buffer), SDG_OUTP_s, ch+1, Value);
    status = viWrite(instr, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);

    return status;
}
void MainWindow::handleSDGOUTP(int id)
{
    if (instr_SDG != NULL)
    {
        if (SDG_OUTP_list[id]->palette() == p_OFF){
            SDG_OUTP_list[id]->setPalette(p_ON);
            SDG_OUTP_list[id]->setText(QStringLiteral("输出"));
            VISA_SDG_CtrlOUTP(instr_SDG, id, "ON");
        }
        else{
            SDG_OUTP_list[id]->setPalette(p_OFF);
            SDG_OUTP_list[id]->setText(QStringLiteral("输出"));
            VISA_SDG_CtrlOUTP(instr_SDG, id, "OFF");
        }
    }
    else
    {
        QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("未连接仪器"));
    }
}
uint8_t File_Buf[1024 * 32];
uint8_t * File_BufPtr = (uint8_t *)File_Buf;
void MainWindow::handleSDGSendValue(int id)
{
    if (instr_SDG != NULL)
    {
        QString FilePath = SDG_FilePath_list[id]->text();

        QString SampleRateStr = SDG_SampleRate_list[id]->text();
        QString AmplitudeStr = SDG_Amplitude_list[id]->text();
        QString OffsetStr = SDG_Offset_list[id]->text();
        QString PhaseStr = SDG_Phase_list[id]->text();

        double SampleRate = SampleRateStr.toFloat();
        double Amplitude = AmplitudeStr.toFloat();
        double Offset = OffsetStr.toFloat();
        double Phase = PhaseStr.toFloat();

        QFile file(FilePath);
        if(!file.open(QIODevice::ReadOnly)){
            QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("打开.bin文件失败"));
            return;
        }
        file.read((char *)File_Buf, 1024 * 32);
        ViUInt32 FileSize = file.size();
        file.close();

        ViStatus status;
        ViUInt32 retCount;
        ViChar	buffer[1024*64];	/* For checking errors */
        ViChar * bufferPtr = (ViChar *)buffer;
        ViUInt32 bufferCount;

        bufferCount = sprintf_s(buffer, sizeof(buffer), send_bin, id+1, Amplitude, Offset, Phase);
        qDebug(buffer);
        for (int j=0; j<FileSize; j++)
        {
            *(uint32_t*)(bufferPtr + bufferCount + j) = File_Buf[j];
        }
        status = viWrite(instr_SDG, (ViBuf)buffer, bufferCount+FileSize, (ViPUInt32)&retCount);

        bufferCount = sprintf_s(buffer, sizeof(buffer), set_bin, id+1);
        qDebug(buffer);
        status = viWrite(instr_SDG, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);

        bufferCount = sprintf_s(buffer, sizeof(buffer), SetMode, id+1, "TARB");
        qDebug(buffer);
        status = viWrite(instr_SDG, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);

        bufferCount = sprintf_s(buffer, sizeof(buffer), SetSample, id+1, SampleRate);
        qDebug(buffer);
        status = viWrite(instr_SDG, (ViBuf)buffer, bufferCount, (ViPUInt32)&retCount);
    }
    else
    {
        QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("未连接仪器"));
    }
}
void MainWindow::handleSDGChooseFile(int id)
{
    QString FilePath = QFileDialog::getOpenFileName().replace('\\','/');
    SDG_FilePath_list[id]->setText(FilePath);
}
