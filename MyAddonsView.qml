import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    property string appFontFamily: "Segoe UI"
    property var lexiconController
    property bool isSearching: searchField.text !== ""

    signal addonDetailRequested(var addonData)

    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    clip: true

    onVisibleChanged: {
        if (visible && root.lexiconController) {
            root.lexiconController.installedAddonsFilter.setShowInstalledOnly(true)
        }
    }

    Rectangle {
        id: myAddonsHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 52
        color: "#232323"
        border.color: "#444"
        border.width: 1
        z: 2

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            anchors.topMargin: 6
            anchors.bottomMargin: 6
            spacing: 10

            TextField {
                id: searchField
                Layout.preferredWidth: 320
                Layout.preferredHeight: 36
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true
                placeholderText: "Search installed addons"
                selectByMouse: true
                font.family: root.appFontFamily
                font.pixelSize: 15
                color: "white"
                background: Rectangle {
                    color: "#2a2a2a"
                    radius: 8
                    border.color: "#555"
                    border.width: 1
                }

                onTextChanged: {
                    if (root.lexiconController) {
                        root.lexiconController.installedAddonsFilter.setSearchText(text)
                    }
                }
            }

            Text {
                visible: root.isSearching
                text: myAddonsList.count + " result" + (myAddonsList.count !== 1 ? "s" : "")
                color: "white"
                font.family: root.appFontFamily
                font.pixelSize: 16
                font.bold: true
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    ListView {
        id: myAddonsList
        anchors.top: myAddonsHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        model: root.lexiconController ? root.lexiconController.installedAddonsFilter : null
        spacing: 5
        clip: true
        reuseItems: true
        cacheBuffer: 500

        delegate: Rectangle {
            width: myAddonsList.width - 20
            height: 60
            color: "#2a2a2a"
            radius: 6

            Rectangle {
                anchors.fill: parent
                color: "transparent"
                radius: 6

                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: 50
                    hoverEnabled: true
                    onEntered: parent.parent.color = "#353535"
                    onExited: parent.parent.color = "#2a2a2a"
                    onPressed: parent.parent.color = "#311b44"
                    onReleased: parent.parent.color = containsMouse ? "#353535" : "#2a2a2a"
                    onClicked: {
                        root.addonDetailRequested({
                            "title": model.title,
                            "author": model.author,
                            "version": model.version,
                            "lastUpdated": model.lastUpdated,
                            "formattedDate": model.formattedDate,
                            "categoryId": model.categoryId,
                            "categoryName": model.categoryName,
                            "downloads": model.downloads || 0,
                            "downloadsMonthly": model.downloadsMonthly || 0,
                            "favorites": model.favorites || 0,
                            "isInstalled": model.isInstalled || false,
                            "iconSource": model.iconSource,
                            "fileInfoUri": model.fileInfoUri || "",
                            "modId": model.modId || "",
                            "downloadUrl": model.downloadUrl || ""
                        })
                    }
                }
            }

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

                Column {
                    width: Math.max(0, parent.width - 48 - 42)
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 1

                    Text {
                        width: parent.width
                        text: model.title || "No data"
                        color: "white"
                        font.family: root.appFontFamily
                        font.pixelSize: 16
                        font.bold: true
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }

                    Text {
                        width: parent.width
                        text: model.version || ""
                        color: "#aaaaaa"
                        font.family: root.appFontFamily
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        visible: model.version !== ""
                    }

                    Text {
                        width: parent.width
                        text: "by " + (model.author || "")
                        color: "#aaaaaa"
                        font.family: root.appFontFamily
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }
                }

                Item { width: 6 }

                Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    anchors.verticalCenter: parent.verticalCenter
                    color: uninstallMouse.containsMouse ? "#4a2828" : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "\u00D7"
                        color: uninstallMouse.containsMouse ? "#ef9a9a" : "#555555"
                        font.family: root.appFontFamily
                        font.pixelSize: 22
                    }

                    MouseArea {
                        id: uninstallMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.lexiconController) {
                                root.lexiconController.uninstallAddon(model.modId, model.title)
                            }
                        }
                    }
                }
            }
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            implicitWidth: 8
            background: Rectangle { color: "transparent" }
            contentItem: Rectangle {
                implicitWidth: 8
                implicitHeight: 24
                radius: width / 2
                color: Material.color(Material.Grey, Material.Shade700)
            }
        }
    }
}
