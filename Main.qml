import QtQuick
import QtQuick.Controls.Fusion

ApplicationWindow {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Modding Lexicon")

    FontLoader {
        id: rcFont
        source: "qrc:/fonts/InterVariable.ttf"
    }

    TabBar {
        id: tabBar
        width: parent.width
        height: parent.height * 0.15
        contentHeight: parent.height * 0.15
        topPadding: 15

        font.family: rcFont.font.family
        font.pixelSize: 20
        font.bold: true
        TabButton {
            text: qsTr("My Addons")
        }
        TabButton {
            text: qsTr("Browse Addons")
        }
        TabButton {
            text: qsTr("Settings")
        }
    }
}
