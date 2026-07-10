import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root

    property string appFontFamily: "Segoe UI"
    property var lexiconController: null

    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    clip: true

    Flickable {
        anchors.fill: parent
        contentHeight: settingsColumn.height + 40
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            implicitWidth: 8
        }

        ColumnLayout {
            id: settingsColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 32
            anchors.rightMargin: 32
            anchors.top: parent.top
            anchors.topMargin: 24
            spacing: 24

            GroupBox {
                title: "Addons Directory"
                Layout.fillWidth: true
                font.family: root.appFontFamily
                font.pixelSize: 15
                font.bold: true

                background: Rectangle {
                    y: parent.topPadding - parent.bottomPadding
                    width: parent.width
                    height: parent.height - parent.topPadding + parent.bottomPadding
                    color: "#232323"
                    border.color: "#444"
                    border.width: 1
                    radius: 6
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    Text {
                        text: "Detected location of your Elder Scrolls Online AddOns folder:"
                        color: "#aaaaaa"
                        font.family: root.appFontFamily
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        spacing: 8

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            radius: 4
                            color: "#1a1a1a"
                            border.color: "#555"
                            border.width: 1

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                text: root.lexiconController ? root.lexiconController.addonsPath : ""
                                color: "#cccccc"
                                font.family: root.appFontFamily
                                font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }

                        Button {
                            text: "Browse..."
                            Layout.preferredHeight: 36
                            font.family: root.appFontFamily
                            font.pixelSize: 13
                            Material.accent: Material.DeepPurple
                            highlighted: true

                            onClicked: folderDialog.open()
                        }
                    }

                    Text {
                        visible: pathStatusText.text !== ""
                        id: pathStatusText
                        text: ""
                        color: "#90EE90"
                        font.family: root.appFontFamily
                        font.pixelSize: 12
                    }
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Select AddOns Folder"
        currentFolder: {
            if (root.lexiconController) {
                var p = root.lexiconController.addonsPath
                if (p) return "file:///" + p.replace(/\\/g, "/")
            }
            return ""
        }

        onAccepted: {
            if (root.lexiconController) {
                var selectedPath = selectedFolder.toString().replace(/^(file:\/{2,3})/, "")
                selectedPath = decodeURIComponent(selectedPath)
                root.lexiconController.setAddonsPath(selectedPath)
                pathStatusText.text = "Path updated. Restart the app for changes to take effect."
            }
        }
    }
}
