import QtQuick
import QtQuick.Controls.Material

ApplicationWindow {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Modding Lexicon")

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

    FontLoader { // Using Inter font from Google
        id: rcFont
        source: "qrc:/fonts/InterVariable.ttf"
    }

    TabBar { // Navigation bar at top
        id: bar
        width: parent.width
        height: parent.height * 0.15
        contentHeight: parent.height * 0.15

        background: Rectangle {
            color: "#232323"
        }

        font.family: rcFont.font.family
        font.pixelSize: 20
        font.bold: true
        TabButton {
            id: myAddonsTab
            text: qsTr("My Addons")
        }
        TabButton {
            id: browseAddonsTab
            text: qsTr("Browse Addons")
        }
        TabButton {
            id: settingsTab
            text: qsTr("Settings")
        }
    }
}
