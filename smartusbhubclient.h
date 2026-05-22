#ifndef SMARTUSBHUBCLIENT_H
#define SMARTUSBHUBCLIENT_H

#include <QObject>
#include <QByteArray>
#include <QString>

class QSerialPort;

/*
 * SmartUsbHubClient
 *
 * 通过 USB CDC 虚拟串口与 SmartUSBHub（4 端口可编程 USB 集线器）通信。
 *
 * 帧格式：0x55 0x5A CMD DATA[0] DATA[1] [DATA[2]] SUM8
 *   SUM8 = (CMD + DATA[0] + DATA[1] [+ DATA[2]]) & 0xFF
 *   响应长度因命令而异：电压/电流/默认状态查询是 7 字节，其余 6 字节。
 *
 * 同步 API：每个公开方法内部使用 waitForReadyRead，超时 500ms。
 * 设备按键按下时会主动以 CMD 0x00 帧上报状态；sendAndReceive 在等待响应时
 * 会丢弃 CMD 字段不匹配的帧，避免错位。本类不主动暴露按键事件。
 */
class SmartUsbHubClient : public QObject
{
    Q_OBJECT
public:
    enum Channel : quint8 {
        CH1 = 0x01,
        CH2 = 0x02,
        CH3 = 0x04,
        CH4 = 0x08,
        CHAll = 0x0F
    };

    enum WorkMode {
        ModeNormal = 0,
        ModeInterlock = 1
    };

    explicit SmartUsbHubClient(QObject *parent = nullptr);
    ~SmartUsbHubClient() override;

    bool open(const QString &portName, int baud = 115200);
    void close();
    bool isOpen() const;
    QString lastError() const { return lastError_; }

    // 电源
    bool setPower(quint8 channelMask, bool on);            // CMD 0x01
    bool getPower(quint8 channelMask, bool *on);           // CMD 0x00
    bool setPowerInterlock(quint8 channelMask);            // CMD 0x02

    // 数据线
    bool setData(quint8 channelMask, bool on);             // CMD 0x05
    bool getData(quint8 channelMask, bool *on);            // CMD 0x08

    // 测量
    bool getVoltageMv(quint8 channelMask, quint16 *mv);    // CMD 0x03
    bool getCurrentMa(quint8 channelMask, quint16 *ma);    // CMD 0x04

    // 工作模式
    bool setMode(WorkMode mode);                            // CMD 0x06
    bool getMode(WorkMode *mode);                           // CMD 0x07

    // 其余命令（未上 UI，预留）
    bool setButtonEnabled(bool enabled);                    // CMD 0x09
    bool getButtonEnabled(bool *enabled);                   // CMD 0x0A
    bool setPowerDefault(quint8 channelMask, bool enable, bool value);  // CMD 0x0B
    bool getPowerDefault(quint8 channelMask, bool *enable, bool *value);// CMD 0x0C
    bool setDataDefault(quint8 channelMask, bool enable, bool value);   // CMD 0x0D
    bool getDataDefault(quint8 channelMask, bool *enable, bool *value); // CMD 0x0E
    bool setPowerMemory(bool enabled);                      // CMD 0x0F
    bool getPowerMemory(bool *enabled);                     // CMD 0x10
    bool setAddress(quint16 addr);                          // CMD 0x11
    bool getAddress(quint16 *addr);                         // CMD 0x12
    bool factoryReset();                                    // CMD 0xFC
    bool getFirmwareVersion(quint16 *ver);                  // CMD 0xFD
    bool getHardwareVersion(quint16 *ver);                  // CMD 0xFE

signals:
    void disconnected();
    void errorOccurred(const QString &message);

private:
    QByteArray buildFrame6(quint8 cmd, quint8 d0, quint8 d1) const;
    QByteArray buildFrame7(quint8 cmd, quint8 d0, quint8 d1, quint8 d2) const;
    // 发送帧并等待匹配 expectedCmd 的响应帧（长度 expectedLen ∈ {6,7}）。
    // 异步上报或 CMD 不匹配的帧会被丢弃。timeoutMs 是总超时。
    bool sendAndReceive(const QByteArray &frame, quint8 expectedCmd,
                        int expectedLen, QByteArray *resp, int timeoutMs = 500);
    void setError(const QString &msg);

    QSerialPort *port_;
    QString lastError_;
};

#endif // SMARTUSBHUBCLIENT_H
