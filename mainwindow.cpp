#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QStringList>
#include <QTcpSocket>
#include <QTimer>
#include <QSerialPortInfo>

namespace {

// SCPI 命令模板 —— Rigol DP 系列稳压源。
constexpr const char *kCmdInstSet      = ":INST CH%1\n";
constexpr const char *kCmdOutpSet      = ":OUTP CH%1,%2\n";
constexpr const char *kCmdVoltSet      = ":VOLT %1\n";
constexpr const char *kCmdCurrSet      = ":CURR %1\n";
constexpr const char *kCmdVoltProtSet  = ":VOLT:PROT %1\n";
constexpr const char *kCmdCurrProtSet  = ":CURR:PROT %1\n";
constexpr const char *kCmdVoltProtStat = ":VOLT:PROT:STAT %1\n";
constexpr const char *kCmdCurrProtStat = ":CURR:PROT:STAT %1\n";

constexpr const char *kQryOutp         = ":OUTP? CH%1\n";
constexpr const char *kQryVolt         = ":VOLT?\n";
constexpr const char *kQryCurr         = ":CURR?\n";
constexpr const char *kQryVoltProt     = ":VOLT:PROT?\n";
constexpr const char *kQryCurrProt     = ":CURR:PROT?\n";
constexpr const char *kQryVoltProtStat = ":VOLT:PROT:STAT?\n";
constexpr const char *kQryCurrProtStat = ":CURR:PROT:STAT?\n";
constexpr const char *kQryMeasAll      = ":MEAS:ALL? CH%1\n";

constexpr int kVxi11ProbeConnectTimeoutMs = 1500;
constexpr int kVxi11ConnectTimeoutMs = 3000;

// SmartUSBHub USB VID/PID（CH340 芯片，USB\VID_1A86&PID_FE0C）
constexpr quint16 kHubVid = 0x1A86;
constexpr quint16 kHubPid = 0xFE0C;
constexpr int kHubMeasurePeriodMs = 500;

QByteArray fmtCh(const char *tmpl, int ch1)
{
    return QString(QLatin1String(tmpl)).arg(ch1).toLatin1();
}

QByteArray fmtChStr(const char *tmpl, int ch1, const QString &v)
{
    return QString(QLatin1String(tmpl)).arg(ch1).arg(v).toLatin1();
}

QByteArray fmtVal(const char *tmpl, double v)
{
    // 保持原行为：电压电流用 2 位小数。
    return QString(QLatin1String(tmpl)).arg(v, 0, 'f', 2).toLatin1();
}

QByteArray fmtStr(const char *tmpl, const QString &v)
{
    return QString(QLatin1String(tmpl)).arg(v).toLatin1();
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    p_ON = QPalette();
    p_OFF = QPalette();
    p_ON.setColor(QPalette::Button, QColor("green"));
    p_OFF.setColor(QPalette::Button, QColor("lightgray"));

    setupChannels();
    setupHubChannels();

    udpSend_ = new QUdpSocket(this);
    udpSend_->bind(QHostAddress::Any, 6111, QUdpSocket::ReuseAddressHint);
    udpRecv_ = new QUdpSocket(this);
    udpRecv_->bind(QHostAddress::AnyIPv4, 6111);
    connect(udpRecv_, &QUdpSocket::readyRead, this, &MainWindow::processData);

    // 注意：以下按钮/控件的槽都采用 on_<objectName>_<signal>() 命名规范，
    // setupUi() 会通过 QMetaObject::connectSlotsByName 自动连接一次。
    // 如果再手动 connect 一次，每次点击会触发两次槽，导致连接后立即断开等问题。

    refreshHubPorts();
}

MainWindow::~MainWindow()
{
    if (scpi_) {
        scpi_->disconnectFromHost();
    }
    if (hub_) {
        hub_->close();
    }
    delete ui;
}

void MainWindow::setupChannels()
{
    channels_.reserve(3);
    for (int i = 1; i <= 3; ++i) {
        PowerChannel ch;
        ch.idx = i - 1;
        ch.outp           = ui->tab->findChild<QPushButton *>(QStringLiteral("OUTP_%1").arg(i));
        ch.setParam       = ui->tab->findChild<QPushButton *>(QStringLiteral("SetParam_%1").arg(i));
        ch.getParam       = ui->tab->findChild<QPushButton *>(QStringLiteral("GetParam_%1").arg(i));
        ch.voltLabel      = ui->tab->findChild<QPushButton *>(QStringLiteral("VoltLabel_%1").arg(i));
        ch.currLabel      = ui->tab->findChild<QPushButton *>(QStringLiteral("CurrLabel_%1").arg(i));
        ch.voltProtLabel  = ui->tab->findChild<QPushButton *>(QStringLiteral("VoltprotLabel_%1").arg(i));
        ch.currProtLabel  = ui->tab->findChild<QPushButton *>(QStringLiteral("CurrprotLabel_%1").arg(i));
        ch.measVoltLabel  = ui->tab->findChild<QPushButton *>(QStringLiteral("MEASVoltLabel_%1").arg(i));
        ch.measCurrLabel  = ui->tab->findChild<QPushButton *>(QStringLiteral("MEASCurrLabel_%1").arg(i));
        ch.measPwrLabel   = ui->tab->findChild<QPushButton *>(QStringLiteral("MEASPwrrLabel_%1").arg(i));

        ch.volt     = ui->tab->findChild<QLineEdit *>(QStringLiteral("Volt_%1").arg(i));
        ch.curr     = ui->tab->findChild<QLineEdit *>(QStringLiteral("Curr_%1").arg(i));
        ch.voltProt = ui->tab->findChild<QLineEdit *>(QStringLiteral("Voltprot_%1").arg(i));
        ch.currProt = ui->tab->findChild<QLineEdit *>(QStringLiteral("Currprot_%1").arg(i));
        ch.measVolt = ui->tab->findChild<QLineEdit *>(QStringLiteral("MEASVolt_%1").arg(i));
        ch.measCurr = ui->tab->findChild<QLineEdit *>(QStringLiteral("MEASCurr_%1").arg(i));
        ch.measPwr  = ui->tab->findChild<QLineEdit *>(QStringLiteral("MEASPwrr_%1").arg(i));

        ch.timer = new QTimer(this);
        channels_.append(ch);
    }

    for (int i = 0; i < channels_.size(); ++i) {
        PowerChannel &c = channels_[i];

        if (c.outp) {
            c.outp->setAutoFillBackground(true);
            c.outp->setFlat(true);
            c.outp->setPalette(p_OFF);
            c.outp->setText(QStringLiteral("输出"));
            c.outp->setMinimumWidth(80);
        }
        if (c.voltProtLabel) {
            c.voltProtLabel->setAutoFillBackground(true);
            c.voltProtLabel->setFlat(true);
            c.voltProtLabel->setPalette(p_OFF);
            c.voltProtLabel->setText(QStringLiteral("限压(V)"));
            c.voltProtLabel->setMinimumWidth(80);
        }
        if (c.currProtLabel) {
            c.currProtLabel->setAutoFillBackground(true);
            c.currProtLabel->setFlat(true);
            c.currProtLabel->setPalette(p_OFF);
            c.currProtLabel->setText(QStringLiteral("限流(A)"));
            c.currProtLabel->setMinimumWidth(80);
        }
        if (c.voltLabel) {
            c.voltLabel->setDisabled(true);
            c.voltLabel->setText(QStringLiteral("电压(V)"));
            c.voltLabel->setMinimumWidth(80);
        }
        if (c.currLabel) {
            c.currLabel->setDisabled(true);
            c.currLabel->setText(QStringLiteral("电流(A)"));
            c.currLabel->setMinimumWidth(80);
        }
        if (c.setParam) {
            c.setParam->setText(QStringLiteral("设置参数"));
            c.setParam->setMinimumWidth(80);
        }
        if (c.getParam) {
            c.getParam->setText(QStringLiteral("读取参数"));
            c.getParam->setMinimumWidth(80);
        }
        if (c.volt)     { c.volt->setMinimumWidth(80);     c.volt->setText(QStringLiteral("5")); }
        if (c.curr)     { c.curr->setMinimumWidth(80);     c.curr->setText(QStringLiteral("1.5")); }
        if (c.voltProt) { c.voltProt->setMinimumWidth(80); c.voltProt->setText(QStringLiteral("6")); }
        if (c.currProt) { c.currProt->setMinimumWidth(80); c.currProt->setText(QStringLiteral("2")); }

        if (c.measVoltLabel) {
            c.measVoltLabel->setDisabled(true);
            c.measVoltLabel->setText(QStringLiteral("电压(V)"));
        }
        if (c.measCurrLabel) {
            c.measCurrLabel->setDisabled(true);
            c.measCurrLabel->setText(QStringLiteral("电流(A)"));
        }
        if (c.measPwrLabel) {
            c.measPwrLabel->setDisabled(true);
            c.measPwrLabel->setText(QStringLiteral("功率(W)"));
        }
        if (c.measVolt) { c.measVolt->setDisabled(true); c.measVolt->setMinimumWidth(80); }
        if (c.measCurr) { c.measCurr->setDisabled(true); c.measCurr->setMinimumWidth(80); }
        if (c.measPwr)  { c.measPwr->setDisabled(true);  c.measPwr->setMinimumWidth(80); }

        if (c.outp) {
            connect(c.outp, &QPushButton::clicked, this, [this, i]() { handleOUTP(i); });
        }
        if (c.setParam) {
            connect(c.setParam, &QPushButton::clicked, this, [this, i]() { handleSetParam(i); });
        }
        if (c.getParam) {
            connect(c.getParam, &QPushButton::clicked, this, [this, i]() { handleGetParam(i); });
        }
        if (c.voltProtLabel) {
            connect(c.voltProtLabel, &QPushButton::clicked, this, [this, i]() { handleVoltprotLabel(i); });
        }
        if (c.currProtLabel) {
            connect(c.currProtLabel, &QPushButton::clicked, this, [this, i]() { handleCurrprotLabel(i); });
        }
        connect(c.timer, &QTimer::timeout, this, [this, i]() { handleMeas(i); });
    }
}

bool MainWindow::parseBool(const QByteArray &raw) const
{
    const QByteArray v = raw.trimmed().toUpper();
    return (v == "ON" || v == "1");
}

void MainWindow::writeChannelCommand(int channelIdx, const char *fmt, double value)
{
    if (!scpi_) return;
    // 通道选择 + 设置值。
    scpi_->write(fmtCh(kCmdInstSet, channelIdx + 1));
    scpi_->write(fmtVal(fmt, value));
}

void MainWindow::writeChannelBool(int channelIdx, const char *fmt, bool on)
{
    if (!scpi_) return;
    scpi_->write(fmtCh(kCmdInstSet, channelIdx + 1));
    scpi_->write(fmtStr(fmt, on ? QStringLiteral("ON") : QStringLiteral("OFF")));
}

QByteArray MainWindow::queryChannel(int channelIdx, const char *fmt)
{
    if (!scpi_) return {};
    scpi_->write(fmtCh(kCmdInstSet, channelIdx + 1));
    return scpi_->query(QByteArray(fmt));
}

void MainWindow::handleOUTP(int id)
{
    if (!scpi_ || !scpi_->isConnected()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("未连接仪器"));
        return;
    }
    PowerChannel &c = channels_[id];
    const bool turnOn = (c.outp->palette() == p_OFF);
    c.outp->setPalette(turnOn ? p_ON : p_OFF);
    c.outp->setText(QStringLiteral("输出"));

    // :OUTP CHx,ON/OFF 一次性带通道，不需要先 :INST。
    scpi_->write(fmtChStr(kCmdOutpSet, id + 1, turnOn ? QStringLiteral("ON") : QStringLiteral("OFF")));
}

void MainWindow::handleSetParam(int id)
{
    if (!scpi_ || !scpi_->isConnected()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("未连接仪器"));
        return;
    }
    PowerChannel &c = channels_[id];
    writeChannelCommand(id, kCmdVoltSet,     c.volt->text().toDouble());
    writeChannelCommand(id, kCmdCurrSet,     c.curr->text().toDouble());
    writeChannelCommand(id, kCmdVoltProtSet, c.voltProt->text().toDouble());
    writeChannelCommand(id, kCmdCurrProtSet, c.currProt->text().toDouble());
}

void MainWindow::handleGetParam(int id)
{
    if (!scpi_ || !scpi_->isConnected()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("未连接仪器"));
        return;
    }
    PowerChannel &c = channels_[id];

    // :OUTP? CHx 不用先 :INST。
    const QByteArray outpResp = scpi_->query(fmtCh(kQryOutp, id + 1));
    const bool outpOn = parseBool(outpResp);
    c.outp->setPalette(outpOn ? p_ON : p_OFF);
    c.outp->setText(QStringLiteral("输出"));
    // 与原行为一致：把读取到的状态再写回仪器（保证 UI 状态一致）。
    scpi_->write(fmtChStr(kCmdOutpSet, id + 1, outpOn ? QStringLiteral("ON") : QStringLiteral("OFF")));

    auto setNum = [](QLineEdit *e, const QByteArray &raw) {
        if (!e) return;
        bool ok = false;
        double v = QString::fromLatin1(raw).trimmed().toDouble(&ok);
        e->setText(ok ? QStringLiteral("%1").arg(v, 0, 'g', 4) : QString::fromLatin1(raw.trimmed()));
    };

    setNum(c.volt,     queryChannel(id, kQryVolt));
    setNum(c.curr,     queryChannel(id, kQryCurr));
    setNum(c.voltProt, queryChannel(id, kQryVoltProt));
    setNum(c.currProt, queryChannel(id, kQryCurrProt));

    const bool voltProtOn = parseBool(queryChannel(id, kQryVoltProtStat));
    c.voltProtLabel->setPalette(voltProtOn ? p_ON : p_OFF);
    c.voltProtLabel->setText(QStringLiteral("限压(V)"));
    writeChannelBool(id, kCmdVoltProtStat, voltProtOn);

    const bool currProtOn = parseBool(queryChannel(id, kQryCurrProtStat));
    c.currProtLabel->setPalette(currProtOn ? p_ON : p_OFF);
    c.currProtLabel->setText(QStringLiteral("限流(A)"));
    writeChannelBool(id, kCmdCurrProtStat, currProtOn);
}

void MainWindow::handleVoltprotLabel(int id)
{
    if (!scpi_ || !scpi_->isConnected()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("未连接仪器"));
        return;
    }
    PowerChannel &c = channels_[id];
    const bool turnOn = (c.voltProtLabel->palette() == p_OFF);
    c.voltProtLabel->setPalette(turnOn ? p_ON : p_OFF);
    c.voltProtLabel->setText(QStringLiteral("限压(V)"));
    writeChannelBool(id, kCmdVoltProtStat, turnOn);
}

void MainWindow::handleCurrprotLabel(int id)
{
    if (!scpi_ || !scpi_->isConnected()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("未连接仪器"));
        return;
    }
    PowerChannel &c = channels_[id];
    const bool turnOn = (c.currProtLabel->palette() == p_OFF);
    c.currProtLabel->setPalette(turnOn ? p_ON : p_OFF);
    c.currProtLabel->setText(QStringLiteral("限流(A)"));
    writeChannelBool(id, kCmdCurrProtStat, turnOn);
}

void MainWindow::handleMeas(int id)
{
    if (!scpi_ || !scpi_->isConnected()) return;
    const QByteArray resp = scpi_->query(fmtCh(kQryMeasAll, id + 1));
    const QString line = QString::fromLatin1(resp).trimmed();
    const QStringList parts = line.split(QChar(','));
    if (parts.size() < 3) return;
    PowerChannel &c = channels_[id];
    if (c.measVolt) c.measVolt->setText(QStringLiteral("%1").arg(parts.at(0).toFloat(), 0, 'g', 4));
    if (c.measCurr) c.measCurr->setText(QStringLiteral("%1").arg(parts.at(1).toFloat(), 0, 'g', 4));
    if (c.measPwr)  c.measPwr->setText(QStringLiteral("%1").arg(parts.at(2).toFloat(), 0, 'g', 4));
}

void MainWindow::on_SCANLAN_clicked()
{
    ui->comboBox_DP->clear();
    probeQueue_.clear();
    probedIps_.clear();

    static const uint8_t kPortmapPkt[56] = {
        0x00,0x01,0x23,0x45,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x02,0x00,0x01,0x86,0xa0,
        0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x03,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x06,0x07,0xaf,0x00,0x00,0x00,0x01,
        0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x00
    };

    const QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : ifaces) {
        const QList<QNetworkAddressEntry> addrs = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : addrs) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol) continue;
            if (entry.broadcast().toString().isEmpty()) continue;
            udpSend_->writeDatagram(reinterpret_cast<const char *>(kPortmapPkt),
                                    static_cast<qint64>(sizeof(kPortmapPkt)),
                                    entry.broadcast(), 111);
        }
    }
    statusBar()->showMessage(QStringLiteral("正在扫描..."), 2000);
}

void MainWindow::processData()
{
    while (udpRecv_->hasPendingDatagrams()) {
        QHostAddress targetIp;
        quint16 srcPort = 0;
        QByteArray dg;
        dg.resize(static_cast<int>(udpRecv_->pendingDatagramSize()));
        udpRecv_->readDatagram(dg.data(), dg.size(), &targetIp, &srcPort);

        // 仅接受来自 portmap (111) 的 RPC 应答；最小 28 字节
        // (xid+msg_type+reply_stat+verf{flav,len}+accept_stat+port = 28)。
        // 末尾 4 字节即 VXI-11 Core 在该设备上动态分配的 TCP 端口。
        if (dg.size() < 28 || srcPort != 111) continue;
        const uchar *p = reinterpret_cast<const uchar *>(dg.constData()) + dg.size() - 4;
        const quint32 port = (quint32(p[0]) << 24) | (quint32(p[1]) << 16) |
                             (quint32(p[2]) << 8)  |  quint32(p[3]);
        if (port == 0 || port > 0xFFFFu) continue;   // 0 = 服务未注册

        const QString ip = targetIp.toString();
        if (probedIps_.contains(ip)) continue;
        probedIps_.insert(ip);
        probeQueue_.enqueue(qMakePair(ip, quint16(port)));
    }
    if (!probing_ && !probeQueue_.isEmpty()) {
        probing_ = true;
        QTimer::singleShot(0, this, &MainWindow::probeNext);
    }
}

void MainWindow::probeNext()
{
    if (probeQueue_.isEmpty()) {
        probing_ = false;
        statusBar()->showMessage(QStringLiteral("扫描完成"), 2000);
        return;
    }
    const QPair<QString, quint16> entry = probeQueue_.dequeue();
    const QString ip = entry.first;
    const quint16 port = entry.second;

    // 同步探测：每个候选阻塞 ~1.5s。在主线程执行可以接受，因为 SCAN
    // 已经异步地把所有 IP 收集到队列里再逐个验证。
    Vxi11Client probe;
    bool matched = false;
    if (probe.connectToHost(ip, port, kVxi11ProbeConnectTimeoutMs)) {
        const QByteArray idn = probe.query(QByteArrayLiteral("*IDN?\n"), 1500);
        probe.disconnectFromHost();
        const QString s = QString::fromLatin1(idn).trimmed();
        const QStringList parts = s.split(QChar(','));
        if (parts.size() >= 2) {
            const QString model = parts[1].trimmed();
            if (model.contains(QStringLiteral("DP"), Qt::CaseSensitive)) {
                // 在 itemData 中保存端口，避免后续连接时再做一次 portmap。
                ui->comboBox_DP->addItem(ip + QStringLiteral("--") + model, int(port));
                matched = true;
            } else if (!model.isEmpty()) {
                statusBar()->showMessage(QStringLiteral("%1: 非 DP 设备 (%2)").arg(ip, model), 1500);
            }
        }
    } else {
        statusBar()->showMessage(QStringLiteral("%1:%2 探测失败").arg(ip).arg(port), 1500);
    }
    Q_UNUSED(matched);
    QTimer::singleShot(0, this, &MainWindow::probeNext);
}

void MainWindow::on_CONNECTLAN_DP_clicked()
{
    if (ui->CONNECTLAN_DP->text() == QStringLiteral("连接")) {
        if (ui->comboBox_DP->count() == 0) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("没有可用仪器"));
            return;
        }
        const QString item = ui->comboBox_DP->currentText();
        const QString ip = item.section(QStringLiteral("--"), 0, 0);
        const quint16 port = quint16(ui->comboBox_DP->currentData().toInt());
        if (port == 0) {
            QMessageBox::information(this, QStringLiteral("提示"),
                                     QStringLiteral("条目缺少端口信息，请重新扫描"));
            return;
        }

        scpi_ = new Vxi11Client(this);
        connect(scpi_, &Vxi11Client::errorOccurred, this, [this](const QString &msg) {
            qDebug() << "VXI-11:" << msg;
        });

        if (!scpi_->connectToHost(ip, port, kVxi11ConnectTimeoutMs)) {
            QMessageBox::information(this, QStringLiteral("提示"),
                                     QStringLiteral("连接 %1:%2 失败").arg(ip).arg(port));
            scpi_->deleteLater();
            scpi_ = nullptr;
            return;
        }
        connect(scpi_, &Vxi11Client::disconnected, this, &MainWindow::onScpiDisconnected);

        ui->CONNECTLAN_DP->setText(QStringLiteral("断开"));
        ui->comboBox_DP->setDisabled(true);

        for (int i = 0; i < channels_.size(); ++i) {
            handleGetParam(i);
            if (channels_[i].timer) channels_[i].timer->start(500);
        }
    } else {
        for (PowerChannel &c : channels_) {
            if (c.timer) c.timer->stop();
        }
        if (scpi_) {
            Vxi11Client *c = scpi_;
            scpi_ = nullptr;                 // null first so onScpiDisconnected is a no-op
            disconnect(c, nullptr, this, nullptr);
            c->disconnectFromHost();
            c->deleteLater();
        }
        ui->CONNECTLAN_DP->setText(QStringLiteral("连接"));
        ui->comboBox_DP->setEnabled(true);
    }
}

void MainWindow::onScpiDisconnected()
{
    // 仪器主动断开 / 网线被拔 等：停 timer，UI 恢复到"连接"状态。
    for (PowerChannel &c : channels_) {
        if (c.timer) c.timer->stop();
    }
    if (scpi_) {
        scpi_->deleteLater();
        scpi_ = nullptr;
    }
    if (ui->CONNECTLAN_DP->text() != QStringLiteral("连接")) {
        ui->CONNECTLAN_DP->setText(QStringLiteral("连接"));
        ui->comboBox_DP->setEnabled(true);
        statusBar()->showMessage(QStringLiteral("仪器连接已断开"), 3000);
    }
}

// ============================================================
// SmartUSBHub
// ============================================================

void MainWindow::setupHubChannels()
{
    hubChannels_.reserve(4);
    static const quint8 masks[4] = {0x01, 0x02, 0x04, 0x08};
    for (int i = 1; i <= 4; ++i) {
        UsbHubChannel uc;
        uc.idx = i - 1;
        uc.mask = masks[i - 1];
        uc.power   = ui->tab_hub->findChild<QPushButton *>(QStringLiteral("HUB_POWER_%1").arg(i));
        uc.data    = ui->tab_hub->findChild<QPushButton *>(QStringLiteral("HUB_DATA_%1").arg(i));
        uc.voltage = ui->tab_hub->findChild<QLineEdit *>(QStringLiteral("HUB_VOLT_%1").arg(i));
        uc.current = ui->tab_hub->findChild<QLineEdit *>(QStringLiteral("HUB_CURR_%1").arg(i));
        hubChannels_.append(uc);
    }

    for (int i = 0; i < hubChannels_.size(); ++i) {
        UsbHubChannel &c = hubChannels_[i];
        if (c.power) {
            c.power->setAutoFillBackground(true);
            c.power->setFlat(true);
            c.power->setPalette(p_OFF);
            connect(c.power, &QPushButton::clicked, this, [this, i]() { handleHubPower(i); });
        }
        if (c.data) {
            c.data->setAutoFillBackground(true);
            c.data->setFlat(true);
            c.data->setPalette(p_OFF);
            connect(c.data, &QPushButton::clicked, this, [this, i]() { handleHubData(i); });
        }
    }

    hubMeasureTimer_ = new QTimer(this);
    hubMeasureTimer_->setInterval(kHubMeasurePeriodMs);
    connect(hubMeasureTimer_, &QTimer::timeout, this, &MainWindow::handleHubMeasure);

    hubSetUiConnected(false);
}

void MainWindow::refreshHubPorts()
{
    ui->comboBox_HubPort->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        if (!info.hasVendorIdentifier() || !info.hasProductIdentifier()) continue;
        if (info.vendorIdentifier() != kHubVid || info.productIdentifier() != kHubPid) continue;
        const QString label = info.portName() + QStringLiteral(" (SmartUSBHub)");
        ui->comboBox_HubPort->addItem(label, info.portName());
    }
    if (ui->comboBox_HubPort->count() == 0) {
        ui->comboBox_HubPort->addItem(QStringLiteral("未找到 SmartUSBHub"), QString());
    }
}

void MainWindow::on_REFRESHPORT_HUB_clicked()
{
    refreshHubPorts();
}

void MainWindow::hubSetUiConnected(bool connected)
{
    ui->CONNECT_HUB->setText(connected ? QStringLiteral("断开") : QStringLiteral("连接"));
    ui->comboBox_HubPort->setEnabled(!connected);
    ui->REFRESHPORT_HUB->setEnabled(!connected);
    ui->REFRESHSTATE_HUB->setEnabled(connected);
    ui->comboBox_HubMode->setEnabled(connected);
    for (UsbHubChannel &c : hubChannels_) {
        if (c.power) c.power->setEnabled(connected);
        if (c.data)  c.data->setEnabled(connected);
        if (!connected) {
            if (c.power) c.power->setPalette(p_OFF);
            if (c.data)  c.data->setPalette(p_OFF);
            if (c.voltage) c.voltage->clear();
            if (c.current) c.current->clear();
        }
    }
}

void MainWindow::on_CONNECT_HUB_clicked()
{
    if (ui->CONNECT_HUB->text() == QStringLiteral("连接")) {
        if (ui->comboBox_HubPort->count() == 0) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("未发现可用串口"));
            return;
        }
        const QString portName = ui->comboBox_HubPort->currentData().toString();
        if (!hub_) {
            hub_ = new SmartUsbHubClient(this);
            connect(hub_, &SmartUsbHubClient::disconnected, this, &MainWindow::onHubDisconnected);
            connect(hub_, &SmartUsbHubClient::errorOccurred, this, [this](const QString &msg) {
                qDebug() << "Hub:" << msg;
            });
        }
        if (!hub_->open(portName)) {
            QMessageBox::warning(this, QStringLiteral("提示"),
                                 QStringLiteral("打开 %1 失败: %2").arg(portName, hub_->lastError()));
            return;
        }
        hubSetUiConnected(true);
        hubRefreshFullState();
        hubMeasureTimer_->start();
    } else {
        if (hubMeasureTimer_) hubMeasureTimer_->stop();
        if (hub_) hub_->close();
        hubSetUiConnected(false);
    }
}

void MainWindow::onHubDisconnected()
{
    if (hubMeasureTimer_) hubMeasureTimer_->stop();
    hubSetUiConnected(false);
    statusBar()->showMessage(QStringLiteral("USB Hub 已断开"), 3000);
}

void MainWindow::hubRefreshFullState()
{
    if (!hub_ || !hub_->isOpen()) return;

    SmartUsbHubClient::WorkMode mode;
    if (hub_->getMode(&mode)) {
        QSignalBlocker blocker(ui->comboBox_HubMode);
        ui->comboBox_HubMode->setCurrentIndex(mode == SmartUsbHubClient::ModeInterlock ? 1 : 0);
    }

    for (UsbHubChannel &c : hubChannels_) {
        bool on = false;
        if (hub_->getPower(c.mask, &on) && c.power) {
            c.power->setPalette(on ? p_ON : p_OFF);
        }
        if (hub_->getData(c.mask, &on) && c.data) {
            c.data->setPalette(on ? p_ON : p_OFF);
        }
    }
}

void MainWindow::on_REFRESHSTATE_HUB_clicked()
{
    hubRefreshFullState();
}

void MainWindow::on_comboBox_HubMode_currentIndexChanged(int idx)
{
    if (!hub_ || !hub_->isOpen()) return;
    const auto mode = (idx == 1) ? SmartUsbHubClient::ModeInterlock
                                 : SmartUsbHubClient::ModeNormal;
    hub_->setMode(mode);
    // 模式切换后，互锁会强制改通道状态，同步一次。
    hubRefreshFullState();
}

void MainWindow::handleHubPower(int i)
{
    if (!hub_ || !hub_->isOpen()) return;
    UsbHubChannel &c = hubChannels_[i];
    const bool turnOn = (c.power->palette() == p_OFF);
    bool ok = false;
    // 互锁模式下 CMD 0x01 无效（设备静默拒绝并返回 FF FF FF）。
    // 开启时用 CMD 0x02 打开指定通道；关闭时用 CMD 0x02 0x0F "全部关"。
    if (ui->comboBox_HubMode->currentIndex() == 1) {
        ok = hub_->setPowerInterlock(turnOn ? c.mask : static_cast<quint8>(SmartUsbHubClient::CHAll));
    } else {
        ok = hub_->setPower(c.mask, turnOn);
    }
    if (!ok) {
        statusBar()->showMessage(QStringLiteral("CH%1 电源切换失败").arg(i + 1), 2000);
    }
    // 互锁模式会影响其他通道，整体回读最稳。
    if (ui->comboBox_HubMode->currentIndex() == 1) {
        hubRefreshFullState();
    } else {
        bool state = false;
        if (hub_->getPower(c.mask, &state)) {
            c.power->setPalette(state ? p_ON : p_OFF);
        }
    }
}

void MainWindow::handleHubData(int i)
{
    if (!hub_ || !hub_->isOpen()) return;
    UsbHubChannel &c = hubChannels_[i];
    const bool turnOn = (c.data->palette() == p_OFF);
    if (!hub_->setData(c.mask, turnOn)) {
        statusBar()->showMessage(QStringLiteral("CH%1 数据切换失败").arg(i + 1), 2000);
        return;
    }
    bool state = false;
    if (hub_->getData(c.mask, &state)) {
        c.data->setPalette(state ? p_ON : p_OFF);
    }
}

void MainWindow::handleHubMeasure()
{
    if (!hub_ || !hub_->isOpen()) return;
    for (UsbHubChannel &c : hubChannels_) {
        quint16 mv = 0;
        quint16 ma = 0;
        if (hub_->getVoltageMv(c.mask, &mv) && c.voltage) {
            c.voltage->setText(QString::number(mv));
        }
        if (hub_->getCurrentMa(c.mask, &ma) && c.current) {
            c.current->setText(QString::number(ma));
        }
    }
}
