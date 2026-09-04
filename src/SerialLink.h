#pragma once

#include <QSerialPort>
#include <QTimer>
#include <QVariantList>
#include <QtQmlIntegration>

// Owns the serial port. Emits raw bytes; it knows nothing about lines,
// encodings or the user interface.
class SerialLink : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application as the 'serial' context property")

    Q_PROPERTY(QVariantList ports READ ports NOTIFY portsChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString portName READ portName NOTIFY connectionChanged)
    Q_PROPERTY(bool retrying READ isRetrying NOTIFY connectionChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool statusIsError READ statusIsError NOTIFY statusChanged)

public:
    explicit SerialLink(QObject *parent = nullptr);

    QVariantList ports() const { return m_ports; }
    bool isConnected() const { return m_port.isOpen(); }
    QString portName() const { return m_desiredPort; }
    bool isRetrying() const { return m_retrying && !m_port.isOpen(); }
    QString status() const { return m_status; }
    bool statusIsError() const { return m_statusIsError; }

    Q_INVOKABLE void open(const QString &portName, int baudRate);
    Q_INVOKABLE void close();
    Q_INVOKABLE void pulseReset();

signals:
    void bytesReceived(const QByteArray &data);
    void portsChanged();
    void connectionChanged();
    void statusChanged();

private:
    void scanPorts();
    void readAvailable();
    void handleError(QSerialPort::SerialPortError error);
    void setStatus(const QString &text, bool isError = false);
    QString describeOpenFailure() const;

    QSerialPort m_port;
    QTimer m_scanTimer;
    QVariantList m_ports;
    QString m_status;
    QString m_desiredPort;
    int m_desiredBaud = 115200;
    bool m_retrying = false;  // RF-05: reopen m_desiredPort when it reappears
    bool m_statusIsError = false;
};
