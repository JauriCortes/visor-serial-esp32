#include <QGuiApplication>
#include <QRandomGenerator>
#include <QTimer>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "LineAssembler.h"
#include "LogModel.h"
#include "SerialLink.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("TecnologiaDigital"));
    app.setApplicationName(QStringLiteral("Esp32Visor"));

    // Material carries a real dark palette for the stock controls; Basic
    // only ever draws them light, which clashed with the dark views.
    QQuickStyle::setStyle(QStringLiteral("Material"));

    // The pipeline of RNF-08: each stage only knows the one after it.
    SerialLink link;
    LineAssembler assembler;
    LogModel model;
    LogFilter filter;
    filter.setSourceModel(&model);

    QObject::connect(&link, &SerialLink::bytesReceived, &assembler, &LineAssembler::feed);
    QObject::connect(&assembler, &LineAssembler::lineReady, &model, &LogModel::append);
    // Drop any half-received line so a reconnect cannot splice two messages.
    QObject::connect(&link, &SerialLink::connectionChanged, &assembler, &LineAssembler::reset);

    // Development aid: --demo pushes synthetic bytes into the same entry point
    // the serial port uses, so both views can be exercised with no board
    // attached. Delete this block and nothing else changes.
    QTimer demoTimer;
    if (app.arguments().contains(QStringLiteral("--demo"))) {
        static const QList<QByteArray> samples = {
            "Sistema iniciado\n",
            "WiFi conectado, IP 192.168.1.42\n",
            "Lectura sensor: 24.5 C\n",
            "Boton presionado\n",
            "WARNING: temperatura alta\n",
            "ERROR: no responde el sensor I2C\n",
            QString::fromUtf8("Medición: 24,5 °C — ñandú\n").toUtf8(),
        };
        // Owned by the lambda: a local captured by reference would dangle as
        // soon as this block ends.
        QObject::connect(&demoTimer, &QTimer::timeout, &assembler, [&assembler, tick = 0]() mutable {
            if (++tick % 25 == 0) {  // a burst, to exercise RNF-02
                QByteArray burst;
                for (int i = 0; i < 200; ++i)
                    burst += QByteArray("Rafaga linea ") + QByteArray::number(i) + '\n';
                assembler.feed(burst);
                return;
            }
            assembler.feed(samples.at(QRandomGenerator::global()->bounded(samples.size())));
        });
        demoTimer.start(600);
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("serial"), &link);
    engine.rootContext()->setContextProperty(QStringLiteral("logModel"), &model);
    engine.rootContext()->setContextProperty(QStringLiteral("logFilter"), &filter);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Esp32Visor", "Main");

    return app.exec();
}
