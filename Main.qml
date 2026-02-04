import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

import ModdingLexicon   

ApplicationWindow {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Modding Lexicon")
    minimumWidth: 640   
    minimumHeight: 480

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

    FontLoader { // Using Inter font from Google
        id: rcFont
        source: "qrc:/fonts/InterVariable.ttf"
    }

    Lexicon {
        id: lexicon

        onMasterListReady: (filePath) => {
            console.log("Master list ready at: " + filePath)
            statusText.text = "Master list loaded."
        }
    }

    TabBar { // Navigation bar at top
        id: bar
        //width: parent.width
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
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

    Rectangle {
        id: statusBar
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 50
        color: "#1a1a1a"
        border.color: "#444"
        border.width: 1

        Text {
            id: statusText
            anchors.centerIn: parent
            color: "#90EE90"
            text: "Loading master list..."
            font.pixelSize: 14
            font.family: rcFont.font.family
        }
    }
}