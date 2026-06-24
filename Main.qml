import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

import ModdingLexicon   

ApplicationWindow {
    id: window
    width: 1280
    height: 720
    visible: true
    title: qsTr("Modding Lexicon")
    minimumWidth: 640   
    minimumHeight: 480

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

    property string appFontFamily: "Segoe UI"

    Lexicon {
        id: lexicon

        onMasterListReady: (filePath) => {
            console.log("Master list ready at: " + filePath)
            statusText.text = "Master list loaded."
        }
    }

    TabBar {
        id: bar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 72
        contentHeight: 72

        background: Rectangle {
            color: "#232323"
        }

        font.family: appFontFamily
        font.pixelSize: 20
        font.bold: true

        TabButton {
            text: qsTr("My Addons")
            //hoverEnabled: false
        }

        TabButton {
            text: qsTr("Browse Addons")
            //hoverEnabled: false
        }

        TabButton {
            text: qsTr("Settings")
            //hoverEnabled: false
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
            font.family: appFontFamily
        }
    }

    SwipeView {
        id: swipeView
        anchors.top: statusBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        currentIndex: bar.currentIndex

        interactive: false

        Item {
            ListView {
                anchors.fill: parent
                anchors.margins: 10
                model: lexicon.installedAddonsFilter
                spacing: 5
                clip: true
                reuseItems: true
                cacheBuffer: 500

                delegate: ListItemDelegate {
                    appFontFamily: window.appFontFamily
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
            }
        }

        Item {
            ListView {
                anchors.fill: parent
                anchors.margins: 10
                model: lexicon.addonModel
                spacing: 5
                clip: true
                reuseItems: true
                cacheBuffer: 500

                delegate: ListItemDelegate {
                    appFontFamily: window.appFontFamily
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
            }
        }

        Item {
            Text {
                anchors.centerIn: parent
                text: "Settings Page"
                color: "white"
                font.family: appFontFamily
                font.pixelSize: 16
            }
        }
    }
}
