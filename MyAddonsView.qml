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

        delegate: ListItemDelegate {
            title: model.title
            author: model.author
            version: model.version
            iconSource: model.iconSource
            appFontFamily: root.appFontFamily

            onAddonClicked: {
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
