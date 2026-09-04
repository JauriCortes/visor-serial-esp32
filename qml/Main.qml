import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: window

    width: 1150
    height: 720
    minimumWidth: 720
    minimumHeight: 420
    visible: true
    title: qsTr("Visor Serial ESP32")
    color: Theme.background

    // Attached here so every stock control inherits the dark palette.
    Material.theme: Material.Dark
    Material.background: Theme.surface
    Material.foreground: Theme.textPrimary
    Material.accent: Theme.accent

    // RF-41: remembered across runs, no C++ needed.
    Settings {
        id: prefs
        property string port: ""
        property int baud: 115200
        property int view: 0
        property int capacity: 5000
    }

    Component.onCompleted: logModel.capacity = prefs.capacity

    header: ConnectionBar {
        id: connectionBar
        Component.onCompleted: {
            rememberedPort = prefs.port
            rememberedBaud = prefs.baud
        }
        onRememberedPortChanged: prefs.port = rememberedPort
        onRememberedBaudChanged: prefs.baud = rememberedBaud
    }

    footer: StatusBar {}

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabs
            Layout.fillWidth: true
            currentIndex: prefs.view
            onCurrentIndexChanged: prefs.view = currentIndex

            TabButton { text: qsTr("Consola") }
            TabButton { text: qsTr("Visual") }
        }

        // RF-34: a StackLayout keeps both views instantiated, so switching
        // costs nothing and neither the history nor the link is disturbed.
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            ConsoleView {}
            GenerativeView {}
        }
    }
}
