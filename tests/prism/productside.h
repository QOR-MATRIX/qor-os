// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

// A product's side of the QOR host protocol, as little of it as the tests
// need, for the programs that stand in for ARQADE and QOR Media: what their
// ArqadeLink and MediaLink do. It reads the token from stdin, connects to the
// channel it was given, says hello, and leaves the long poll to its caller.

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QtEndian>
#include <QtNetwork/QLocalSocket>

#include <cstdio>
#include <functional>

class ProductSide : public QObject
{
public:
    std::function<void(const QJsonObject &)> welcomed;
    // An `error` frame: a refusal of the hello (no id) or of a request.
    std::function<void(const QJsonObject &)> refused;
    // A long poll's answer: the command, or null when there was none.
    std::function<void(const QJsonValue &)> answered;
    std::function<void()> lost;

    // The token's line from stdin, where the host wrote it and closed it.
    static QByteArray readToken()
    {
        QFile in;
        if (!in.open(stdin, QIODevice::ReadOnly))
            return {};
        return in.readLine(128).trimmed();
    }

    // The value of --launcher-channel in `arguments`.
    static QString channelIn(const QStringList &arguments)
    {
        const qsizetype at = arguments.indexOf(QStringLiteral("--launcher-channel"));
        return at >= 0 && at + 1 < arguments.size() ? arguments.at(at + 1) : QString();
    }

    void connectTo(const QString &channel, const QJsonObject &hello)
    {
        connect(&m_socket, &QLocalSocket::connected, this, [this, hello] { send(hello); });
        connect(&m_socket, &QLocalSocket::readyRead, this, [this] { read(); });
        connect(&m_socket, &QLocalSocket::disconnected, this, [this] {
            if (lost)
                lost();
        });
        m_socket.connectToServer(channel);
    }

    void ask(const QString &method, const QJsonObject &params = {})
    {
        send({ { QStringLiteral("type"), QStringLiteral("request") },
               { QStringLiteral("id"), ++m_id },
               { QStringLiteral("method"), method },
               { QStringLiteral("params"), params } });
    }

    void send(const QJsonObject &frame)
    {
        const QByteArray body = QJsonDocument(frame).toJson(QJsonDocument::Compact);
        QByteArray length(4, '\0');
        qToBigEndian(quint32(body.size()), length.data());
        writeRaw(length + body);
    }

    // Bytes as they are, framed or not (for the tests of a broken peer).
    void writeRaw(const QByteArray &bytes)
    {
        m_socket.write(bytes);
        m_socket.flush();
    }

private:
    void read()
    {
        m_buffer += m_socket.readAll();
        while (m_buffer.size() >= 4) {
            const quint32 length = qFromBigEndian<quint32>(m_buffer.constData());
            if (m_buffer.size() < 4 + qsizetype(length))
                return;
            const QJsonObject frame = QJsonDocument::fromJson(m_buffer.mid(4, length)).object();
            m_buffer.remove(0, 4 + qsizetype(length));
            const QString type = frame.value(QStringLiteral("type")).toString();
            if (type == QStringLiteral("welcome") && welcomed)
                welcomed(frame);
            else if (type == QStringLiteral("error") && refused)
                refused(frame);
            else if (type == QStringLiteral("result") && answered)
                answered(frame.value(QStringLiteral("result")).toObject().value(QStringLiteral("command")));
        }
    }

    QLocalSocket m_socket;
    QByteArray m_buffer;
    qint64 m_id = 0;
};
