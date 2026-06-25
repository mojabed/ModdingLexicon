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
    property string selectedCategoryId: ""
    property string selectedCategoryName: ""
    property bool viewingCategoryAddons: false

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
            // Category Grid View (visible when not viewing category addons)
            GridView {
                id: categoryGrid
                anchors.fill: parent
                anchors.margins: 20
                cellWidth: 160
                cellHeight: 160
                clip: true
                model: lexicon.categoryModel
                visible: !window.viewingCategoryAddons

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
                        onClicked: {
                            console.log("Category clicked:", model.categoryId, "Count:", model.addonCount)
                            window.selectedCategoryId = model.categoryId
                            window.selectedCategoryName = model.categoryName
                            lexicon.installedAddonsFilter.setCategoryFilter(model.categoryId)
                            window.viewingCategoryAddons = true
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
            }

            // Category Addons List (visible when viewing category addons)
            Rectangle {
                anchors.fill: parent
                color: "#1a1a1a"
                visible: window.viewingCategoryAddons

                Rectangle {
                    id: headerRect
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 50
                    color: "#232323"
                    border.color: "#444"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Button {
                            text: "← Back"
                            Layout.preferredWidth: 80
                            onClicked: {
                                console.log("Back button clicked")
                                window.viewingCategoryAddons = false
                                lexicon.installedAddonsFilter.setCategoryFilter("")
                                window.selectedCategoryId = ""
                                window.selectedCategoryName = ""
                            }
                        }

                        Text {
                            text: window.selectedCategoryName + " (" + categoryAddonsList.count + ")"
                            color: "white"
                            font.family: window.appFontFamily
                            font.pixelSize: 14
                            font.bold: true
                            Layout.fillWidth: true
                        }
                    }
                }

                ListView {
                    id: categoryAddonsList
                    anchors.top: headerRect.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10
                    spacing: 5
                    clip: true
                    reuseItems: true
                    cacheBuffer: 500
                    model: lexicon.installedAddonsFilter
                    
                    onCountChanged: console.log("ListView count changed to:", count)

                    delegate: Rectangle {
                        width: categoryAddonsList.width - 20
                        height: 60
                        color: "#2a2a2a"
                        radius: 6
                        
                        Component.onCompleted: console.log("Delegate created - title:", model.title)

                        Row {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 12

                            Image {
                                source: model.iconSource
                                width: 36
                                height: 36
                                fillMode: Image.PreserveAspectFit
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Text {
                                width: Math.max(0, parent.width - 48)
                                text: model.title ? (model.title + "\nby " + model.author) : "No data"
                                color: "white"
                                font.family: window.appFontFamily
                                font.pixelSize: 16
                                maximumLineCount: 2
                                elide: Text.ElideRight
                                wrapMode: Text.WordWrap
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }
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