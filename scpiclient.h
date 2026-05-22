#ifndef SCPICLIENT_H
#define SCPICLIENT_H

#include <QObject>
#include <QByteArray>
#include <QString>

class QTcpSocket;

/*
 * ScpiClient
 *
 * 通过 TCP 走 SCPI Raw Socket (默认 5025 端口) 与仪器通信。
 * 替代 NI-VISA：所有 SCPI 字符串命令直接通过 TCP 发送，响应按 \n 终止流式读取。
 *
 * 设计要点：
 * - 长连接：connectToHost 成功后保持连接，直到 disconnectFromHost / 对端断开。
 * - query 按流式累积读取直到出现 \n 或超时（不能假设单次 readyRead 拿到完整行）。
 * - 响应交由调用方 .trimmed()，统一处理 \r\n 与 \n 差异。
 * - 提供 disconnected / errorOccurred 信号让上层在断网时恢复 UI。
 */
class ScpiClient : public QObject
{
    Q_OBJECT
public:
    explicit ScpiClient(QObject *parent = nullptr);
    ~ScpiClient();

    bool connectToHost(const QString &host, quint16 port = 5025, int timeoutMs = 3000);
    void disconnectFromHost();
    bool isConnected() const;

    bool write(const QByteArray &cmd);
    QByteArray query(const QByteArray &cmd, int timeoutMs = 3000);

signals:
    void disconnected();
    void errorOccurred(const QString &message);

private:
    QByteArray ensureTerminated(const QByteArray &cmd) const;

    QTcpSocket *socket_;
};

#endif // SCPICLIENT_H
