#include "scpiclient.h"

#include <QTcpSocket>
#include <QElapsedTimer>
#include <QDebug>

ScpiClient::ScpiClient(QObject *parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
{
    connect(socket_, &QTcpSocket::disconnected, this, &ScpiClient::disconnected);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(socket_, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(socket_->errorString());
    });
#else
    connect(socket_, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(socket_->errorString());
    });
#endif
}

ScpiClient::~ScpiClient()
{
    disconnectFromHost();
}

bool ScpiClient::connectToHost(const QString &host, quint16 port, int timeoutMs)
{
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->abort();
    }
    socket_->connectToHost(host, port);
    if (!socket_->waitForConnected(timeoutMs)) {
        emit errorOccurred(QStringLiteral("连接 %1:%2 失败: %3")
                           .arg(host).arg(port).arg(socket_->errorString()));
        return false;
    }
    return true;
}

void ScpiClient::disconnectFromHost()
{
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->disconnectFromHost();
        if (socket_->state() != QAbstractSocket::UnconnectedState) {
            socket_->waitForDisconnected(500);
        }
    }
}

bool ScpiClient::isConnected() const
{
    return socket_->state() == QAbstractSocket::ConnectedState;
}

QByteArray ScpiClient::ensureTerminated(const QByteArray &cmd) const
{
    if (cmd.endsWith('\n')) {
        return cmd;
    }
    return cmd + '\n';
}

bool ScpiClient::write(const QByteArray &cmd)
{
    if (!isConnected()) {
        emit errorOccurred(QStringLiteral("尚未连接，无法写入: %1").arg(QString::fromLatin1(cmd)));
        return false;
    }
    const QByteArray payload = ensureTerminated(cmd);
    const qint64 written = socket_->write(payload);
    if (written != payload.size()) {
        emit errorOccurred(QStringLiteral("写入失败: %1").arg(socket_->errorString()));
        return false;
    }
    socket_->flush();
    return true;
}

QByteArray ScpiClient::query(const QByteArray &cmd, int timeoutMs)
{
    if (!write(cmd)) {
        return {};
    }

    QByteArray buffer;
    QElapsedTimer timer;
    timer.start();

    // 流式累积：TCP 不保证单次 readyRead 拿到完整一行，循环读到 \n 或超时。
    while (!buffer.contains('\n')) {
        const int remaining = timeoutMs - static_cast<int>(timer.elapsed());
        if (remaining <= 0) {
            emit errorOccurred(QStringLiteral("query 超时: %1").arg(QString::fromLatin1(cmd)));
            break;
        }
        if (!socket_->waitForReadyRead(remaining)) {
            if (socket_->state() != QAbstractSocket::ConnectedState) {
                emit errorOccurred(QStringLiteral("query 期间连接断开: %1").arg(QString::fromLatin1(cmd)));
            } else {
                emit errorOccurred(QStringLiteral("query 等待响应超时: %1").arg(QString::fromLatin1(cmd)));
            }
            break;
        }
        buffer.append(socket_->readAll());
    }
    return buffer;
}
