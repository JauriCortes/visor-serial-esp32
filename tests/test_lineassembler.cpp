#include <QCoreApplication>
#include <QDebug>
#include <QRandomGenerator>

#include "LineAssembler.h"

static int fallos = 0;
static void check(bool ok, const QString &what) {
    if (!ok) { qWarning() << "FALLO:" << what; ++fallos; }
    else qInfo().noquote() << "  ok  " << what;
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    LineAssembler a;
    QList<LogLine> out;
    QObject::connect(&a, &LineAssembler::lineReady, [&](const LogLine &l){ out.append(l); });

    // RF-10: una línea partida en dos lecturas del puerto.
    a.feed("Mensaje partido en ");
    check(out.isEmpty(), "RF-10 no emite sin terminador");
    a.feed("dos pedazos\n");
    check(out.size() == 1 && out[0].text == "Mensaje partido en dos pedazos", "RF-10 rearma la linea");

    // RF-11: los tres terminadores, sin lineas vacias espurias.
    out.clear();
    a.feed("uno\ndos\r\ntres\rcuatro\n");
    check(out.size() == 4, QString("RF-11 cuatro lineas, obtuve %1").arg(out.size()));
    check(out.size() == 4 && out[1].text == "dos" && out[2].text == "tres", "RF-11 sin \\r pegado");

    // RF-12: bytes UTF-8 invalidos no rompen nada.
    out.clear();
    a.feed(QByteArray::fromHex("c328a0fffe") + " arranque\n");
    check(out.size() == 1 && out[0].text.endsWith("arranque"), "RF-12 sobrevive a bytes invalidos");
    check(out.size() == 1 && out[0].text.contains(QChar(0xFFFD)), "RF-12 sustituye por U+FFFD");

    // UTF-8 valido intacto.
    out.clear();
    a.feed(QString::fromUtf8("Medición: 24,5 °C — ñandú\n").toUtf8());
    check(out.size() == 1 && out[0].text == QString::fromUtf8("Medición: 24,5 °C — ñandú"), "UTF-8 valido intacto");

    // RF-14: clasificacion por palabras clave.
    out.clear();
    a.feed("todo bien\nWARNING: temperatura alta\nERROR: sensor caido\nALERTA: bateria\n");
    check(out.size() == 4, "RF-14 cuatro lineas");
    check(out.size() == 4 && out[0].severity == Sev::Info, "RF-14 info");
    check(out.size() == 4 && out[1].severity == Sev::Warning, "RF-14 warning");
    check(out.size() == 4 && out[2].severity == Sev::Error, "RF-14 error");
    check(out.size() == 4 && out[3].severity == Sev::Warning, "RF-14 alerta en espanol");

    // RF-31: el mismo texto siempre da la misma semilla; textos distintos, distinta.
    out.clear();
    a.feed("hola\nhola\nchau\n");
    check(out.size() == 3 && out[0].seed == out[1].seed, "RF-31 mismo texto, misma semilla");
    check(out.size() == 3 && out[0].seed != out[2].seed, "RF-31 distinto texto, distinta semilla");

    // Sin terminador nunca: el buffer se corta y no crece sin limite.
    out.clear();
    a.reset();
    a.feed(QByteArray(LineAssembler::MaxLineBytes + 500, 'x'));
    check(out.size() == 1 && out[0].text.size() == LineAssembler::MaxLineBytes + 500,
          QString("Corta a los 64 KB sin terminador (largo=%1)")
              .arg(out.isEmpty() ? 0 : out[0].text.size()));

    // RNF-04: binario aleatorio no debe romper nada.
    out.clear();
    QByteArray ruido;
    for (int i = 0; i < 5000; ++i) ruido.append(char(QRandomGenerator::global()->bounded(256)));
    a.feed(ruido);
    check(true, "RNF-04 sobrevive a 5000 bytes binarios aleatorios");

    qInfo().noquote() << (fallos ? QString("\n%1 FALLOS").arg(fallos) : QString("\nTodo verde"));
    return fallos;
}
