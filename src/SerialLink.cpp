#include "SerialLink.h"

#include <QFileInfo>
#include <QSerialPortInfo>

namespace {
// The scan doubles as hot-plug detection (RF-04) and reconnect polling (RF-05).
constexpr int ScanIntervalMs = 1000;
constexpr int ResetPulseMs = 100;
}  // namespace

SerialLink::SerialLink(QObject *parent) : QObject(parent) {
    connect(&m_port, &QSerialPort::readyRead, this, &SerialLink::readAvailable);
    connect(&m_port, &QSerialPort::errorOccurred, this, &SerialLink::handleError);

    m_scanTimer.setInterval(ScanIntervalMs);
    connect(&m_scanTimer, &QTimer::timeout, this, &SerialLink::scanPorts);
    m_scanTimer.start();

    scanPorts();
    setStatus(tr("Sin conexión"));
}

void SerialLink::scanPorts() {
    QVariantList found;
    bool desiredPresent = false;

    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        QString label = info.portName();
        if (!info.description().isEmpty())
            label += QStringLiteral(" — ") + info.description();
        else if (!info.manufacturer().isEmpty())
            label += QStringLiteral(" — ") + info.manufacturer();

        found.append(QVariantMap{{QStringLiteral("name"), info.portName()},
                                 {QStringLiteral("label"), label}});
        if (info.portName() == m_desiredPort)
            desiredPresent = true;
    }

    if (found != m_ports) {
        m_ports = found;
        emit portsChanged();
    }

    // RF-05: the board was unplugged and is back, so reopen it by itself.
    if (m_retrying && desiredPresent && !m_port.isOpen())
        open(m_desiredPort, m_desiredBaud);
}

void SerialLink::open(const QString &portName, int baudRate) {
    if (m_port.isOpen())
        m_port.close();

    m_desiredPort = portName;
    m_desiredBaud = baudRate;

    m_port.setPortName(portName);
    m_port.setBaudRate(baudRate);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port.open(QIODevice::ReadOnly)) {
        // Keep retrying: the cause is often transient (board still booting).
        m_retrying = true;
        setStatus(describeOpenFailure(), true);
        emit connectionChanged();
        return;
    }

    // RF-07: on CP2102/CH340 boards DTR and RTS drive the auto-reset circuit.
    // The kernel raises both while opening the node, which is harmless: EN only
    // goes low when RTS is high AND DTR is low. So the order matters here.
    // Release RTS first (DTR high, RTS low only pulls IO0, which the running
    // firmware ignores) and DTR afterwards. Dropping DTR first would leave
    // DTR low with RTS still high for a moment -- exactly the combination
    // pulseReset() uses on purpose -- and reboot the board on every connect.
    m_port.setRequestToSend(false);
    m_port.setDataTerminalReady(false);

    m_retrying = true;
    setStatus(tr("Conectado a %1 · %2 baudios").arg(portName).arg(baudRate));
    emit connectionChanged();
}

void SerialLink::close() {
    m_retrying = false;  // an explicit disconnect must not reconnect itself
    if (m_port.isOpen())
        m_port.close();
    setStatus(tr("Desconectado"));
    emit connectionChanged();
}

void SerialLink::pulseReset() {
    if (!m_port.isOpen())
        return;
    // Disagreeing RTS/DTR pulls EN low on the usual auto-reset wiring.
    m_port.setRequestToSend(true);
    QTimer::singleShot(ResetPulseMs, this, [this] {
        if (m_port.isOpen())
            m_port.setRequestToSend(false);
    });
    setStatus(tr("Reinicio enviado a %1").arg(m_desiredPort));
}

void SerialLink::readAvailable() {
    const QByteArray chunk = m_port.readAll();
    if (!chunk.isEmpty())
        emit bytesReceived(chunk);
}

void SerialLink::handleError(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::NoError)
        return;

    // Losing the device mid-session: close cleanly and let scanPorts() retry.
    if (error == QSerialPort::ResourceError || error == QSerialPort::DeviceNotFoundError) {
        if (m_port.isOpen())
            m_port.close();
        setStatus(tr("Se perdió %1. Esperando a que vuelva…").arg(m_desiredPort), true);
        emit connectionChanged();
        return;
    }

    if (error == QSerialPort::ReadError) {
        setStatus(tr("Error de lectura en %1: %2").arg(m_desiredPort, m_port.errorString()), true);
    }
}

QString SerialLink::describeOpenFailure() const {
    const QSerialPortInfo info(m_desiredPort);
    const QString node = info.systemLocation();

    switch (m_port.error()) {
    case QSerialPort::DeviceNotFoundError:
        return tr("El puerto %1 no existe. ¿Está enchufado el ESP32?").arg(m_desiredPort);

    case QSerialPort::PermissionError:
        // Qt reports the same error for "no access rights" and "already in
        // use", so tell them apart by asking whether we may write the node.
        if (!node.isEmpty() && QFileInfo(node).isWritable())
            return tr("El puerto %1 está ocupado por otro programa "
                      "(¿el monitor serie del IDE de Arduino?).")
                .arg(m_desiredPort);
#ifdef Q_OS_LINUX
        return tr("Permiso denegado sobre %1. Agregá tu usuario al grupo del puerto "
                  "y volvé a iniciar sesión:  sudo usermod -aG uucp $USER")
            .arg(node.isEmpty() ? m_desiredPort : node);
#else
        return tr("Permiso denegado sobre %1.").arg(m_desiredPort);
#endif

    default:
        return tr("No se pudo abrir %1: %2").arg(m_desiredPort, m_port.errorString());
    }
}

void SerialLink::setStatus(const QString &text, bool isError) {
    if (m_status == text && m_statusIsError == isError)
        return;
    m_status = text;
    m_statusIsError = isError;
    emit statusChanged();
}
