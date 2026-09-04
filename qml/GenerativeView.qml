import QtQuick
import QtQuick.Particles

// Three stacked layers, each with one job (RF-30 to RF-34):
//   1. ambient particles whose density tracks how busy the link is
//   2. a burst per message, plus an expanding ring when it is not routine
//   3. the latest message itself, large and always readable
Item {
    id: root
    clip: true

    // How busy the link feels right now, 0 to 1. Every message pushes it up,
    // the decay timer pulls it down, and everything visual reads from it.
    property real activity: 0
    property color tint: Theme.info
    property int activeEffects: 0

    // RNF-03: a hard ceiling on live effects is what keeps a 100 line/s burst
    // from turning into thousands of animating items. The console keeps every
    // line; this view is allowed to skip the ones it cannot draw in time.
    readonly property int maxEffects: 24

    // How long a headline is guaranteed to stay up. Firmware tends to print
    // related lines back to back -- a sketch alternating two tasks emits both
    // within a few milliseconds -- and without this floor the first one is
    // replaced mid entry animation and never actually gets read.
    readonly property int headlineHoldMs: 500

    // Newest arrival waiting for the floor to expire, or null. Only one is
    // kept: under a real burst the in-between lines are exactly the ones this
    // view is allowed to skip, and the console still has them all.
    property var pendingHeadline: null

    function showHeadline(text, frameColor) {
        fadeAnimation.stop()
        headlineLabel.text = text
        headlineFrame.border.color = frameColor
        entryAnimation.restart()
        idleTimer.restart()
        headlineHold.restart()
    }

    function present(text, severity, seed) {
        var lineColor = Theme.seedColor(seed, severity)
        tint = lineColor
        activity = Math.min(1.0, activity + (severity === 2 ? 0.50 : severity === 1 ? 0.35 : 0.20))

        // Layer 3: show it now, or queue it behind the line still on screen.
        if (headlineHold.running)
            pendingHeadline = {"text": text, "frameColor": lineColor}
        else
            showHeadline(text, lineColor)

        if (activeEffects >= maxEffects)
            return

        // Everything about the burst is derived from the hash, so the same
        // text always draws the same shape and two texts never look alike.
        var pulse = pulseComponent.createObject(effectLayer, {
            // >>> and not >>: the seed is a full 32-bit value, and a signed
            // shift turns anything past 2^31 negative, which threw the burst
            // off the left edge of the canvas.
            "x": width * (0.2 + ((seed >>> 16) % 1000) / 1000 * 0.6),
            "y": height * (0.2 + ((seed >>> 4) % 1000) / 1000 * 0.6),
            "tint": tint,
            "dots": 8 + (seed % 9),
            "jitter": ((seed >>> 8) % 360) * Math.PI / 180,
            "energy": severity === 2 ? 1.0 : severity === 1 ? 0.7 : 0.45,
            "ring": severity > 0
        })
        if (pulse)
            activeEffects++
    }

    Timer {
        id: headlineHold
        interval: root.headlineHoldMs
        onTriggered: {
            if (root.pendingHeadline) {
                var next = root.pendingHeadline
                root.pendingHeadline = null
                root.showHeadline(next.text, next.frameColor)
            }
        }
    }

    // RF-33: with nothing arriving, activity bleeds away and every layer
    // thins out with it. After ~10 s the canvas is bare.
    Timer {
        interval: 100
        running: true
        repeat: true
        onTriggered: root.activity = Math.max(0, root.activity * 0.965 - 0.002)
    }

    Connections {
        target: logModel
        function onLineAppended(text, severity, seed) { root.present(text, severity, seed) }
    }

    // ------------------------------------------------- layer 0: background
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Qt.hsla(root.tint.hslHue, 0.45, 0.06 + 0.09 * root.activity, 1.0)
                Behavior on color { ColorAnimation { duration: 700 } }
            }
            GradientStop { position: 1.0; color: Theme.background }
        }
    }

    // ------------------------------------------ layer 1: ambient particles
    ParticleSystem {
        id: ambient
        anchors.fill: parent
    }

    ImageParticle {
        system: ambient
        color: root.tint
        colorVariation: 0.55
        alpha: 0.45
        entryEffect: ImageParticle.Fade
    }

    Emitter {
        system: ambient
        anchors.fill: parent
        emitRate: 2 + 28 * root.activity
        lifeSpan: 4200
        lifeSpanVariation: 1600
        size: 7
        sizeVariation: 5
        endSize: 1
        velocity: AngleDirection {
            angle: 270
            angleVariation: 45
            magnitude: 14
            magnitudeVariation: 12
        }
    }

    // ---------------------------------------------- layer 2: message bursts
    Item {
        id: effectLayer
        anchors.fill: parent
    }

    Component {
        id: pulseComponent

        Item {
            id: pulse

            property color tint: "white"
            property real energy: 0.5
            property int dots: 10
            property real jitter: 0
            property bool ring: false

            Component.onDestruction: root.activeEffects--

            Timer {
                interval: 1500
                running: true
                onTriggered: pulse.destroy()
            }

            // The ring is reserved for warnings and errors, so an unusual
            // message is recognisable from across the room (R-05).
            Rectangle {
                id: ringShape
                anchors.centerIn: parent
                width: 0
                height: width
                radius: width / 2
                color: "transparent"
                border.color: pulse.tint
                border.width: 2
                opacity: 0
                visible: pulse.ring

                ParallelAnimation {
                    running: pulse.ring
                    NumberAnimation {
                        target: ringShape; property: "width"
                        from: 0; to: 180 + 260 * pulse.energy
                        duration: 1300; easing.type: Easing.OutQuad
                    }
                    SequentialAnimation {
                        NumberAnimation { target: ringShape; property: "opacity"; to: 0.85; duration: 120 }
                        NumberAnimation { target: ringShape; property: "opacity"; to: 0; duration: 1180 }
                    }
                }
            }

            Repeater {
                model: pulse.dots

                delegate: Rectangle {
                    id: dot
                    required property int index

                    readonly property real angle: pulse.jitter + index * 2 * Math.PI / pulse.dots
                    readonly property real distance: (70 + 130 * pulse.energy) * (0.65 + (index % 5) * 0.14)

                    width: 4 + 5 * pulse.energy
                    height: width
                    radius: width / 2
                    color: pulse.tint
                    x: -width / 2
                    y: -height / 2
                    opacity: 0

                    ParallelAnimation {
                        running: true
                        NumberAnimation {
                            target: dot; property: "x"
                            to: Math.cos(dot.angle) * dot.distance - dot.width / 2
                            duration: 1100; easing.type: Easing.OutCubic
                        }
                        NumberAnimation {
                            target: dot; property: "y"
                            to: Math.sin(dot.angle) * dot.distance - dot.height / 2
                            duration: 1100; easing.type: Easing.OutCubic
                        }
                        SequentialAnimation {
                            NumberAnimation { target: dot; property: "opacity"; to: 0.9; duration: 90 }
                            NumberAnimation { target: dot; property: "opacity"; to: 0; duration: 1000 }
                        }
                    }
                }
            }
        }
    }

    // ------------------------------------------------ layer 3: the message
    Item {
        id: headline
        anchors.centerIn: parent
        width: parent.width * 0.82
        height: headlineFrame.height
        opacity: 0
        z: 10

        // RF-32: a solid backdrop is what guarantees the text stays legible no
        // matter what colour the layers underneath happen to be.
        Rectangle {
            id: headlineFrame
            anchors.centerIn: parent
            width: parent.width
            height: headlineLabel.implicitHeight + 44
            radius: 12
            color: Qt.rgba(0, 0, 0, 0.62)
            border.width: 2
            border.color: Theme.info
        }

        Text {
            id: headlineLabel
            anchors.centerIn: headlineFrame
            width: headlineFrame.width - 48
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            maximumLineCount: 3
            elide: Text.ElideRight
            font.pixelSize: 40
            font.bold: true
            color: "white"
        }
    }

    SequentialAnimation {
        id: entryAnimation
        PropertyAction { target: headline; property: "opacity"; value: 0 }
        PropertyAction { target: headline; property: "scale"; value: 1.16 }
        ParallelAnimation {
            NumberAnimation { target: headline; property: "opacity"; to: 1.0; duration: 200 }
            NumberAnimation {
                target: headline; property: "scale"; to: 1.0
                duration: 450; easing.type: Easing.OutBack
            }
        }
    }

    NumberAnimation {
        id: fadeAnimation
        target: headline
        property: "opacity"
        to: 0
        duration: 1400
    }

    Timer {
        id: idleTimer
        interval: 8000
        onTriggered: fadeAnimation.start()
    }

    Text {
        anchors.centerIn: parent
        visible: logModel.count === 0
        color: Theme.textDim
        font.pixelSize: 18
        text: serial.connected ? qsTr("Esperando mensajes del ESP32…")
                               : qsTr("Conectá el ESP32 para ver los mensajes acá.")
    }
}
