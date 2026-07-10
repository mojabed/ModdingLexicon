import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    property string appFontFamily: "Segoe UI"
    property var lexiconController
    property var appWindow
    property bool isSearching: searchField.text !== ""

    signal addonDetailRequested(var addonData)

    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    clip: true

    onVisibleChanged: {
        if (visible && root.lexiconController) {
            var filter = root.lexiconController.installedAddonsFilter
            filter.setShowInstalledOnly(true)
            filter.setSortMode("title")
            filter.setSortOrder(Qt.AscendingOrder)
        }
    }

    Rectangle {
        id: myAddonsHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 52
        color: root.appWindow ? root.appWindow.headerColor : "#232323"
        border.color: root.appWindow ? root.appWindow.headerBorder : "#444"
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
                color: root.appWindow ? root.appWindow.headingColor : "white"
                background: Rectangle {
                    color: root.appWindow ? root.appWindow.cardColor : "#2a2a2a"
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
                color: root.appWindow ? root.appWindow.headingColor : "white"
                font.family: root.appFontFamily
                font.pixelSize: 16
                font.bold: true
                Layout.alignment: Qt.AlignVCenter
            }

            Button {
                visible: root.lexiconController && root.lexiconController.availableUpdates > 1
                text: "Update All (" + (root.lexiconController ? root.lexiconController.availableUpdates : 0) + ")"
                Layout.preferredHeight: 32
                Layout.alignment: Qt.AlignVCenter
                font.family: root.appFontFamily
                font.pixelSize: 12
                font.bold: true
                Material.accent: Material.DeepPurple
                highlighted: true
                onClicked: {
                    if (!root.lexiconController) return
                    var f = root.lexiconController.installedAddonsFilter
                    for (var i = 0; i < f.rowCount(); i++) {
                        var idx = f.index(i, 0)
                        if (f.data(idx, 0x0114)) {
                            root.lexiconController.installAddon(
                                f.data(idx, 0x0101), f.data(idx, 0x0103), f.data(idx, 0x0110))
                        }
                    }
                }
            }

            // Clean libraries button
            Button {
                id: cleanBtn
                Layout.preferredHeight: 32
                Layout.alignment: Qt.AlignVCenter
                font.family: root.appFontFamily
                font.pixelSize: 12
                font.bold: true
                Material.accent: Material.DeepPurple
                highlighted: true
                text: "Clean"
                onClicked: cleanDialog.open()
            }
        }
    }

    // Clean libraries dialog
    Dialog {
        id: cleanDialog
        title: "Clean Unused Libraries"
        anchors.centerIn: parent
        width: 420
        modal: true
        closePolicy: Popup.NoAutoClose
        Material.accent: Material.DeepPurple

        property int cleanedCount: 0
        property bool cleaningInProgress: false
        property bool cleaningDone: false

        background: Rectangle {
            color: "#232323"; radius: 8; border.color: "#444"; border.width: 1
        }

        header: Text {
            text: cleanDialog.title
            color: "white"
            font.family: root.appFontFamily
            font.pixelSize: 16
            font.bold: true
            leftPadding: 20; topPadding: 16; bottomPadding: 8
        }

        contentItem: ColumnLayout {
            spacing: 16
            width: parent ? parent.width - 40 : 380

            Text {
                visible: !cleanDialog.cleaningInProgress && !cleanDialog.cleaningDone
                text: "Cleaning will remove libraries not currently being used by any installed addon."
                color: "#aaaaaa"
                font.family: root.appFontFamily
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Text {
                visible: !cleanDialog.cleaningInProgress && !cleanDialog.cleaningDone
                text: "Are you sure you want to uninstall unused libraries?"
                color: "white"
                font.family: root.appFontFamily
                font.pixelSize: 14
                font.bold: true
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            ColumnLayout {
                visible: cleanDialog.cleaningInProgress
                spacing: 6
                Layout.fillWidth: true

                Text {
                    text: "Removing unused libraries..."
                    color: "#aaaaaa"
                    font.family: root.appFontFamily
                    font.pixelSize: 13
                }

                ProgressBar {
                    id: cleanProgressBar
                    from: 0; to: 1; value: 0
                    Layout.fillWidth: true
                    Material.accent: Material.DeepPurple
                }

                Text {
                    text: cleanProgressLabel.text
                    color: "#888888"
                    font.family: root.appFontFamily
                    font.pixelSize: 12
                }

                Text {
                    id: cleanProgressLabel
                    text: ""
                }
            }

            ColumnLayout {
                visible: cleanDialog.cleaningDone
                spacing: 8
                Layout.fillWidth: true

                Text {
                    text: cleanDialog.cleanedCount + " libraries removed successfully."
                    color: "#90EE90"
                    font.family: root.appFontFamily
                    font.pixelSize: 14
                    font.bold: true
                }
            }

            // Buttons
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight
                spacing: 10

                // Yes / No
                Button {
                    visible: !cleanDialog.cleaningInProgress && !cleanDialog.cleaningDone
                    text: "Yes"
                    font.family: root.appFontFamily
                    font.pixelSize: 13
                    Material.accent: Material.DeepPurple
                    highlighted: true
                    onClicked: {
                        cleanDialog.cleaningInProgress = true
                        root.lexiconController.cleanUnusedLibraries()
                    }
                }
                Button {
                    visible: !cleanDialog.cleaningInProgress && !cleanDialog.cleaningDone
                    text: "No"
                    font.family: root.appFontFamily
                    font.pixelSize: 13
                    onClicked: cleanDialog.close()
                }

                // Done button
                Button {
                    visible: cleanDialog.cleaningDone
                    text: "Done"
                    font.family: root.appFontFamily
                    font.pixelSize: 13
                    Material.accent: Material.DeepPurple
                    highlighted: true
                    onClicked: {
                        cleanDialog.cleaningDone = false
                        cleanDialog.cleaningInProgress = false
                        cleanDialog.close()
                    }
                }
            }
        }

        onClosed: {
            cleanDialog.cleaningInProgress = false
            cleanDialog.cleaningDone = false
            cleanProgressBar.value = 0
            cleanProgressLabel.text = ""
        }
    }

    Connections {
        target: root.lexiconController
        function onCleanLibrariesProgress(current, total, libTitle) {
            cleanProgressBar.to = total
            cleanProgressBar.value = current
            cleanProgressLabel.text = "(" + current + "/" + total + ") " + libTitle
        }
        function onCleanLibrariesFinished(count) {
            cleanProgressBar.value = cleanProgressBar.to
            cleanDialog.cleaningInProgress = false
            cleanDialog.cleaningDone = true
            cleanDialog.cleanedCount = count
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
            color: root.appWindow ? root.appWindow.cardColor : "#2a2a2a"
            radius: 6

            Rectangle {
                anchors.fill: parent
                color: "transparent"
                radius: 6

                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: 50
                    hoverEnabled: true
                    onEntered: parent.parent.color = root.appWindow ? root.appWindow.cardHoverColor : "#353535"
                    onExited: parent.parent.color = root.appWindow ? root.appWindow.cardColor : "#2a2a2a"
                    onPressed: parent.parent.color = root.appWindow ? root.appWindow.cardPressColor : "#311b44"
                    onReleased: parent.parent.color = containsMouse ? (root.appWindow ? root.appWindow.cardHoverColor : "#353535") : (root.appWindow ? root.appWindow.cardColor : "#2a2a2a")
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
                    width: Math.max(0, parent.width - 48 - 110)
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 1

                    Text {
                        width: parent.width
                        text: model.title || "No data"
                        color: root.appWindow ? root.appWindow.headingColor : "white"
                        font.family: root.appFontFamily
                        font.pixelSize: 16
                        font.bold: true
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }

                    Text {
                        width: parent.width
                        text: model.version || ""
                        color: root.appWindow ? root.appWindow.subtitleColor : "#aaaaaa"
                        font.family: root.appFontFamily
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        visible: model.version !== ""
                    }

                    Text {
                        width: parent.width
                        text: "by " + (model.author || "")
                        color: root.appWindow ? root.appWindow.subtitleColor : "#aaaaaa"
                        font.family: root.appFontFamily
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }
                }

                // Update button
                Text {
                    id: updateBtn
                    text: "Update"
                    color: updMouse.containsMouse ? "#b39ddb" : "#666666"
                    font.family: root.appFontFamily
                    font.pixelSize: 12
                    font.bold: true
                    visible: model.hasUpdate || false
                    anchors.verticalCenter: parent.verticalCenter

                    MouseArea {
                        id: updMouse
                        anchors.fill: parent
                        anchors.margins: -6
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.lexiconController)
                                root.lexiconController.installAddon(model.modId, model.title, model.downloadUrl)
                        }
                    }
                }

                Item { width: 6; height: 1 }

                // Uninstall button
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
                            uninstallTargetId = model.modId
                            uninstallTargetTitle = model.title
                            uninstallConfirmDialog.open()
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

    property string uninstallTargetId: ""
    property string uninstallTargetTitle: ""

    Dialog {
        id: uninstallConfirmDialog
        title: "Uninstall Addon"
        anchors.centerIn: parent
        width: 340
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        Material.accent: Material.DeepPurple

        background: Rectangle {
            color: root.appWindow ? root.appWindow.cardColor : "#2a2a2a"
            radius: 8
            border.color: "#555"
        }

        contentItem: Text {
            text: "Are you sure you want to uninstall \"" + root.uninstallTargetTitle + "\"?"
            color: root.appWindow ? root.appWindow.subtitleColor : "#cccccc"
            font.family: root.appFontFamily
            font.pixelSize: 14
            wrapMode: Text.WordWrap
        }

        onAccepted: {
            if (root.lexiconController) {
                root.lexiconController.uninstallAddon(root.uninstallTargetId, root.uninstallTargetTitle)
            }
        }
    }
}
