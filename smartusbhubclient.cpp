#include "smartusbhubclient.h"

#include <QSerialPort>
#include <QElapsedTimer>
#include <QtEndian>

namespace {
constexpr quint8 kHeader1 = 0x55;
constexpr quint8 kHeader2 = 0x5A;
}

SmartUsbHubClient::SmartUsbHubClient(QObject *parent)
    : QObject(parent)
    , port_(new QSerialPort(this))
{
    // 端口关闭/错误时主动通知上层。
    connect(port_, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError err) {
        if (err == QSerialPort::NoError) return;
        // ResourceError = 设备被拔走 / IO 错误。
        if (err == QSerialPort::ResourceError || err == QSerialPort::DeviceNotFoundError) {
            if (port_->isOpen()) port_->close();
            emit disconnected();
        }
        emit errorOccurred(port_->errorString());
    });
}

SmartUsbHubClient::~SmartUsbHubClient()
{
    close();
}

bool SmartUsbHubClient::open(const QString &portName, int baud)
{
    if (port_->isOpen()) {
        port_->close();
    }
    port_->setPortName(portName);
    port_->setBaudRate(baud);
    port_->setDataBits(QSerialPort::Data8);
    port_->setParity(QSerialPort::NoParity);
    port_->setStopBits(QSerialPort::OneStop);
    port_->setFlowControl(QSerialPort::NoFlowControl);
    if (!port_->open(QIODevice::ReadWrite)) {
        setError(QStringLiteral("打开 %1 失败: %2").arg(portName, port_->errorString()));
        return false;
    }
    port_->clear();
    return true;
}

void SmartUsbHubClient::close()
{
    if (port_->isOpen()) {
        port_->close();
    }
}

bool SmartUsbHubClient::isOpen() const
{
    return port_->isOpen();
}

void SmartUsbHubClient::setError(const QString &msg)
{
    lastError_ = msg;
    emit errorOccurred(msg);
}

QByteArray SmartUsbHubClient::buildFrame6(quint8 cmd, quint8 d0, quint8 d1) const
{
    QByteArray f(6, 0);
    f[0] = static_cast<char>(kHeader1);
    f[1] = static_cast<char>(kHeader2);
    f[2] = static_cast<char>(cmd);
    f[3] = static_cast<char>(d0);
    f[4] = static_cast<char>(d1);
    f[5] = static_cast<char>((cmd + d0 + d1) & 0xFF);
    return f;
}

QByteArray SmartUsbHubClient::buildFrame7(quint8 cmd, quint8 d0, quint8 d1, quint8 d2) const
{
    QByteArray f(7, 0);
    f[0] = static_cast<char>(kHeader1);
    f[1] = static_cast<char>(kHeader2);
    f[2] = static_cast<char>(cmd);
    f[3] = static_cast<char>(d0);
    f[4] = static_cast<char>(d1);
    f[5] = static_cast<char>(d2);
    f[6] = static_cast<char>((cmd + d0 + d1 + d2) & 0xFF);
    return f;
}

bool SmartUsbHubClient::sendAndReceive(const QByteArray &frame, quint8 expectedCmd,
                                       int expectedLen, QByteArray *resp, int timeoutMs)
{
    if (!port_->isOpen()) {
        setError(QStringLiteral("串口未打开"));
        return false;
    }
    port_->clear(QSerialPort::Input);
    if (port_->write(frame) != frame.size()) {
        setError(QStringLiteral("写入失败: %1").arg(port_->errorString()));
        return false;
    }
    port_->flush();

    QElapsedTimer timer;
    timer.start();
    QByteArray buf;
    while (timer.elapsed() < timeoutMs) {
        const int remaining = timeoutMs - static_cast<int>(timer.elapsed());
        if (remaining <= 0) break;
        if (!port_->waitForReadyRead(remaining)) {
            break;
        }
        buf.append(port_->readAll());

        // 帧解析：在 buf 中查找 0x55 0x5A 帧头，按长度 6 或 7 取出。
        // CMD 不等于 expectedCmd 的视为异步上报或上次残留，丢弃。
        while (buf.size() >= expectedLen) {
            // 对齐到帧头。
            int headIdx = -1;
            for (int i = 0; i + 1 < buf.size(); ++i) {
                if (static_cast<quint8>(buf[i]) == kHeader1 &&
                    static_cast<quint8>(buf[i + 1]) == kHeader2) {
                    headIdx = i;
                    break;
                }
            }
            if (headIdx < 0) {
                buf.clear();
                break;
            }
            if (headIdx > 0) buf.remove(0, headIdx);
            if (buf.size() < expectedLen) break;

            const quint8 cmd = static_cast<quint8>(buf[2]);
            // 异步按键上报 = CMD 0x00 长度 6；如果当前期望命令也是 0x00，仍按匹配处理。
            // 不同命令长度可能不同：如果该 CMD 是 7 字节命令但我们正期望 6 字节，
            // 可能造成残留；这里按 expectedLen 兜底，多余字节下轮重新对齐。
            if (cmd != expectedCmd) {
                // 丢一字节，下一轮重对齐。
                buf.remove(0, 1);
                continue;
            }
            // 校验和
            quint8 sum = 0;
            for (int i = 2; i < expectedLen - 1; ++i) {
                sum = static_cast<quint8>((sum + static_cast<quint8>(buf[i])) & 0xFF);
            }
            if (sum != static_cast<quint8>(buf[expectedLen - 1])) {
                buf.remove(0, 1);
                continue;
            }
            *resp = buf.left(expectedLen);
            buf.remove(0, expectedLen);
            return true;
        }
    }
    setError(QStringLiteral("响应超时 (CMD=0x%1)").arg(expectedCmd, 2, 16, QChar('0')));
    return false;
}

// ====== 电源 ======
bool SmartUsbHubClient::setPower(quint8 ch, bool on)
{
    QByteArray resp;
    return sendAndReceive(buildFrame6(0x01, ch, on ? 1 : 0), 0x01, 6, &resp);
}

bool SmartUsbHubClient::getPower(quint8 ch, bool *on)
{
    QByteArray resp;
    if (!sendAndReceive(buildFrame6(0x00, ch, 0), 0x00, 6, &resp)) return false;
    if (on) *on = (static_cast<quint8>(resp[4]) != 0);
    return true;
}

bool SmartUsbHubClient::setPowerInterlock(quint8 ch)
{
    QByteArray resp;
    return sendAndReceive(buildFrame6(0x02, ch, 1), 0x02, 6, &resp);
}

// ====== 数据 ======
bool SmartUsbHubClient::setData(quint8 ch, bool on)
{
    QByteArray resp;
    return sendAndReceive(buildFrame6(0x05, ch, on ? 1 : 0), 0x05, 6, &resp);
}

bool SmartUsbHubClient::getData(quint8 ch, bool *on)
{
    QByteArray resp;
    if (!sendAndReceive(buildFrame6(0x08, ch, 0), 0x08, 6, &resp)) return false;
    if (on) *on = (static_cast<quint8>(resp[4]) != 0);
    return true;
}

// ====== 测量 ======
bool SmartUsbHubClient::getVoltageMv(quint8 ch, quint16 *mv)
{
    QByteArray resp;
    if (!sendAndReceive(buildFrame6(0x03, ch, 0), 0x03, 7, &resp)) return false;
    if (mv) *mv = (static_cast<quint16>(static_cast<quint8>(resp[4])) << 8) |
                   static_cast<quint8>(resp[5]);
    return true;
}

bool SmartUsbHubClient::getCurrentMa(quint8 ch, quint16 *ma)
{
    QByteArray resp;
    if (!sendAndReceive(buildFrame6(0x04, ch, 0), 0x04, 7, &resp)) return false;
    if (ma) *ma = (static_cast<quint16>(static_cast<quint8>(resp[4])) << 8) |
                   static_cast<quint8>(resp[5]);
    return true;
}

// ====== 模式 ======
bool SmartUsbHubClient::setMode(WorkMode mode)
{
    QByteArray resp;
    return sendAndReceive(buildFrame6(0x06, 0, mode == ModeInterlock ? 1 : 0), 0x06, 6, &resp);
}

bool SmartUsbHubClient::getMode(WorkMode *mode)
{
    QByteArray resp;
    if (!sendAndReceive(buildFrame6(0x07, 0, 0), 0x07, 6, &resp)) return false;
    if (mode) *mode = (static_cast<quint8>(resp[4]) ? ModeInterlock : ModeNormal);
    return true;
}

// ====== 其余预留 ======
bool SmartUsbHubClient::setButtonEnabled(bool enabled)
{
    QByteArray resp;
    return sendAndReceive(buildFrame6(0x09, 0, enabled ? 1 : 0), 0x09, 6, &resp);
}

bool SmartUsbHubClient::getButtonEnabled(bool *enabled)
{
    QByteArray resp;
    if (!sendAndReceive(buildFrame6(0x0A, 0, 0), 0x0A, 6, &resp)) return false;
    if (enabled) *enabled = (static_cast<quint8>(resp[4]) != 0);
    return true;
}

bool SmartUsbHubClient::setPowerDefault(quint8 ch, bool enable, bool value)
{
    QByteArray resp;
    return sendAndReceive(buildFrame7(0x0B, ch, enable ? 1 : 0, value ? 1 : 0), 0x0B, 7, &resp);
}

bool SmartUsbHubClient::getPowerDefault(quint8 ch, bool *enable, bool *value)
{
    QByteArray resp;
    if (!sendAndReceive(buildFrame7(0x0C, ch, 0, 0), 0x0C, 7, &resp)) return false;
    if (enable) *enable = (static_cast<quint8>(resp[4]) != 0);
    if (value)  *value  = (static_cast<quint8>(resp[5]) != 0);
    return true;
}

bool SmartUsbHubClient::setDataDefault(quint8 ch, bool enable, bool value)
{
    QByteArray resp;
    return sendAndReceive(buildFrame7(0x0D, ch, enable ? 1 : 0, value ? 1 : 0), 0x0D, 7, &resp);
}

bool SmartUsbHubClient::getDataDefault(quint8 ch, bool *enable, bool *value)
{
    QByteArray resp;
    if (!sendAndReceive(buildFrame7(0x0E, ch, 0, 0), 0x0E, 7, &resp)) return false;
    if (enable) *enable = (static_cast<quint8>(resp[4]) != 0);
    if (value)  *value  = (static_cast<quint8>(resp[5]) != 0);
    return true;
}

bool SmartUsbHubClient::setPowerMemory(bool enabled)
{
    QByteArray resp;
    return sendAndReceive(buildFrame6(0x0F, 0, enabled ? 1 : 0), 0x0F, 6, &resp);
}

bool SmartUsbHubClient::getPowerMemory(bool *enabled)
{
    QByteArray resp;
    if (!sendAndReceive(buildFrame6(0x10, 0, 0), 0x10, 6, &resp)) return false;
    if (enabled) *enabled = (static_cast<quint8>(resp[4]) != 0);
    return true;
}

bool SmartUsbHubClient::setAddress(quint16 addr)
{
    QByteArray resp;
    return sendAndReceive(buildFrame6(0x11, (addr >> 8) & 0xFF, addr & 0xFF), 0x11, 6, &resp);
}

bool SmartUsbHubClient::getAddress(quint16 *addr)
{
    QByteArray resp;
    if (!sendAndReceive(buildFrame6(0x12, 0, 0), 0x12, 6, &resp)) return false;
    if (addr) *addr = (static_cast<quint16>(static_cast<quint8>(resp[3])) << 8) |
                       static_cast<quint8>(resp[4]);
    return true;
}

bool SmartUsbHubClient::factoryReset()
{
    QByteArray resp;
    return sendAndReceive(buildFrame6(0xFC, 0, 0), 0xFC, 6, &resp);
}

bool SmartUsbHubClient::getFirmwareVersion(quint16 *ver)
{
    QByteArray resp;
    if (!sendAndReceive(buildFrame6(0xFD, 0, 0), 0xFD, 6, &resp)) return false;
    if (ver) *ver = (static_cast<quint16>(static_cast<quint8>(resp[3])) << 8) |
                     static_cast<quint8>(resp[4]);
    return true;
}

bool SmartUsbHubClient::getHardwareVersion(quint16 *ver)
{
    QByteArray resp;
    if (!sendAndReceive(buildFrame6(0xFE, 0, 0), 0xFE, 6, &resp)) return false;
    if (ver) *ver = (static_cast<quint16>(static_cast<quint8>(resp[3])) << 8) |
                     static_cast<quint8>(resp[4]);
    return true;
}
