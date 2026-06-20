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

    property bool smoothAnimation: true
    property string appFontFamily: "Inter"

    FontLoader { // Using Inter font from Google
        id: rcFont
        source: "qrc:/fonts/InterVariable.ttf"
        onStatusChanged: {
            if (status === FontLoader.Ready) {
                appFontFamily = rcFont.font.family
    }
        }
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
        currentIndex: swipeView.currentIndex
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        //height: parent.height * 0.15
        //contentHeight: parent.height * 0.15
        height: 72
        contentHeight: 72

        background: Rectangle {
            color: "#232323"
        }

        font.family: appFontFamily
        font.pixelSize: 20
        font.bold: true
        TabButton {
            //id: myAddonsTab
            text: qsTr("My Addons")
            
        }
        TabButton {
            //id: browseAddonsTab
            text: qsTr("Browse Addons")
        }
        TabButton {
            //id: settingsTab
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

        Loader {
            active: SwipeView.isCurrentItem || SwipeView.isNextItem || SwipeView.isPreviousItem
            sourceComponent: Item {
                ListView {
                    anchors.fill: parent
                    anchors.margins: 10
                    model: lexicon.installedAddonsFilter
                    spacing: 5
                    clip: true
                    
                    reuseItems: true

                    cacheBuffer: 500
                    displayMarginBeginning: 200
                    displayMarginEnd: 200

                    delegate: Rectangle {
                        required property string title
                        required property string author
                        
                        width: ListView.view.width
                        height: 40
                        color: "#2a2a2a"
                        radius: 4

                        Text {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            
                            text: title + " by " + author
                            color: "white"
                        font.family: appFontFamily
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }
                }
            }
        }

        // Browse Addons Page
        Loader {
            active: SwipeView.isCurrentItem || SwipeView.isNextItem || SwipeView.isPreviousItem
            sourceComponent: Item {
                ListView {
                    anchors.fill: parent
                    anchors.margins: 10
                    model: lexicon.addonModel
                    spacing: 5
                    clip: true

                    reuseItems: true

                    cacheBuffer: 500
                    displayMarginBeginning: 200
                    displayMarginEnd: 200

                    delegate: Rectangle {
                        required property string title
                        required property string author
                        
                        width: ListView.view.width
                        height: 40
                        color: "#2a2a2a"
                        radius: 4

                        Text {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            
                        text: title + " by " + author
                            color: "white"
                        font.family: appFontFamily
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }
                }
            }
        }

        // Settings Page
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
