import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

// Scrollable, filterable history (RF-20 to RF-24, RF-40).
Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ------------------------------------------------------- filter bar
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: filterRow.implicitHeight + 2 * Theme.gap
            color: Theme.surface
            border.color: Theme.border
            border.width: 1

            RowLayout {
                id: filterRow
                anchors.fill: parent
                anchors.margins: Theme.gap
                spacing: Theme.gap

                TextField {
                    Layout.fillWidth: true
                    placeholderText: qsTr("Filtrar por texto…")
                    onTextChanged: logFilter.query = text
                }

                ComboBox {
                    Layout.preferredWidth: 170
                    model: [qsTr("Todos"), qsTr("Avisos y errores"), qsTr("Solo errores")]
                    onActivated: logFilter.minSeverity = currentIndex
                }

                Label {
                    color: Theme.textDim
                    text: logFilter.count === logModel.count
                          ? qsTr("%1 líneas").arg(logModel.count)
                          : qsTr("%1 de %2 líneas").arg(logFilter.count).arg(logModel.count)
                }

                Button {
                    text: qsTr("Exportar…")
                    enabled: logModel.count > 0
                    onClicked: exportDialog.open()
                }

                Button {
                    text: qsTr("Limpiar")
                    enabled: logModel.count > 0
                    // RF-24: only ask when there is something worth losing.
                    onClicked: logModel.count > 100 ? clearDialog.open() : logModel.clear()
                }
            }
        }

        // ---------------------------------------------------------- history
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                anchors.fill: parent
                anchors.margins: Theme.gap
                clip: true
                model: logFilter
                spacing: 2
                reuseItems: true
                cacheBuffer: 400
                boundsBehavior: Flickable.StopAtBounds

                // RF-22: following the tail is the default, but scrolling up by
                // hand must not fight the user.
                property bool following: true

                onMovementStarted: following = false
                onMovementEnded: following = atYEnd
                // Deferred: the new delegate does not exist yet at this point.
                onCountChanged: if (following) Qt.callLater(positionViewAtEnd)

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: Row {
                    id: entry

                    required property string time
                    required property string text
                    required property int severity

                    width: ListView.view.width
                    spacing: Theme.gap

                    Label {
                        id: stamp
                        text: entry.time
                        font.family: Theme.mono
                        color: Theme.textDim
                    }

                    // Colour plus glyph plus word: never colour alone (RF-21).
                    Label {
                        id: badge
                        width: 78
                        text: Theme.severityGlyph(entry.severity) + " " + Theme.severityLabel(entry.severity)
                        font.family: Theme.mono
                        color: Theme.severityColor(entry.severity)
                    }

                    Label {
                        width: entry.width - stamp.width - badge.width - 2 * entry.spacing
                        text: entry.text
                        font.family: Theme.mono
                        color: entry.severity === 0 ? Theme.textPrimary : Theme.severityColor(entry.severity)
                        wrapMode: Text.Wrap
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: logModel.count === 0
                    color: Theme.textDim
                    horizontalAlignment: Text.AlignHCenter
                    text: serial.connected
                          ? qsTr("Conectado. Esperando mensajes del ESP32…")
                          : qsTr("Elegí un puerto y presioná Conectar.")
                }
            }

            Button {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 2 * Theme.gap
                visible: !list.following && list.count > 0
                text: qsTr("↓ Ir al final")
                onClicked: {
                    list.following = true
                    list.positionViewAtEnd()
                }
            }
        }
    }

    // ------------------------------------------------------------- dialogs
    Dialog {
        id: clearDialog
        anchors.centerIn: Overlay.overlay
        modal: true
        title: qsTr("Limpiar historial")
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: logModel.clear()

        Label {
            text: qsTr("Se van a descartar %1 líneas recibidas. ¿Seguro?").arg(logModel.count)
        }
    }

    FileDialog {
        id: exportDialog
        title: qsTr("Guardar historial")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "txt"
        nameFilters: [qsTr("Archivo de texto (*.txt)"), qsTr("Todos los archivos (*)")]
        currentFile: "sesion-esp32.txt"
        onAccepted: {
            if (!logModel.exportToFile(selectedFile)) {
                errorDialog.text = logModel.lastError()
                errorDialog.open()
            }
        }
    }

    Dialog {
        id: errorDialog
        property alias text: errorLabel.text
        anchors.centerIn: Overlay.overlay
        modal: true
        title: qsTr("No se pudo exportar")
        standardButtons: Dialog.Ok

        Label { id: errorLabel; wrapMode: Text.Wrap; width: 400 }
    }
}
