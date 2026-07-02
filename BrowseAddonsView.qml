import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    property string appFontFamily: "Segoe UI"
    property var appWindow
    property var lexiconController
    property bool isSearching: searchField.text !== ""

    signal addonDetailRequested(var addonData)

    function goBack() {
        searchField.clear()
        if (root.appWindow) {
            root.appWindow.viewingCategoryAddons = false
            root.appWindow.selectedCategoryId = ""
            root.appWindow.selectedCategoryName = ""
        }
        if (root.lexiconController) {
            root.lexiconController.installedAddonsFilter.setCategoryFilter("")
            root.lexiconController.installedAddonsFilter.setShowInstalledOnly(true)
        }
    }

    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    clip: true

    onVisibleChanged: {
        if (visible && root.lexiconController) {
            var filter = root.lexiconController.installedAddonsFilter
            // Set default sort for browse view: downloads
            filter.setSortMode("downloads")
            filter.setSortOrder(Qt.DescendingOrder)

            // Show all addons unless viewing a category
            if (!root.appWindow || !root.appWindow.viewingCategoryAddons) {
                filter.setShowInstalledOnly(false)
                filter.setCategoryFilter("")
            }

            if (searchField.text !== "") {
                filter.setSearchText(searchField.text)
            }
        }
    }

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
                font.pixelSize: 15
                font.bold: true
                hoverEnabled: true
                onClicked: {
                    root.goBack()
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
                        var filter = root.lexiconController.installedAddonsFilter
                        filter.setSearchText(text)

                        if (text !== "" && !root.appWindow.viewingCategoryAddons) {
                            // Searching from grid: show all addons, clear category filter
                            filter.setShowInstalledOnly(false)
                            filter.setCategoryFilter("")
                        } else if (text === "" && !root.appWindow.viewingCategoryAddons) {
                            // Search cleared, back to grid
                            filter.setShowInstalledOnly(true)
                            filter.setCategoryFilter("")
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }

            // Sort button
            Item {
                width: 36
                height: 36
                Layout.alignment: Qt.AlignVCenter

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: sortMouse.containsMouse ? "#3a3a3a" : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "\u2B0D"
                        color: "#cccccc"
                        font.family: root.appFontFamily
                        font.pixelSize: 20
                    }

                    MouseArea {
                        id: sortMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sortPopup.open()
                    }
                }

                Popup {
                    id: sortPopup
                    y: parent.height
                    x: -120
                    width: 180
                    padding: 8
                    modal: false
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                    background: Rectangle {
                        color: "#2a2a2a"
                        radius: 8
                        border.color: "#555"
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Text {
                            text: "Sort by"
                            color: "#aaaaaa"
                            font.family: root.appFontFamily
                            font.pixelSize: 12
                            font.bold: true
                            Layout.leftMargin: 6
                        }

                        Repeater {
                            model: [
                                { mode: "downloads", label: "Downloads" },
                                { mode: "monthly", label: "Monthly Downloads" },
                                { mode: "favorites", label: "Favorites" },
                                { mode: "api", label: "API Version" }
                            ]

                            delegate: Item {
                                width: 164
                                height: 32

                                Rectangle {
                                    anchors.fill: parent
                                    radius: 4
                                    color: delegateMouse.containsMouse ? "#3a3a3a" : "transparent"
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8

                                    Text {
                                        text: modelData.label
                                        color: {
                                            var f = root.lexiconController ? root.lexiconController.installedAddonsFilter : null
                                            return f && f.sortMode === modelData.mode ? "#b39ddb" : "#cccccc"
                                        }
                                        font.family: root.appFontFamily
                                        font.pixelSize: 14
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: {
                                            var f = root.lexiconController ? root.lexiconController.installedAddonsFilter : null
                                            return f && f.sortMode === modelData.mode
                                                ? (f.sortOrder === Qt.DescendingOrder ? "\u2193" : "\u2191")
                                                : ""
                                        }
                                        color: "#b39ddb"
                                        font.family: root.appFontFamily
                                        font.pixelSize: 16
                                    }
                                }

                                MouseArea {
                                    id: delegateMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (root.lexiconController) {
                                            var filter = root.lexiconController.installedAddonsFilter
                                            if (filter.sortMode === modelData.mode) {
                                                filter.setSortOrder(
                                                    filter.sortOrder === Qt.DescendingOrder
                                                        ? Qt.AscendingOrder : Qt.DescendingOrder
                                                )
                                            } else {
                                                filter.setSortMode(modelData.mode)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Filter outdated toggle
            RowLayout {
                spacing: 6
                Layout.alignment: Qt.AlignVCenter

                Text {
                    text: "Hide outdated"
                    color: "#aaaaaa"
                    font.family: root.appFontFamily
                    font.pixelSize: 13
                }

                Switch {
                    id: outdatedToggle
                    checked: true
                    Material.accent: Material.DeepPurple
                    onCheckedChanged: {
                        if (root.lexiconController) {
                            root.lexiconController.installedAddonsFilter.setExcludeBelowApiVersion(
                                checked ? 101042 : 0
                            )
                            root.lexiconController.refreshCategoryCounts()
                        }
                    }
                }
            }

            Text {
                visible: {
                    if (!root.appWindow) return false
                    return root.appWindow.viewingCategoryAddons || root.isSearching
                }
                text: {
                    if (!root.appWindow) return ""
                    if (root.isSearching && !root.appWindow.viewingCategoryAddons)
                        return "Search results (" + categoryAddonsList.count + ")"
                    return root.appWindow.selectedCategoryName + " (" + categoryAddonsList.count + ")"
                }
                color: "white"
                font.family: root.appFontFamily
                font.pixelSize: 16
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
        visible: root.appWindow ? (!root.appWindow.viewingCategoryAddons && !root.isSearching) : true
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
                        font.pixelSize: 16
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: (model.addonCount || 0) + " addon" + ((model.addonCount || 0) !== 1 ? "s" : "")
                        color: "#90EE90"
                        font.family: root.appFontFamily
                        font.pixelSize: 13
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.color = "#353535"
                    onExited: parent.color = "#2a2a2a"
                    onPressed: parent.color = "#311b44"
                    onReleased: parent.color = containsMouse ? "#353535" : "#2a2a2a"
                    onClicked: {
                        root.appWindow.selectedCategoryId = model.categoryId
                        root.appWindow.selectedCategoryName = model.categoryName
                        var filter = root.lexiconController.installedAddonsFilter
                        filter.setCategoryFilter(model.categoryId)
                        filter.setSortMode("downloads")
                        filter.setSortOrder(Qt.DescendingOrder)
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
        visible: root.appWindow ? (root.appWindow.viewingCategoryAddons || root.isSearching) : false
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

            delegate: Rectangle {
                width: categoryAddonsList.width - 20
                height: 60
                color: "#2a2a2a"
                radius: 6

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.color = "#353535"
                    onExited: parent.color = "#2a2a2a"
                    onPressed: parent.color = "#311b44"
                    onReleased: parent.color = containsMouse ? "#353535" : "#2a2a2a"
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
                            "downloadUrl": model.downloadUrl || "",
                            "gameVersion": model.gameVersion || "",
                            "hasUpdate": model.hasUpdate || false
                        })
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
                        width: Math.max(0, parent.width - 48)
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
