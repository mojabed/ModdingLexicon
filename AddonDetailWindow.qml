import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: detailWindow

    property string addonTitle: ""
    property string addonAuthor: ""
    property string addonVersion: ""
    property string addonLastUpdated: ""
    property string addonCategory: ""
    property string addonDescription: ""
    property string addonFileInfoUri: ""
    property string addonId: ""
    property string addonDownloadUrl: ""
    property int addonDownloads: 0
    property int addonDownloadsMonthly: 0
    property int addonFavorites: 0
    property bool addonIsInstalled: false
    property bool addonIsInstalling: false
    property int addonInstallPercent: 0
    property url addonIconSource

    property string appFontFamily: "Segoe UI"
    property var lexiconController: null

    readonly property string descriptionHtml: {
        if (lexiconController && addonFileInfoUri)
            return lexiconController.currentDescription || ""
        return ""
    }

    function formatNumber(n) {
        return n.toLocaleString(Qt.locale(), 'f', 0)
    }

    width: 900
    height: 750
    minimumWidth: 480
    minimumHeight: 500

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

    color: "#1e1e1e"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        // Header: icon + title + install button
        RowLayout {
            spacing: 12

            Image {
                source: detailWindow.addonIconSource.toString() !== "" ? detailWindow.addonIconSource : ""
                width: 40
                height: 40
                fillMode: Image.PreserveAspectFit
                Layout.alignment: Qt.AlignTop
                visible: detailWindow.addonIconSource.toString() !== ""
            }

            ColumnLayout {
                spacing: 1

                Text {
                    text: detailWindow.addonTitle || "Unknown Addon"
                    color: "white"
                    font.family: detailWindow.appFontFamily
                    font.pixelSize: 21
                    font.bold: true
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Text {
                    text: "by " + (detailWindow.addonAuthor || "Unknown")
                    color: "#aaaaaa"
                    font.family: detailWindow.appFontFamily
                    font.pixelSize: 14
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: installButton
                Layout.preferredWidth: 130
                Layout.preferredHeight: 44
                Layout.alignment: Qt.AlignTop
                visible: !detailWindow.addonIsInstalled || detailWindow.addonIsInstalling
                enabled: !detailWindow.addonIsInstalling && detailWindow.addonDownloadUrl !== ""

                font.family: detailWindow.appFontFamily
                font.pixelSize: 16
                font.bold: true

                background: Rectangle {
                    radius: 22
                    color: installButton.enabled
                           ? (installButton.hovered ? Material.color(Material.DeepPurple, Material.Shade400)
                                                    : Material.color(Material.DeepPurple, Material.Shade500))
                           : Material.color(Material.Grey, Material.Shade700)
                    border.color: installButton.enabled
                                  ? Material.color(Material.DeepPurple, Material.Shade300)
                                  : Material.color(Material.Grey, Material.Shade600)
                    border.width: 1
                }

                contentItem: Text {
                    text: detailWindow.addonIsInstalling
                          ? "Installing " + detailWindow.addonInstallPercent + "%"
                          : "Install"
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font: installButton.font
                }

                onClicked: {
                    if (detailWindow.lexiconController) {
                        detailWindow.addonIsInstalling = true
                        detailWindow.addonInstallPercent = 0
                        detailWindow.lexiconController.installAddon(
                            detailWindow.addonId,
                            detailWindow.addonTitle,
                            detailWindow.addonDownloadUrl
                        )
                    }
                }
            }
        }

        // Compact info row
        RowLayout {
            spacing: 20

            InfoBadge { label: "v" + (detailWindow.addonVersion || "-") }
            InfoBadge { label: detailWindow.addonCategory || "-" }
            InfoBadge { label: formatNumber(detailWindow.addonDownloads) + " DL" }
            InfoBadge {
                label: detailWindow.addonIsInstalled ? "Installed" : "Not Installed"
                badgeColor: detailWindow.addonIsInstalled ? "#4caf50" : "#888"
            }
        }

        // Divider
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#444"
        }

        // Description area
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollView {
                id: scrollView
                anchors.fill: parent
                clip: true

                TextEdit {
                    id: descriptionText
                    width: scrollView.width - 20
                    readOnly: true
                    selectByMouse: true
                    textFormat: TextEdit.RichText
                    color: "#cccccc"
                    font.family: detailWindow.appFontFamily
                    font.pixelSize: 15
                    wrapMode: TextEdit.Wrap
                    text: detailWindow.descriptionHtml || "Loading description..."
                    leftPadding: 10
                    rightPadding: 10
                    topPadding: 10
                    activeFocusOnPress: true
                    persistentSelection: false

                    onLinkActivated: function(link) {
                        if (link.startsWith("file:///")) {
                            imageOverlay.source = link
                            imageOverlay.open()
                        } else {
                            Qt.openUrlExternally(link)
                        }
                    }
                }
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: detailWindow.descriptionHtml === ""
                Material.accent: Material.DeepPurple
            }
        }
    }

    Popup {
        id: imageOverlay
        modal: true
        dim: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, fullImage.sourceSize.width > 0 ? fullImage.sourceSize.width + 16 : parent.width - 40)
        height: Math.min(parent.height - 40, fullImage.sourceSize.height > 0 ? fullImage.sourceSize.height + 44 : parent.height - 40)
        padding: 0
        background: Rectangle {
            color: "#111"
            radius: 8
            border.color: "#555"
        }

        property alias source: fullImage.source

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Label {
                    text: "\u00D7"
                    color: "#ccc"
                    font.pixelSize: 22
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: imageOverlay.close()
                    }
                }
            }

            Image {
                id: fullImage
                Layout.fillWidth: true
                Layout.fillHeight: true
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                onStatusChanged: {
                    if (status === Image.Ready) {
                        imageOverlay.width = Math.min(sourceSize.width + 16, imageOverlay.parent.width - 40)
                        imageOverlay.height = Math.min(sourceSize.height + 44, imageOverlay.parent.height - 40)
                    }
                }
            }
        }
    }

    Connections {
        target: detailWindow.lexiconController
        enabled: detailWindow.lexiconController !== null

        function onAddonInstallStarted(modId) {
            if (modId === detailWindow.addonId) {
                detailWindow.addonIsInstalling = true
                detailWindow.addonInstallPercent = 0
            }
        }

        function onAddonInstallProgress(modId, percent) {
            if (modId === detailWindow.addonId) {
                detailWindow.addonInstallPercent = percent
            }
        }

        function onAddonInstallFinished(modId) {
            if (modId === detailWindow.addonId) {
                detailWindow.addonIsInstalling = false
                detailWindow.addonIsInstalled = true
            }
        }

        function onAddonInstallFailed(modId, error) {
            if (modId === detailWindow.addonId) {
                detailWindow.addonIsInstalling = false
                console.error("Install failed:", error)
            }
        }
    }

    component InfoBadge: Text {
        property string label: ""
        property string badgeColor: "#cccccc"

        text: label
        color: badgeColor
        font.family: detailWindow.appFontFamily
        font.pixelSize: 14
    }
}
