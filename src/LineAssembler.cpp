#include "LineAssembler.h"

#include <QStringDecoder>

namespace {
// FNV-1a: a small deterministic hash, so the same message always paints the
// same colour (RF-31). qHash() is randomised per process and would not.
quint32 stableHash(const QString &text) {
    quint32 h = 2166136261u;
    for (const QByteArray utf8 = text.toUtf8(); const char c : utf8) {
        h ^= static_cast<quint8>(c);
        h *= 16777619u;
    }
    return h;
}
}  // namespace

LineAssembler::LineAssembler(QObject *parent)
    : QObject(parent),
      m_errorKeywords{QStringLiteral("ERROR"), QStringLiteral("FATAL"),
                      QStringLiteral("FAIL"), QStringLiteral("PANIC"),
                      QStringLiteral("ASSERT")},
      m_warningKeywords{QStringLiteral("WARN"), QStringLiteral("ALERTA"),
                        QStringLiteral("CUIDADO")} {}

void LineAssembler::feed(const QByteArray &data) {
    m_buffer += data;

    qsizetype start = 0;
    for (qsizetype i = 0; i < m_buffer.size(); ++i) {
        const char c = m_buffer.at(i);
        if (c != '\n' && c != '\r')
            continue;
        // RF-11: treating both bytes as terminators and dropping the empty
        // pieces handles \n, \r\n and a bare \r without spurious blank lines.
        emitLine(m_buffer.sliced(start, i - start));
        start = i + 1;
    }
    m_buffer.remove(0, start);

    if (m_buffer.size() >= MaxLineBytes) {
        emitLine(m_buffer);
        m_buffer.clear();
    }
}

void LineAssembler::reset() {
    m_buffer.clear();
}

void LineAssembler::emitLine(const QByteArray &raw) {
    if (raw.isEmpty())
        return;

    // RF-12: invalid bytes become U+FFFD instead of aborting the decode, so the
    // 74880-baud noise the ESP32 bootloader emits cannot break the session.
    QStringDecoder decoder(QStringDecoder::Utf8);
    QString text = decoder(raw);

    text.remove(QChar(u'\0'));
    while (!text.isEmpty() && text.back().isSpace())
        text.chop(1);
    if (text.isEmpty())
        return;

    LogLine line;
    line.timestamp = QDateTime::currentDateTime();
    line.text = text;
    line.severity = classify(text);
    line.seed = stableHash(text);
    emit lineReady(line);
}

int LineAssembler::classify(const QString &text) const {
    const QString upper = text.toUpper();
    for (const QString &keyword : m_errorKeywords)
        if (upper.contains(keyword))
            return Sev::Error;
    for (const QString &keyword : m_warningKeywords)
        if (upper.contains(keyword))
            return Sev::Warning;
    return Sev::Info;
}
