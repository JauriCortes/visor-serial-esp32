import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Port selection, connection control and status readout (RF-01 to RF-07).
Rectangle {
    id: bar

    // Persisted between runs (RF-41).
    property string rememberedPort: ""
    property int rememberedBaud: 115200

    implicitHeight: layout.implicitHeight + 2 * Theme.gap
    color: Theme.surface
    border.color: Theme.border
    border.width: 1

    readonly property var baudRates: [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600]

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Theme.gap
        spacing: Theme.gap

        Label {
            text: qsTr("Puerto")
            color: Theme.textDim
        }

        ComboBox {
            id: portBox
            // Flexible, so the bar survives a narrow window instead of
            // pushing the buttons past the right edge.
            Layout.fillWidth: true
            Layout.minimumWidth: 150
            Layout.preferredWidth: 300
            enabled: !serial.connected
            model: serial.ports
            textRole: "label"
            valueRole: "name"
            // With no index selected there is still something to say: the port
            // we are waiting for, by name.
            displayText: currentIndex >= 0 ? currentText
                       : bar.rememberedPort !== "" ? qsTr("%1 (desconectado)").arg(bar.rememberedPort)
                       : qsTr("Sin puertos detectados")

            function indexOfPort(name) {
                for (var i = 0; i < count; ++i)
                    if (serial.ports[i].name === name)
                        return i
                return -1
            }

            // RF-04: the list is rebuilt every second, so re-select the same
            // device by name instead of letting the index snap back to 0.
            function restoreSelection() {
                var found = indexOfPort(bar.rememberedPort)
                if (found >= 0) {
                    currentIndex = found
                    return
                }

                // The chosen port is not there right now -- the board is
                // unplugged. Keep waiting for it by name instead of sliding
                // onto whatever else is in the list: index 0 is typically a
                // motherboard ttyS*, which opens fine and then says nothing,
                // so the app looks broken while it is faithfully reading an
                // empty port.
                if (bar.rememberedPort !== "") {
                    currentIndex = -1
                    return
                }

                // Nothing remembered (first run): a USB adapter is a far better
                // guess than a built-in ttyS*.
                for (var i = 0; i < count; ++i) {
                    if (/^tty(USB|ACM)/.test(serial.ports[i].name)) {
                        currentIndex = i
                        return
                    }
                }
                if (count > 0 && currentIndex < 0)
                    currentIndex = 0
            }

            onModelChanged: restoreSelection()
            // The remembered port arrives after this ComboBox is built.
            Connections {
                target: bar
                function onRememberedPortChanged() { portBox.restoreSelection() }
            }
            onActivated: bar.rememberedPort = currentValue
            Component.onCompleted: restoreSelection()
        }

        Label {
            text: qsTr("Baudios")
            color: Theme.textDim
        }

        ComboBox {
            id: baudBox
            Layout.preferredWidth: 110
            enabled: !serial.connected
            model: bar.baudRates
            currentIndex: Math.max(0, bar.baudRates.indexOf(bar.rememberedBaud))
            onActivated: bar.rememberedBaud = bar.baudRates[currentIndex]
        }

        Button {
            id: connectButton
            text: serial.connected ? qsTr("Desconectar") : qsTr("Conectar")
            enabled: serial.connected || portBox.count > 0 || bar.rememberedPort !== ""
            onClicked: {
                if (serial.connected) {
                    serial.close()
                    return
                }
                // currentValue is undefined while the awaited port is missing;
                // trying it anyway is what produces the "no existe" message,
                // which is the honest answer (RF-06).
                var chosen = portBox.currentIndex >= 0 ? portBox.currentValue
                                                       : bar.rememberedPort
                if (!chosen)
                    return
                bar.rememberedPort = chosen
                bar.rememberedBaud = bar.baudRates[baudBox.currentIndex]
                serial.open(chosen, bar.rememberedBaud)
            }
        }

        // RF-07: resetting the board is a deliberate action, never a side
        // effect of opening the port.
        Button {
            text: qsTr("Reiniciar ESP32")
            enabled: serial.connected
            onClicked: serial.pulseReset()
        }

    }
}
