pragma Singleton

import QtQuick

QtObject {
    readonly property color background: "#0d1117"
    readonly property color surface: "#161b22"
    readonly property color surfaceHigh: "#1f262e"
    readonly property color border: "#2d3540"
    readonly property color textPrimary: "#e6edf3"
    readonly property color textDim: "#8b949e"
    readonly property color accent: "#4ea1ff"
    readonly property color danger: "#ff7b72"

    readonly property color info: "#79c0ff"
    readonly property color warning: "#e3b341"
    readonly property color error: "#ff7b72"

    readonly property int gap: 10
    readonly property int radius: 6
    readonly property string mono: "monospace"

    function severityColor(severity) {
        switch (severity) {
        case 2: return error
        case 1: return warning
        default: return info
        }
    }

    // RF-21: severity must be readable without relying on colour alone, so
    // every line also carries a glyph and a word.
    function severityGlyph(severity) {
        switch (severity) {
        case 2: return "✖"
        case 1: return "▲"
        default: return "●"
        }
    }

    function severityLabel(severity) {
        switch (severity) {
        case 2: return "ERROR"
        case 1: return "AVISO"
        default: return "INFO "
        }
    }

    // RF-31: hue comes from the message hash, energy from the severity, so the
    // same text always paints the same colour and errors always burn brighter.
    function seedColor(seed, severity, alpha) {
        var hue = (seed % 360) / 360
        var saturation = severity === 2 ? 0.95 : severity === 1 ? 0.80 : 0.65
        var lightness = severity === 2 ? 0.62 : 0.58
        return Qt.hsla(hue, saturation, lightness, alpha === undefined ? 1.0 : alpha)
    }
}
