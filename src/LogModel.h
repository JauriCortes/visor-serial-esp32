#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QUrl>
#include <QtQmlIntegration>

#include "LogLine.h"

// Bounded history of received lines, exposed to QML as a list model.
class LogModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application as the 'logModel' context property")

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int capacity READ capacity WRITE setCapacity NOTIFY capacityChanged)

public:
    // Mirrors Sev::Level; declared here so QML can write LogModel.Error.
    enum Severity { Info = Sev::Info, Warning = Sev::Warning, Error = Sev::Error };
    Q_ENUM(Severity)

    enum Roles { TimeRole = Qt::UserRole + 1, TextRole, SeverityRole, SeedRole };

    explicit LogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return static_cast<int>(m_lines.size()); }
    int capacity() const { return m_capacity; }
    void setCapacity(int capacity);

    Q_INVOKABLE void clear();
    Q_INVOKABLE bool exportToFile(const QUrl &fileUrl);
    Q_INVOKABLE QString lastError() const { return m_lastError; }

public slots:
    void append(const LogLine &line);

signals:
    void countChanged();
    void capacityChanged();
    void lineAppended(const QString &text, int severity, uint seed);

private:
    void flushPending();

    QList<LogLine> m_lines;
    QList<LogLine> m_pending;
    QTimer m_flushTimer;
    QString m_lastError;
    int m_capacity = 5000;
};

// Text + severity filtering for the console view (RF-23).
class LogFilter : public QSortFilterProxyModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application as the 'logFilter' context property")

    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY filterChanged)
    Q_PROPERTY(int minSeverity READ minSeverity WRITE setMinSeverity NOTIFY filterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit LogFilter(QObject *parent = nullptr);

    QString query() const { return m_query; }
    void setQuery(const QString &query);
    int minSeverity() const { return m_minSeverity; }
    void setMinSeverity(int severity);
    int count() const { return rowCount(); }

signals:
    void filterChanged();
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_query;
    int m_minSeverity = Sev::Info;
};
