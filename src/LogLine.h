#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>

// Severity is kept in its own namespace so the pure-logic classes
// (LineAssembler) do not have to include the model.
namespace Sev {
enum Level { Info = 0, Warning = 1, Error = 2 };
}

// A single decoded message received from the device.
struct LogLine {
    QDateTime timestamp;
    QString text;
    int severity = Sev::Info;
    quint32 seed = 0;  // stable hash of the text; drives the generative colours
};

Q_DECLARE_METATYPE(LogLine)
