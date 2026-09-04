#pragma once

#include <QObject>
#include <QStringList>

#include "LogLine.h"

// Turns the byte stream into complete, decoded, classified lines.
// Deliberately free of Qt Quick and of QSerialPort so it can be tested
// by feeding it byte arrays (RNF-08).
class LineAssembler : public QObject {
    Q_OBJECT

public:
    // A device that never sends a terminator must not grow the buffer forever.
    static constexpr int MaxLineBytes = 64 * 1024;

    explicit LineAssembler(QObject *parent = nullptr);

    // RF-14: the keyword lists are configurable, these are the defaults.
    void setErrorKeywords(const QStringList &keywords) { m_errorKeywords = keywords; }
    void setWarningKeywords(const QStringList &keywords) { m_warningKeywords = keywords; }

public slots:
    void feed(const QByteArray &data);
    void reset();

signals:
    void lineReady(const LogLine &line);

private:
    void emitLine(const QByteArray &raw);
    int classify(const QString &text) const;

    QByteArray m_buffer;
    QStringList m_errorKeywords;
    QStringList m_warningKeywords;
};
