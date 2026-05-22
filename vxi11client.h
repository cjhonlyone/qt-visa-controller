#ifndef VXI11CLIENT_H
#define VXI11CLIENT_H

#include <QObject>
#include <QByteArray>
#include <QString>

class QTcpSocket;

/*
 * Vxi11Client
 *
 * 自实现的 VXI-11 (Sun ONC RPC over TCP) 客户端，替代 NI-VISA。
 * 用法：
 *   1. 上层先用 portmap GETPORT 拿到设备的 VXI-11 Core 动态端口；
 *   2. connectToHost(ip, port) —— 建立 TCP + CREATE_LINK；
 *   3. write() / query() —— DEVICE_WRITE / DEVICE_READ；
 *   4. disconnectFromHost() —— DESTROY_LINK + TCP 关闭。
 *
 * 协议要点：
 *   - 程序号 = 0x0607AF (395183)，版本 = 1。
 *   - RPC over TCP 用 record marking：4 字节头，bit31 = last-fragment 位，
 *     低 31 位 = 该 fragment 长度。
 *   - XDR：所有原子类型 4 字节对齐，opaque/string = 长度(4) + 数据 + pad。
 *   - device_write 的 flags 位：END=0x08（最后一片）。
 *   - device_read 的 flags 位：TERMCHRSET=0x80（按字符终止）。
 *   - device_read 返回 reason 位：REQCNT=0x01 / CHR=0x02 / END=0x04；
 *     reason=0 表示数据未读完，需继续 read。
 */
class Vxi11Client : public QObject
{
    Q_OBJECT
public:
    static constexpr quint32 kProgram         = 395183u;
    static constexpr quint32 kVersion         = 1u;
    static constexpr quint32 kProcCreateLink  = 10u;
    static constexpr quint32 kProcDeviceWrite = 11u;
    static constexpr quint32 kProcDeviceRead  = 12u;
    static constexpr quint32 kProcDestroyLink = 23u;

    explicit Vxi11Client(QObject *parent = nullptr);
    ~Vxi11Client() override;

    bool connectToHost(const QString &host, quint16 port, int timeoutMs = 3000);
    void disconnectFromHost();
    bool isConnected() const;

    bool write(const QByteArray &data, int ioTimeoutMs = 2000);
    // 同步发送 + 读取直到 END/CHR/REQCNT；返回 trimmed 后的响应。
    QByteArray query(const QByteArray &cmd, int ioTimeoutMs = 2000);

    QString lastError() const { return lastError_; }

signals:
    void disconnected();
    void errorOccurred(const QString &msg);

private:
    bool rpcCall(quint32 proc, const QByteArray &args, QByteArray *result, int timeoutMs);
    bool createLink(int timeoutMs);
    void destroyLink();
    bool readExact(QByteArray *out, int n, int timeoutMs);
    void setError(const QString &msg);

    QTcpSocket *sock_;
    quint32 xid_;
    qint32  lid_;
    quint32 maxRecvSize_;
    QString lastError_;
};

#endif // VXI11CLIENT_H
