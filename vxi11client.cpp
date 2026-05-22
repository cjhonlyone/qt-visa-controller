#include "vxi11client.h"

#include <QTcpSocket>
#include <QtEndian>
#include <QElapsedTimer>

namespace {

void appendU32(QByteArray &b, quint32 v)
{
    char buf[4];
    qToBigEndian<quint32>(v, buf);
    b.append(buf, 4);
}

void appendI32(QByteArray &b, qint32 v)
{
    appendU32(b, static_cast<quint32>(v));
}

void appendOpaque(QByteArray &b, const QByteArray &data)
{
    appendU32(b, static_cast<quint32>(data.size()));
    b.append(data);
    const int pad = (4 - (data.size() & 3)) & 3;
    if (pad) b.append(QByteArray(pad, '\0'));
}

quint32 readU32(const QByteArray &b, int off)
{
    return qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(b.constData()) + off);
}

} // namespace

Vxi11Client::Vxi11Client(QObject *parent)
    : QObject(parent)
    , sock_(new QTcpSocket(this))
    , xid_(1)
    , lid_(-1)
    , maxRecvSize_(1024 * 1024)
{
    connect(sock_, &QTcpSocket::disconnected, this, [this]() {
        lid_ = -1;
        emit disconnected();
    });
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(sock_, &QAbstractSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) {
                emit errorOccurred(sock_->errorString());
            });
#else
    connect(sock_, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this,
            [this](QAbstractSocket::SocketError) {
                emit errorOccurred(sock_->errorString());
            });
#endif
}

Vxi11Client::~Vxi11Client()
{
    disconnectFromHost();
}

void Vxi11Client::setError(const QString &msg)
{
    lastError_ = msg;
    emit errorOccurred(msg);
}

bool Vxi11Client::connectToHost(const QString &host, quint16 port, int timeoutMs)
{
    sock_->abort();
    lid_ = -1;
    sock_->connectToHost(host, port);
    if (!sock_->waitForConnected(timeoutMs)) {
        setError(QStringLiteral("TCP 连接 %1:%2 失败: %3")
                     .arg(host).arg(port).arg(sock_->errorString()));
        return false;
    }
    if (!createLink(timeoutMs)) {
        sock_->disconnectFromHost();
        if (sock_->state() != QAbstractSocket::UnconnectedState) {
            sock_->waitForDisconnected(300);
        }
        return false;
    }
    return true;
}

void Vxi11Client::disconnectFromHost()
{
    if (sock_->state() == QAbstractSocket::ConnectedState) {
        if (lid_ >= 0) destroyLink();
        sock_->disconnectFromHost();
        if (sock_->state() != QAbstractSocket::UnconnectedState) {
            sock_->waitForDisconnected(500);
        }
    }
    lid_ = -1;
}

bool Vxi11Client::isConnected() const
{
    return sock_->state() == QAbstractSocket::ConnectedState && lid_ >= 0;
}

bool Vxi11Client::readExact(QByteArray *out, int n, int timeoutMs)
{
    QElapsedTimer t;
    t.start();
    while (out->size() < n) {
        const int rem = timeoutMs - static_cast<int>(t.elapsed());
        if (rem <= 0) return false;
        if (sock_->bytesAvailable() == 0 && !sock_->waitForReadyRead(rem)) return false;
        out->append(sock_->read(n - out->size()));
        if (sock_->state() != QAbstractSocket::ConnectedState && sock_->bytesAvailable() == 0) {
            return out->size() >= n;
        }
    }
    return true;
}

bool Vxi11Client::rpcCall(quint32 proc, const QByteArray &args, QByteArray *result, int timeoutMs)
{
    if (sock_->state() != QAbstractSocket::ConnectedState) {
        setError(QStringLiteral("RPC: 未连接"));
        return false;
    }
    const quint32 xid = xid_++;

    QByteArray body;
    body.reserve(40 + args.size());
    appendU32(body, xid);
    appendU32(body, 0);          // msg_type = CALL
    appendU32(body, 2);          // rpcvers
    appendU32(body, kProgram);
    appendU32(body, kVersion);
    appendU32(body, proc);
    appendU32(body, 0); appendU32(body, 0);   // cred = AUTH_NULL
    appendU32(body, 0); appendU32(body, 0);   // verf = AUTH_NULL
    body.append(args);

    QByteArray frame;
    appendU32(frame, 0x80000000u | static_cast<quint32>(body.size()));
    frame.append(body);
    if (sock_->write(frame) != frame.size()) {
        setError(QStringLiteral("RPC: 写入失败"));
        return false;
    }
    sock_->flush();

    QByteArray reply;
    QElapsedTimer t;
    t.start();
    bool last = false;
    while (!last) {
        int rem = timeoutMs - static_cast<int>(t.elapsed());
        if (rem <= 0) { setError(QStringLiteral("RPC: 超时")); return false; }
        QByteArray hdr;
        if (!readExact(&hdr, 4, rem)) { setError(QStringLiteral("RPC: 读取帧头超时")); return false; }
        const quint32 h = readU32(hdr, 0);
        last = (h & 0x80000000u) != 0;
        const quint32 flen = h & 0x7FFFFFFFu;
        rem = timeoutMs - static_cast<int>(t.elapsed());
        if (rem <= 0) { setError(QStringLiteral("RPC: 超时")); return false; }
        QByteArray frag;
        if (!readExact(&frag, static_cast<int>(flen), rem)) {
            setError(QStringLiteral("RPC: 读取分片超时"));
            return false;
        }
        reply.append(frag);
    }

    int off = 0;
    if (reply.size() < 24) { setError(QStringLiteral("RPC: 响应过短")); return false; }
    const quint32 rxid = readU32(reply, off); off += 4;
    if (rxid != xid) { setError(QStringLiteral("RPC: XID 不匹配")); return false; }
    if (readU32(reply, off) != 1u) { setError(QStringLiteral("RPC: 非 REPLY")); return false; }
    off += 4;
    if (readU32(reply, off) != 0u) { setError(QStringLiteral("RPC: MSG_DENIED")); return false; }
    off += 4;
    off += 4;                                          // verf flavor
    const quint32 vlen = readU32(reply, off); off += 4;
    off += static_cast<int>((vlen + 3u) & ~3u);        // verf body (padded)
    if (off + 4 > reply.size()) { setError(QStringLiteral("RPC: 截断")); return false; }
    if (readU32(reply, off) != 0u) {
        setError(QStringLiteral("RPC: accept_stat != SUCCESS"));
        return false;
    }
    off += 4;
    *result = reply.mid(off);
    return true;
}

bool Vxi11Client::createLink(int timeoutMs)
{
    QByteArray args;
    appendU32(args, 0);                         // clientId
    appendU32(args, 0);                         // lockDevice = false
    appendU32(args, 0);                         // lock_timeout
    appendOpaque(args, QByteArrayLiteral("inst0"));
    QByteArray r;
    if (!rpcCall(kProcCreateLink, args, &r, timeoutMs)) return false;
    if (r.size() < 16) { setError(QStringLiteral("CREATE_LINK 响应过短")); return false; }
    const quint32 err = readU32(r, 0);
    if (err != 0) {
        setError(QStringLiteral("CREATE_LINK error=%1").arg(err));
        return false;
    }
    lid_ = static_cast<qint32>(readU32(r, 4));
    // r[8..11] = abortPort (XDR 4 字节)，本客户端不用 abort 通道。
    maxRecvSize_ = readU32(r, 12);
    if (maxRecvSize_ == 0 || maxRecvSize_ > 16u * 1024u * 1024u) {
        maxRecvSize_ = 1024 * 1024;
    }
    return true;
}

void Vxi11Client::destroyLink()
{
    QByteArray args;
    appendI32(args, lid_);
    QByteArray r;
    rpcCall(kProcDestroyLink, args, &r, 500);
    lid_ = -1;
}

bool Vxi11Client::write(const QByteArray &data, int ioTimeoutMs)
{
    if (!isConnected()) { setError(QStringLiteral("未连接")); return false; }
    if (data.isEmpty()) return true;
    int sent = 0;
    while (sent < data.size()) {
        const int chunk = qMin<int>(static_cast<int>(maxRecvSize_), data.size() - sent);
        const bool last = (sent + chunk == data.size());
        QByteArray args;
        appendI32(args, lid_);
        appendU32(args, static_cast<quint32>(ioTimeoutMs));
        appendU32(args, 0);                          // lock_timeout
        appendU32(args, last ? 0x08u : 0u);          // END on last
        appendOpaque(args, data.mid(sent, chunk));
        QByteArray r;
        if (!rpcCall(kProcDeviceWrite, args, &r, ioTimeoutMs + 1000)) return false;
        if (r.size() < 8) { setError(QStringLiteral("DEVICE_WRITE 响应过短")); return false; }
        const quint32 err = readU32(r, 0);
        if (err != 0) {
            setError(QStringLiteral("DEVICE_WRITE error=%1").arg(err));
            return false;
        }
        sent += chunk;
    }
    return true;
}

QByteArray Vxi11Client::query(const QByteArray &cmd, int ioTimeoutMs)
{
    if (!write(cmd, ioTimeoutMs)) return {};
    QByteArray out;
    while (true) {
        QByteArray args;
        appendI32(args, lid_);
        appendU32(args, maxRecvSize_);                  // requestSize
        appendU32(args, static_cast<quint32>(ioTimeoutMs));
        appendU32(args, 0);                              // lock_timeout
        appendU32(args, 0x80u);                          // flags: TERMCHRSET
        appendU32(args, static_cast<quint32>('\n'));     // termChar
        QByteArray r;
        if (!rpcCall(kProcDeviceRead, args, &r, ioTimeoutMs + 1000)) return {};
        if (r.size() < 12) { setError(QStringLiteral("DEVICE_READ 响应过短")); return {}; }
        const quint32 err = readU32(r, 0);
        if (err != 0) {
            setError(QStringLiteral("DEVICE_READ error=%1").arg(err));
            return {};
        }
        const quint32 reason = readU32(r, 4);
        const quint32 dlen = readU32(r, 8);
        if (r.size() < static_cast<int>(12u + dlen)) {
            setError(QStringLiteral("DEVICE_READ 数据截断"));
            return {};
        }
        out.append(r.mid(12, static_cast<int>(dlen)));
        if (reason != 0) break;     // 0x01 REQCNT / 0x02 CHR / 0x04 END
    }
    return out.trimmed();
}
