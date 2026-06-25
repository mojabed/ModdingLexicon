import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

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
        }

        TabButton {
            text: qsTr("Browse Addons")
        }

        TabButton {
            text: qsTr("Settings")
        }
    }

    SwipeView {
        id: swipeView
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        currentIndex: bar.currentIndex

        interactive: false

        // Tab 0: My Addons
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

        // Tab 1: Browse Addons
        Item {
            GridView {
                anchors.fill: parent
                anchors.margins: 20
                cellWidth: 160
                cellHeight: 160
                clip: true
                model: lexicon.categoryModel

                delegate: Rectangle {
                    width: 150
                    height: 150
                    color: "#2a2a2a"
                    border.color: "#444"
                    border.width: 1
                    radius: 8
                    
                    Column {
                        anchors.centerIn: parent
                        anchors.margins: 10
                        spacing: 8
                        width: parent.width - 20

                        Image {
                            anchors.horizontalCenter: parent.horizontalCenter
                            source: model.iconSource
                            width: 64
                            height: 64
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width
                            text: model.categoryName
                            color: "#ffffff"
                            font.family: window.appFontFamily
                            font.pixelSize: 12
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: (model.addonCount || 0) + " addon" + ((model.addonCount || 0) !== 1 ? "s" : "")
                            color: "#90EE90"
                            font.family: window.appFontFamily
                            font.pixelSize: 11
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: parent.color = "#353535"
                        onExited: parent.color = "#2a2a2a"
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
            }
        }

        // Tab 2: Settings
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