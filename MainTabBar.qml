import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

TabBar {
    id: bar
    height: 72
    contentHeight: 72

    property string appFontFamily: "Segoe UI"

    signal browseTabReclicked()

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple
    Material.foreground: Material.PrimaryText
    Material.background: "#232323"

    background: Rectangle {
        color: Material.background
    }

    font.family: appFontFamily
    font.pixelSize: 23
    font.bold: true

    TabButton {
        text: qsTr("My Addons")
    }

    TabButton {
        text: qsTr("Browse Addons")
        onClicked: {
            if (bar.currentIndex === 1) {
                bar.browseTabReclicked()
            }
        }
    }

    TabButton {
        text: qsTr("Settings")
    }
}
