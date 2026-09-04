# Visor Serial ESP32

Aplicación de escritorio en C++/Qt 6 que lee las líneas de texto que un ESP32
envía por el puerto serie y las muestra de dos formas: una consola con
historial y filtros, y una vista generativa donde cada mensaje dispara una
animación derivada de su propio contenido.

El diseño y los criterios de aceptación están en
[docs/01-requerimientos.md](docs/01-requerimientos.md).

## Compilar

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/esp32visor
```

Requiere Qt 6.5 o superior con los módulos Quick, QuickControls2 y SerialPort.
En Arch: `sudo pacman -S qt6-base qt6-declarative qt6-serialport`.

> El proyecto vivió un tiempo en `/mnt/jcortesca`, que es un montaje rclone
> (almacenamiento en la nube). No lo pongas ahí: CMake hace miles de
> operaciones de archivo pequeñas y sobre rclone el `configure` pasa de 1,4
> segundos a más de 6 minutos sin llegar a terminar. Tiene que estar en disco
> local.

## Instalar

Para poder abrirlo por nombre desde cualquier terminal, o desde el lanzador
de escritorio:

```bash
cmake --install build --prefix ~/.local
```

Deja el binario en `~/.local/bin/esp32visor` y la entrada de escritorio en
`~/.local/share/applications/`. Para desinstalarlo, borrá esos dos archivos.

## Permisos del puerto serie

En Arch los nodos `/dev/ttyUSB*` pertenecen al grupo `uucp` (no `dialout`, que
es lo que dicen los tutoriales de Ubuntu). Si la app dice «Permiso denegado»:

```bash
sudo usermod -aG uucp $USER
```

Hay que cerrar sesión y volver a entrar para que el grupo tome efecto.

## Probar sin ESP32

```bash
./build/esp32visor --demo
```

Inyecta mensajes sintéticos en el mismo punto por el que entran los bytes del
puerto: líneas partidas, los tres terminadores, UTF-8 inválido y ráfagas de 200
líneas. Es andamiaje de desarrollo, está aislado en un bloque de `main.cpp` y se
puede borrar sin tocar nada más.

También hay un simulador que habla por un puerto serie virtual, si querés
probar el camino completo incluyendo `QSerialPort`. Necesita `socat`
(`sudo pacman -S socat`, no está instalado):

```bash
socat -d -d PTY,raw,echo=0,link=/tmp/esp32sim PTY,raw,echo=0,link=/tmp/esp32app &
./tools/simulador.py /tmp/esp32sim
```

## Estructura

```
src/SerialLink.*       abre el puerto, emite bytes crudos
src/LineAssembler.*    arma líneas, decodifica UTF-8, clasifica severidad
src/LogModel.*         historial acotado + filtro, expuesto a QML
src/main.cpp           conecta las tres etapas y arranca el motor QML
qml/ConsoleView.qml    historial con filtros, autoscroll y exportación
qml/GenerativeView.qml tres capas: partículas, ondas, tipografía cinética
qml/ConnectionBar.qml  selección de puerto, conexión, estado
```

Ninguna de las clases de `src/` depende de Qt Quick: se pueden compilar y
probar sin abrir una ventana.

## Licencia

GPL-3.0. El texto completo está en [LICENSE](LICENSE).
