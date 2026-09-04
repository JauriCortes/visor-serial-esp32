#!/usr/bin/env python3
"""Fake ESP32 on a virtual serial port, for testing the viewer without hardware.

    socat -d -d PTY,raw,echo=0,link=/tmp/esp32sim PTY,raw,echo=0,link=/tmp/esp32app
    ./tools/simulador.py /tmp/esp32sim

Then connect the viewer to /tmp/esp32app. Baud rate is irrelevant on a pty.
"""

import random
import sys
import time

MENSAJES = [
    (b"Sistema iniciado", 0),
    (b"WiFi conectado, IP 192.168.1.42", 0),
    (b"Lectura sensor: 24.5 C", 0),
    (b"Boton presionado", 0),
    (b"WARNING: temperatura alta", 1),
    (b"ALERTA: bateria baja", 1),
    (b"ERROR: no responde el sensor I2C", 2),
    (b"Acentos y enies: canion, medicion, nino", 0),
]


def escribir(puerto, datos):
    puerto.write(datos)
    puerto.flush()


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    ruta = sys.argv[1]

    with open(ruta, "wb", buffering=0) as puerto:
        # Bytes invalidos: imita el ruido a 74880 baudios del bootloader (RF-12).
        escribir(puerto, bytes([0xC3, 0x28, 0xA0, 0xFF, 0xFE]) + b" arranque\n")
        time.sleep(0.4)

        # Una linea partida en dos escrituras (RF-10).
        escribir(puerto, b"Mensaje partido en ")
        time.sleep(0.3)
        escribir(puerto, b"dos pedazos\n")

        # Los tres terminadores (RF-11).
        escribir(puerto, b"Terminada en LF\n")
        escribir(puerto, b"Terminada en CRLF\r\n")
        escribir(puerto, b"Terminada en CR solo\r")

        # UTF-8 real de verdad.
        escribir(puerto, "Medición: 24,5 °C — ñandú\n".encode("utf-8"))

        ciclo = 0
        while True:
            ciclo += 1

            # Cada 20 ciclos, una rafaga de 200 lineas para medir RNF-02.
            if ciclo % 20 == 0:
                print("rafaga de 200 lineas", flush=True)
                for i in range(200):
                    escribir(puerto, b"Rafaga linea %d\n" % i)
                time.sleep(1.0)
                continue

            texto, _ = random.choice(MENSAJES)
            escribir(puerto, texto + b"\n")
            time.sleep(random.uniform(0.3, 1.2))

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        pass
