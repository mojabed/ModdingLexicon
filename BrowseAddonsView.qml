import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    property string appFontFamily: "Segoe UI"
    property var appWindow
    property var lexiconController

    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    clip: true

    Rectangle {
        id: browseHeader
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

            Button {
                id: backButton
                visible: root.appWindow ? root.appWindow.viewingCategoryAddons : false
                Layout.preferredWidth: 108
                Layout.preferredHeight: 36
                Layout.alignment: Qt.AlignVCenter
                padding: 0
                font.family: root.appFontFamily
                font.pixelSize: 13
                font.bold: true
                hoverEnabled: true
                onClicked: {
                    console.log("Back button clicked")
                    root.appWindow.viewingCategoryAddons = false
                    root.lexiconController.installedAddonsFilter.setCategoryFilter("")
                    root.appWindow.selectedCategoryId = ""
                    root.appWindow.selectedCategoryName = ""
                }

                background: Rectangle {
                    color: backButton.hovered
                           ? Material.color(Material.Grey, Material.Shade700)
                           : Material.color(Material.Grey, Material.Shade800)
                    radius: 8
                    border.color: Material.color(Material.Grey, Material.Shade600)
                    border.width: 1
                }

                contentItem: Text {
                    text: "Back"
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font: backButton.font
                }
            }

            TextField {
                id: searchField
                Layout.preferredWidth: 320
                Layout.preferredHeight: 36
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true
                placeholderText: "Search for more addons"
                selectByMouse: true
                font.family: root.appFontFamily
                font.pixelSize: 13
                color: "white"
                background: Rectangle {
                    color: "#2a2a2a"
                    radius: 8
                    border.color: "#555"
                    border.width: 1
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Text {
                visible: root.appWindow ? root.appWindow.viewingCategoryAddons : false
                text: root.appWindow ? (root.appWindow.selectedCategoryName + " (" + categoryAddonsList.count + ")") : ""
                color: "white"
                font.family: root.appFontFamily
                font.pixelSize: 14
                font.bold: true
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Rectangle {
        anchors.top: browseHeader.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        color: "#1a1a1a"
        visible: root.appWindow ? !root.appWindow.viewingCategoryAddons : true
        clip: true

        GridView {
            id: categoryGrid
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            width: {
                const itemWidth = 160
                const margin = 20
                const availableWidth = parent.width - (margin * 2)
                const itemsPerRow = Math.max(1, Math.floor(availableWidth / itemWidth))
                return Math.min(itemsPerRow * itemWidth, availableWidth)
            }
            height: Math.max(0, parent.height - 40)
            model: root.lexiconController ? root.lexiconController.categoryModel : null
            cellWidth: 160
            cellHeight: 160

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
                        asynchronous: true
                        width: 64
                        height: 64
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width
                        text: model.categoryName
                        color: "#ffffff"
                        font.family: root.appFontFamily
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: (model.addonCount || 0) + " addon" + ((model.addonCount || 0) !== 1 ? "s" : "")
                        color: "#90EE90"
                        font.family: root.appFontFamily
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
                        root.appWindow.selectedCategoryId = model.categoryId
                        root.appWindow.selectedCategoryName = model.categoryName
                        root.lexiconController.installedAddonsFilter.setCategoryFilter(model.categoryId)
                        root.appWindow.viewingCategoryAddons = true
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

    Rectangle {
        anchors.top: browseHeader.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        color: "#1a1a1a"
        visible: root.appWindow ? root.appWindow.viewingCategoryAddons : false
        clip: true

        ListView {
            id: categoryAddonsList
            anchors.fill: parent
            anchors.margins: 10
            spacing: 5
            clip: true
            reuseItems: true
            cacheBuffer: 500
            model: root.lexiconController ? root.lexiconController.installedAddonsFilter : null

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
                        font.family: root.appFontFamily
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
}
