# Visor Serial ESP32 — Requerimientos y Requisitos

Documento de diseño previo al desarrollo.
Estado: **implementado, pendiente de prueba con hardware** · Fecha: 2026-09-03

---

## 1. Contexto y alcance

Un ESP32 conectado por USB emite mensajes de texto libre, una línea por mensaje,
terminadas en salto de línea. Se necesita una aplicación de escritorio que reciba
esas líneas y las presente en pantalla de forma legible **y** visualmente atractiva.

**Dentro del alcance:** recepción, decodificación, historial y presentación visual.
**Fuera del alcance:** el firmware del ESP32, y enviar datos hacia el ESP32 (la
comunicación es unidireccional: ESP32 → PC).

---

## 2. Requerimientos (necesidades del usuario)

Qué necesita lograr el usuario, sin comprometerse todavía con una solución técnica.

| ID | Necesidad |
|----|-----------|
| R-01 | Conectarme al ESP32 sin tener que averiguar a mano el nombre del puerto. |
| R-02 | Ver los mensajes apenas el ESP32 los emite, sin retardo perceptible. |
| R-03 | Poder revisar lo que llegó antes, no solo el último mensaje. |
| R-04 | Que la lectura sea agradable: no un volcado de texto plano y monótono. |
| R-05 | Que un mensaje importante se distinga a simple vista del ruido de fondo. |
| R-06 | Que si desenchufo y vuelvo a enchufar el ESP32, la app se recupere sola. |
| R-07 | Guardar una sesión para revisarla o entregarla después. |
| R-08 | Entender por qué no llegan datos cuando algo falla (puerto ocupado, permisos, cable). |

---

## 3. Requisitos funcionales

### 3.1 Conexión serial

| ID | Requisito | Criterio de aceptación |
|----|-----------|------------------------|
| RF-01 | Listar los puertos serie disponibles con su descripción y fabricante. | Con un ESP32 conectado, aparece en la lista identificado (ej. `/dev/ttyUSB0 — CP2102 USB to UART`). |
| RF-02 | Permitir elegir velocidad (baudios) de una lista con 115200 por defecto. | La lista incluye al menos 9600, 57600, 115200, 230400, 921600. |
| RF-03 | Abrir y cerrar el puerto desde la interfaz. | Un solo control alterna Conectar/Desconectar y refleja el estado real. |
| RF-04 | Refrescar la lista de puertos al detectar cambios de hardware. | Al conectar el ESP32 con la app abierta, el puerto aparece sin reiniciarla. |
| RF-05 | Reconectar automáticamente al puerto anterior si se pierde la conexión. | Tras desenchufar y volver a enchufar, la recepción se reanuda sin intervención. |
| RF-06 | Reportar los errores de apertura con causa entendible. | Distingue explícitamente: puerto inexistente, permiso denegado y puerto ocupado por otro proceso. |
| RF-07 | No alterar DTR/RTS al abrir el puerto, salvo que el usuario lo pida. | Al conectar, el ESP32 **no** se reinicia. Existe un botón separado "Reiniciar ESP32" que sí pulsa DTR/RTS. |

### 3.2 Procesamiento de las líneas

| ID | Requisito | Criterio de aceptación |
|----|-----------|------------------------|
| RF-10 | Ensamblar los bytes recibidos en líneas completas, cortando en `\n`. | Un mensaje partido en dos lecturas del puerto se muestra como una sola línea. |
| RF-11 | Tolerar terminadores `\n`, `\r\n` y `\r`. | Ninguna variante produce líneas vacías espurias ni caracteres visibles de control. |
| RF-12 | Decodificar como UTF-8 y sustituir los bytes inválidos sin abortar. | Ruido de arranque del ESP32 (baudios distintos) se muestra como `�` y la app sigue funcionando. |
| RF-13 | Marcar cada línea con la hora local de recepción, con milisegundos. | Cada entrada muestra `HH:MM:SS.mmm`. |
| RF-14 | Clasificar cada línea por severidad según palabras clave configurables. | `ERROR: fallo` se clasifica como error; `WARN`/`WARNING` como advertencia; el resto, informativo. |
| RF-15 | Descartar líneas antiguas al superar un límite de historial. | Con límite de 5000, la línea 5001 desplaza a la primera y la memoria no crece indefinidamente. |

### 3.3 Vista consola

| ID | Requisito | Criterio de aceptación |
|----|-----------|------------------------|
| RF-20 | Mostrar el historial como lista de `hora │ severidad │ texto`, monoespaciada. | Las columnas quedan alineadas verticalmente. |
| RF-21 | Colorear cada línea según su severidad. | Error, advertencia e info usan colores distinguibles y distinguibles también para daltonismo rojo-verde (se acompaña de un icono o etiqueta, no solo color). |
| RF-22 | Autoscroll al final, con pausa automática al subir manualmente. | Al desplazarse hacia arriba el autoscroll se detiene; un botón "Ir al final" lo reanuda. |
| RF-23 | Filtrar el historial por texto y por severidad. | El filtro se aplica sobre el historial completo, incluidas líneas ya recibidas. |
| RF-24 | Limpiar el historial en pantalla. | Requiere confirmación si hay más de 100 líneas. |

### 3.4 Vista generativa

| ID | Requisito | Criterio de aceptación |
|----|-----------|------------------------|
| RF-30 | Al llegar una línea, emitir una animación en un lienzo, derivada del contenido del mensaje. | Dos mensajes con texto distinto producen visuales distinguibles entre sí. |
| RF-31 | Derivar el color de la visual del hash del texto, y su intensidad de la severidad. | El mismo mensaje repetido produce siempre el mismo color; un error produce una visual notoriamente más intensa. |
| RF-32 | Mostrar el texto del mensaje más reciente, grande y legible, sobre el lienzo. | El texto es legible sobre cualquier fondo generado (contraste garantizado) y cada mensaje se sostiene 500 ms antes de que otro lo reemplace, así dos líneas seguidas se leen las dos. |
| RF-33 | Desvanecer las visuales antiguas para que la pantalla no sature. | Tras 10 s sin mensajes nuevos el lienzo queda esencialmente limpio. |
| RF-34 | Alternar entre vista consola y vista generativa sin perder la conexión ni el historial. | Al volver a la consola, están todas las líneas recibidas mientras se veía la otra vista. |

### 3.5 Persistencia

| ID | Requisito | Criterio de aceptación |
|----|-----------|------------------------|
| RF-40 | Exportar el historial a un archivo de texto con marcas de tiempo. | El archivo exportado se abre en cualquier editor y conserva el orden de llegada. |
| RF-41 | Recordar puerto, baudios y vista elegida entre ejecuciones. | Al reabrir la app, los últimos ajustes están preseleccionados. |

---

## 4. Requisitos no funcionales

| ID | Requisito | Criterio de aceptación |
|----|-----------|------------------------|
| RNF-01 | **Latencia**: una línea recibida se muestra en menos de 100 ms. | Medido con marcas de tiempo entre la llegada al puerto y el repintado. |
| RNF-02 | **Caudal**: soportar 100 líneas/s sostenidas sin perder mensajes ni congelar la UI. | La interfaz sigue respondiendo a clics durante una ráfaga de 30 s. |
| RNF-03 | **Fluidez**: la vista generativa mantiene 60 fps en hardware integrado modesto. | Sin caídas visibles durante una ráfaga de mensajes. |
| RNF-04 | **Robustez**: ningún dato del ESP32 puede provocar un cierre inesperado. | Enviar binario aleatorio, líneas de 1 MB y bytes UTF-8 inválidos no cierra la app. |
| RNF-05 | **Aislamiento**: la lectura serial no bloquea el hilo de interfaz. | Con el ESP32 saturando el puerto, la ventana se puede mover y redimensionar con normalidad. |
| RNF-06 | **Portabilidad**: compila y corre en Linux y Windows sin cambios de código. | Solo se usan APIs de Qt; nada específico de plataforma. |
| RNF-07 | **Construcción reproducible**: se compila con un comando estándar. | `cmake -B build && cmake --build build` funciona en un clon limpio. |
| RNF-08 | **Mantenibilidad**: la lógica de comunicación no depende de la interfaz. | Las clases de serial y modelo compilan y se prueban sin instanciar la UI. |

---

## 5. Decisiones de arquitectura

**Qt Quick (QML) para la interfaz, C++ para la lógica.**
La vista generativa (RF-30 a RF-33) exige animaciones continuas y compuestas;
en QML se declaran en pocas líneas y se ejecutan sobre la GPU, mientras que en
Widgets habría que programarlas a mano con `QPainter` y temporizadores. La
consola (RF-20 a RF-24) se resuelve igual de bien en ambos, así que la vista
generativa decide.

**Separación en tres capas** (respalda RNF-05 y RNF-08):

```
  ESP32 ──USB──> [ SerialLink ]      C++   abre el puerto, emite bytes crudos
                       │                   señal: bytesRecibidos(QByteArray)
                       v
                 [ LineAssembler ]   C++   arma líneas, decodifica, clasifica
                       │                   señal: lineaNueva(Linea)
                       v
                 [ LogModel ]        C++   QAbstractListModel + ring buffer
                       │                   expuesto a QML como modelo
                       v
                 [ QML ]             UI    ConsolaView / GenerativaView
```

`LineAssembler` y `LogModel` no conocen a Qt Quick: reciben y emiten datos, nada
más. Eso permite probarlos sin abrir una ventana (RNF-08).

**Nota sobre hilos:** `QSerialPort` es asíncrono por señales; con un caudal de
100 líneas/s (RNF-02) no hace falta un hilo aparte. Si al medir aparece bloqueo,
mover `SerialLink` a un `QThread` es un cambio localizado, porque ya está aislado.

---

## 6. Fuera de alcance

- Enviar comandos hacia el ESP32.
- Parseo de datos estructurados, gráficas de sensores o dashboards.
- Flasheo o monitor de arranque del ESP32.
- Múltiples puertos simultáneos.
- Empaquetado e instalador.

---

## 7. Riesgos técnicos identificados

| Riesgo | Impacto | Mitigación |
|--------|---------|------------|
| **Permisos del puerto en Linux.** En Arch, `/dev/ttyUSB*` pertenece al grupo `uucp` (no `dialout` como en Debian/Ubuntu). Sin pertenecer al grupo, la apertura falla. | La app parece rota sin serlo. | RF-06: detectar `PermissionError` y mostrar el comando exacto: `sudo usermod -aG uucp $USER` (requiere volver a iniciar sesión). |
| **Reinicio involuntario del ESP32.** Los adaptadores CP2102/CH340 usan DTR/RTS como línea de reset; abrir el puerto puede reiniciar la placa y perder los primeros mensajes. | Se pierden datos al conectar. | RF-07: no tocar DTR/RTS al abrir; ofrecer el reinicio como acción explícita. |
| **Basura al arrancar el ESP32.** El bootloader emite a 74880 baudios; a 115200 se lee como bytes inválidos. | Caracteres ilegibles en pantalla. | RF-12: reemplazo de bytes inválidos, nunca descarte de la conexión. |
| **Ráfagas más rápidas que el repintado.** Un `printf` en bucle puede generar miles de líneas/s. | UI congelada. | RF-15 (ring buffer) + agrupar las inserciones al modelo por lotes de frame. |
| **Línea sin terminador.** Un ESP32 que nunca emite `\n` haría crecer el buffer sin límite. | Consumo de memoria ilimitado. | Cortar y emitir la línea al superar un tamaño máximo (ej. 64 KB). |

---

## 8. Estado de implementación

Todas las etapas del plan están construidas. `cmake --build` termina con cero
errores y cero advertencias.

| Grupo | Estado | Cómo se verificó |
|-------|--------|------------------|
| RF-01 a RF-07 (conexión) | Implementado y probado con hardware | ESP32 con CH340 en `/dev/ttyUSB0`, 2026-09-03: RF-01, RF-02, RF-03, RF-06 (ocupado) y RF-07 verificados en vivo. Faltan RF-04, RF-05 y RF-06 (puerto inexistente), que piden desenchufar la placa. |
| RF-10 a RF-15 (líneas) | Implementado y probado | `ctest`: 16 comprobaciones automáticas, todas en verde. |
| RF-20 a RF-24 (consola) | Implementado | Ejecutado con mensajes sintéticos y verificado por captura. |
| RF-30 a RF-34 (generativa) | Implementado y probado con hardware | Verificado por captura, con el ESP32 en vivo el 2026-09-03: se alternan las dos líneas del sketch. |
| RF-40, RF-41 (persistencia) | Implementado | Revisión de código; falta ejercitar el diálogo de exportación a mano. |
| RNF-01, RNF-02 (latencia, caudal) | Implementado | Ráfagas de 200 líneas sin pérdidas ni bloqueo de la interfaz. |
| RNF-04 (robustez) | Probado | Bytes UTF-8 inválidos, líneas de 64 KB sin terminador y 5000 bytes binarios aleatorios: ninguno rompe nada. |
| RNF-07, RNF-08 (build, aislamiento) | Cumplido | El binario de prueba enlaza solo contra `Qt6::Core`: sin interfaz, sin puerto serie. |

### RF-07 contra la placa: el reinicio lo causaba la propia app

Con el ESP32 conectado se vio que la placa **sí** se reiniciaba al pulsar
Conectar: la consola abría con `ets Jul 29 2019` y `rst:0x1 (POWERON_RESET)`.
No era el kernel, como suponía la nota de «mejor esfuerzo», sino el orden de
las dos llamadas de `SerialLink::open()`.

En el circuito de auto-reinicio de estas placas EN baja cuando **RTS está alto
y DTR bajo**; con las dos líneas al mismo nivel no pasa nada. El kernel levanta
DTR y RTS al abrir el nodo, o sea que llega en un estado seguro. La app bajaba
primero DTR, y ese instante de DTR bajo con RTS todavía alto es exactamente el
pulso que `pulseReset()` manda a propósito.

Invertir las dos líneas —soltar RTS y después DTR— lo resuelve: entre medio
queda DTR alto con RTS bajo, que solo toca IO0 y al firmware ya arrancado le da
igual. Verificado el 2026-09-03: al conectar no aparece el encabezado de
arranque y la cadencia de `millis()` del sketch sigue corrida (`:42.148`,
`:43.148`, `:44.148`), o sea que la placa nunca se reinició. El botón
«Reiniciar ESP32» sí la reinicia, que es lo que pide RF-07.

### El selector de puertos se corría solo al desenchufar

Probando en vivo apareció un fallo peor que el de RF-07. Al desenchufar la
placa, `ttyUSB0` se cae de la lista, el `ComboBox` reconstruye su modelo y el
índice vuelve solo a 0: el primer puerto, que en esta máquina es `ttyS4`, un
puerto serie de la placa madre. Presionar Conectar entonces abre `ttyS4` — que
abre perfecto y no dice nunca nada — y hasta lo guarda en las preferencias. La
app queda «conectada» y muda, y no hay nada en la interfaz que explique por qué.

Arreglado en `qml/ConnectionBar.qml`:

- Si el puerto elegido no está en la lista, el selector **no** se corre a otro:
  queda sin índice y muestra «ttyUSB0 (desconectado)», o sea el puerto que está
  esperando, por nombre.
- Conectar en ese estado reintenta ese mismo puerto y falla con «El puerto
  ttyUSB0 no existe. ¿Está enchufado el ESP32?», que es la respuesta honesta
  (RF-06) en lugar de una conexión silenciosa al puerto equivocado.
- Sin nada recordado (primera ejecución) prefiere un `ttyUSB*`/`ttyACM*` antes
  que un `ttyS*` de la placa madre: un ESP32 siempre es uno de los primeros.

### Dos líneas seguidas y una sola pantalla

El sketch de prueba imprime `[Task A]` y `[Task B]` con unos 3 ms de diferencia
y después calla un segundo. La vista generativa reemplazaba el titular apenas
llegaba la línea nueva, así que `[Task A]` se iba antes de terminar su
animación de entrada: en pantalla parecía que solo llegaba `[Task B]`.

Ahora cada titular tiene un piso de 500 ms en pantalla (`headlineHoldMs`) y la
línea que llega mientras tanto espera. Se guarda **una** sola pendiente: si
entran veinte en esa ventana se muestra la última, que es justo lo que esta
vista tiene permitido descartar (la consola las conserva todas).

### Lo que todavía NO está verificado

Nada de esto está roto necesariamente; simplemente no lo pude comprobar y no
corresponde darlo por bueno:

- **Lo que necesita desenchufar la placa**: que el puerto aparezca solo al
  conectarla con la app abierta (RF-04), la reconexión tras desenchufar y volver
  a enchufar (RF-05) y el mensaje de «puerto inexistente» (RF-06). El resto del
  grupo ya se probó contra la placa: ver más abajo.
- **RNF-03 (60 fps)**: la vista generativa solo se ejecutó con el renderizador
  por software, que no representa el rendimiento real sobre GPU.
- **RNF-06 (Windows)**: solo se compiló en Linux. No se usa ninguna API
  específica de plataforma, pero eso es un argumento, no una prueba.

### Un hallazgo del proceso

La primera ejecución con datos terminaba en *segmentation fault*. El culpable
resultó ser el andamiaje de `--demo`, no el programa: una lambda capturaba por
referencia un `int` declarado dentro de un bloque `if`, y lo leía después de que
ese bloque terminara. AddressSanitizer lo señaló con archivo y línea exactos en
un solo intento, después de que el backtrace normal apuntara a tres lugares
distintos en tres ejecuciones. Vale la pena recordar el comando:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -g" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address"
```

---

## 9. Siguientes pasos

1. Desenchufar y volver a enchufar la placa con la app abierta para cerrar
   RF-04, RF-05 y el mensaje de puerto inexistente de RF-06.
2. Definir la estética final de la vista generativa con la placa en vivo:
   los tiempos de animación están puestos a ojo y el ritmo real de mensajes es
   lo único que dice si están bien.
3. Si aparece bloqueo de la interfaz con caudales altos, mover `SerialLink` a
   un `QThread`; la separación en capas ya deja ese cambio contenido.
