#include "LogModel.h"

#include <QFile>
#include <QTextStream>

namespace {
// One flush per frame: a burst of hundreds of lines becomes a handful of
// model insertions instead of one repaint each (RNF-02, RNF-03).
constexpr int FlushIntervalMs = 16;

QString severityTag(int severity) {
    switch (severity) {
    case Sev::Error:
        return QStringLiteral("ERROR");
    case Sev::Warning:
        return QStringLiteral("WARN ");
    default:
        return QStringLiteral("INFO ");
    }
}
}  // namespace

LogModel::LogModel(QObject *parent) : QAbstractListModel(parent) {
    m_flushTimer.setSingleShot(true);
    m_flushTimer.setInterval(FlushIntervalMs);
    connect(&m_flushTimer, &QTimer::timeout, this, &LogModel::flushPending);
}

int LogModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_lines.size());
}

QVariant LogModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_lines.size())
        return {};

    const LogLine &line = m_lines.at(index.row());
    switch (role) {
    case TimeRole:
        return line.timestamp.toString(QStringLiteral("HH:mm:ss.zzz"));  // RF-13
    case TextRole:
        return line.text;
    case SeverityRole:
        return line.severity;
    case SeedRole:
        return line.seed;
    default:
        return {};
    }
}

QHash<int, QByteArray> LogModel::roleNames() const {
    return {{TimeRole, "time"}, {TextRole, "text"}, {SeverityRole, "severity"}, {SeedRole, "seed"}};
}

void LogModel::append(const LogLine &line) {
    m_pending.append(line);
    if (!m_flushTimer.isActive())
        m_flushTimer.start();
}

void LogModel::flushPending() {
    if (m_pending.isEmpty())
        return;

    const int first = static_cast<int>(m_lines.size());
    beginInsertRows({}, first, first + static_cast<int>(m_pending.size()) - 1);
    m_lines += m_pending;
    endInsertRows();

    const QList<LogLine> justAdded = std::exchange(m_pending, {});

    // RF-15: drop the oldest lines so memory stays bounded.
    const qsizetype excess = m_lines.size() - m_capacity;
    if (excess > 0) {
        beginRemoveRows({}, 0, static_cast<int>(excess) - 1);
        m_lines.remove(0, excess);
        endRemoveRows();
    }

    emit countChanged();
    for (const LogLine &line : justAdded)
        emit lineAppended(line.text, line.severity, line.seed);
}

void LogModel::setCapacity(int capacity) {
    capacity = qMax(1, capacity);
    if (m_capacity == capacity)
        return;
    m_capacity = capacity;
    emit capacityChanged();

    const qsizetype excess = m_lines.size() - m_capacity;
    if (excess > 0) {
        beginRemoveRows({}, 0, static_cast<int>(excess) - 1);
        m_lines.remove(0, excess);
        endRemoveRows();
        emit countChanged();
    }
}

void LogModel::clear() {
    m_flushTimer.stop();
    m_pending.clear();
    beginResetModel();
    m_lines.clear();
    endResetModel();
    emit countChanged();
}

bool LogModel::exportToFile(const QUrl &fileUrl) {
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        m_lastError = tr("No se pudo escribir %1: %2").arg(path, file.errorString());
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    for (const LogLine &line : std::as_const(m_lines)) {
        out << line.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")) << " ["
            << severityTag(line.severity) << "] " << line.text << '\n';
    }
    out.flush();

    if (file.error() != QFile::NoError) {
        m_lastError = tr("Error al guardar %1: %2").arg(path, file.errorString());
        return false;
    }
    m_lastError.clear();
    return true;
}

// ---------------------------------------------------------------- LogFilter

LogFilter::LogFilter(QObject *parent) : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(true);
    for (auto signal : {&QAbstractItemModel::rowsInserted, &QAbstractItemModel::rowsRemoved})
        connect(this, signal, this, &LogFilter::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &LogFilter::countChanged);
}

void LogFilter::setQuery(const QString &query) {
    if (m_query == query)
        return;
    m_query = query;
    invalidate();  // invalidateFilter() is deprecated; we never sort, so the cost is the same
    emit filterChanged();
    emit countChanged();
}

void LogFilter::setMinSeverity(int severity) {
    if (m_minSeverity == severity)
        return;
    m_minSeverity = severity;
    invalidate();  // invalidateFilter() is deprecated; we never sort, so the cost is the same
    emit filterChanged();
    emit countChanged();
}

bool LogFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    const QAbstractItemModel *source = sourceModel();
    if (!source)
        return false;

    const QModelIndex index = source->index(sourceRow, 0, sourceParent);
    if (index.data(LogModel::SeverityRole).toInt() < m_minSeverity)
        return false;
    if (m_query.isEmpty())
        return true;
    return index.data(LogModel::TextRole).toString().contains(m_query, Qt::CaseInsensitive);
}
