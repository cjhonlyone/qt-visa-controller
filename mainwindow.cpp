#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QStringList>
#include <QTcpSocket>
#include <QTimer>

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
constexpr const char *kQryIdn          = "*IDN?\n";

constexpr quint16 kScpiPort = 5025;
constexpr int kProbeConnectTimeoutMs = 1000;
constexpr int kProbeIdnTimeoutMs = 1500;
constexpr int kScpiConnectTimeoutMs = 3000;

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

    udpSend_ = new QUdpSocket(this);
    udpSend_->bind(QHostAddress::Any, 6111, QUdpSocket::ReuseAddressHint);
    udpRecv_ = new QUdpSocket(this);
    udpRecv_->bind(QHostAddress::AnyIPv4, 6111);
    connect(udpRecv_, &QUdpSocket::readyRead, this, &MainWindow::processData);

    connect(ui->SCANLAN, &QPushButton::clicked, this, &MainWindow::on_SCANLAN_clicked);
    connect(ui->CONNECTLAN_DP, &QPushButton::clicked, this, &MainWindow::on_CONNECTLAN_DP_clicked);
}

MainWindow::~MainWindow()
{
    if (scpi_) {
        scpi_->disconnectFromHost();
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
        quint16 targetPort = 0;
        QByteArray dg;
        dg.resize(static_cast<int>(udpRecv_->pendingDatagramSize()));
        udpRecv_->readDatagram(dg.data(), dg.size(), &targetIp, &targetPort);

        // 过滤 ICMP/无效响应：仅来自 portmap 端口 111 且尾部包含非零数据的视为有效。
        if (dg.size() < 20 || targetPort != 111) continue;
        bool hasNonZeroTail = false;
        for (int i = dg.size() - 8; i < dg.size(); ++i) {
            if (static_cast<unsigned char>(dg[i]) != 0x00) { hasNonZeroTail = true; break; }
        }
        if (!hasNonZeroTail) continue;

        const QString ip = targetIp.toString();
        if (probedIps_.contains(ip)) continue;
        probedIps_.insert(ip);
        probeQueue_.enqueue(ip);
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
    const QString ip = probeQueue_.dequeue();

    QTcpSocket *probe = new QTcpSocket(this);
    QTimer *deadline = new QTimer(this);
    deadline->setSingleShot(true);

    auto cleanup = [this, probe, deadline]() {
        deadline->stop();
        deadline->deleteLater();
        probe->disconnect();
        probe->abort();
        probe->deleteLater();
        QTimer::singleShot(0, this, &MainWindow::probeNext);
    };

    auto fail = [this, ip, cleanup](const QString &reason) {
        statusBar()->showMessage(QStringLiteral("%1: %2").arg(ip, reason), 1500);
        cleanup();
    };

    connect(deadline, &QTimer::timeout, this, [fail]() { fail(QStringLiteral("5025 端口探测超时")); });

    connect(probe, &QTcpSocket::connected, this, [probe]() {
        probe->write(kQryIdn);
    });

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(probe, &QAbstractSocket::errorOccurred, this,
            [fail](QAbstractSocket::SocketError) { fail(QStringLiteral("5025 不可达")); });
#else
    connect(probe, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this,
            [fail](QAbstractSocket::SocketError) { fail(QStringLiteral("5025 不可达")); });
#endif

    QByteArray *acc = new QByteArray;
    connect(probe, &QTcpSocket::readyRead, this, [this, ip, probe, acc, cleanup]() {
        acc->append(probe->readAll());
        if (!acc->contains('\n')) return;
        const QString idn = QString::fromLatin1(*acc).trimmed();
        delete acc;
        const QStringList parts = idn.split(QChar(','));
        if (parts.size() >= 2) {
            const QString model = parts[1].trimmed();
            if (model.contains(QStringLiteral("DP"), Qt::CaseSensitive)) {
                ui->comboBox_DP->addItem(ip + QStringLiteral("--") + model);
            } else {
                statusBar()->showMessage(QStringLiteral("%1: 非 DP 设备 (%2)").arg(ip, model), 1500);
            }
        }
        cleanup();
    });

    deadline->start(kProbeConnectTimeoutMs + kProbeIdnTimeoutMs);
    probe->connectToHost(ip, kScpiPort);
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

        scpi_ = new ScpiClient(this);
        connect(scpi_, &ScpiClient::disconnected, this, &MainWindow::onScpiDisconnected);
        connect(scpi_, &ScpiClient::errorOccurred, this, [this](const QString &msg) {
            qDebug() << "SCPI:" << msg;
        });

        if (!scpi_->connectToHost(ip, kScpiPort, kScpiConnectTimeoutMs)) {
            QMessageBox::information(this, QStringLiteral("提示"),
                                     QStringLiteral("连接 %1:%2 失败").arg(ip).arg(kScpiPort));
            scpi_->deleteLater();
            scpi_ = nullptr;
            return;
        }

        ui->CONNECTLAN_DP->setText(QStringLiteral("断开"));
        ui->comboBox_DP->setDisabled(true);

        for (int i = 0; i < channels_.size(); ++i) {
            handleGetParam(i);
            if (channels_[i].timer) channels_[i].timer->start(250);
        }
    } else {
        for (PowerChannel &c : channels_) {
            if (c.timer) c.timer->stop();
        }
        if (scpi_) {
            scpi_->disconnectFromHost();
            scpi_->deleteLater();
            scpi_ = nullptr;
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
