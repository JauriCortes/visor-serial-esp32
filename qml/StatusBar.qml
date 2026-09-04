import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Connection state lives at the bottom of the window so the top bar cannot
// overflow on a narrow window, and so the message has room to be readable.
Rectangle {
    implicitHeight: statusRow.implicitHeight + Theme.gap
    color: Theme.surface
    border.color: Theme.border
    border.width: 1

    RowLayout {
        id: statusRow
        anchors.fill: parent
        anchors.leftMargin: Theme.gap
        anchors.rightMargin: Theme.gap
        spacing: Theme.gap

        Rectangle {
            Layout.preferredWidth: 10
            Layout.preferredHeight: 10
            radius: 5
            color: serial.connected ? "#3fb950"
                                    : serial.statusIsError ? Theme.danger : Theme.textDim

            // Blinking means "trying to get back", which is not the same as
            // being idle or being broken (RF-05).
            SequentialAnimation on opacity {
                running: serial.retrying && !serial.connected
                loops: Animation.Infinite
                NumberAnimation { to: 0.25; duration: 600 }
                NumberAnimation { to: 1.0; duration: 600 }
            }
        }

        Label {
            Layout.fillWidth: true
            text: serial.status
            elide: Text.ElideRight
            color: serial.statusIsError ? Theme.danger : Theme.textDim

            ToolTip.visible: statusHover.hovered && truncated
            ToolTip.text: serial.status
            HoverHandler { id: statusHover }
        }

        Label {
            text: qsTr("%1 líneas").arg(logModel.count)
            color: Theme.textDim
        }
    }
}
