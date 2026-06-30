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
    property string addonGameVersion: ""
    property int addonDownloads: 0
    property int addonDownloadsMonthly: 0
    property int addonFavorites: 0
    property bool addonIsInstalled: false
    property bool addonIsInstalling: false
    property int addonInstallPercent: 0
    property string addonInstallStatus: ""
    property string addonDependencyTitle: ""
    property url addonIconSource

    property string appFontFamily: "Segoe UI"
    property var lexiconController: null

    property string gameVersionLabel: ""
    property bool addonIsOutdated: false

    Timer {
        id: gameVersionPoller
        interval: 200
        running: true
        repeat: true
        onTriggered: {
            if (!detailWindow.gameVersionLabel && detailWindow.lexiconController && detailWindow.addonId) {
                var gv = detailWindow.lexiconController.getGameVersionForAddon(detailWindow.addonId)
                if (gv) {
                    detailWindow.gameVersionLabel = "game version: " + gv
                    var api = detailWindow.lexiconController.getAddonApiVersion(detailWindow.addonId)
                    detailWindow.addonIsOutdated = (api > 0 && api < 101042) || api === 0
                }
            }
        }
    }

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
                Layout.preferredWidth: 150
                Layout.preferredHeight: 44
                Layout.alignment: Qt.AlignTop
                enabled: !detailWindow.addonIsInstalling && (detailWindow.addonIsInstalled || detailWindow.addonDownloadUrl !== "")

                font.family: detailWindow.appFontFamily
                font.pixelSize: 16
                font.bold: true

                readonly property bool isUninstall: detailWindow.addonIsInstalled && !detailWindow.addonIsInstalling

                background: Rectangle {
                    radius: 22
                    color: {
                        if (!installButton.enabled)
                            return Material.color(Material.Grey, Material.Shade700)
                        if (installButton.isUninstall)
                            return installButton.hovered ? "#5e2a2a" : "#4a2020"
                        return installButton.hovered ? Material.color(Material.DeepPurple, Material.Shade400)
                                                     : Material.color(Material.DeepPurple, Material.Shade500)
                    }
                    border.color: {
                        if (!installButton.enabled)
                            return Material.color(Material.Grey, Material.Shade600)
                        if (installButton.isUninstall)
                            return installButton.hovered ? "#8e4a4a" : "#6e3535"
                        return Material.color(Material.DeepPurple, Material.Shade300)
                    }
                    border.width: 1
                }

                contentItem: Text {
                    text: {
                        if (detailWindow.addonIsInstalling) {
                            if (detailWindow.addonInstallStatus === "installing_dependencies" && detailWindow.addonDependencyTitle)
                                return "Dep: " + detailWindow.addonDependencyTitle
                            return "Installing " + detailWindow.addonInstallPercent + "%"
                        }
                        if (installButton.isUninstall)
                            return "Uninstall"
                        return "Install"
                    }
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font: installButton.font
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: 10
                }

                onClicked: {
                    if (!detailWindow.lexiconController)
                        return
                    if (installButton.isUninstall) {
                        detailWindow.lexiconController.uninstallAddon(
                            detailWindow.addonId,
                            detailWindow.addonTitle
                        )
                    } else {
                        detailWindow.addonIsInstalling = true
                        detailWindow.addonInstallPercent = 0
                        detailWindow.addonInstallStatus = ""
                        detailWindow.addonDependencyTitle = ""
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
            InfoBadge { label: formatNumber(detailWindow.addonDownloadsMonthly) + " DL/mo" }
            InfoBadge { label: detailWindow.addonFavorites + " \u2605" }
            InfoBadge {
                label: detailWindow.addonIsInstalled ? "Installed" : "Not Installed"
                badgeColor: detailWindow.addonIsInstalled ? "#4caf50" : "#888"
            }
        }

        // Game version
        Text {
            visible: detailWindow.gameVersionLabel !== ""
            text: detailWindow.gameVersionLabel
            color: "#8eb5d6"
            font.family: detailWindow.appFontFamily
            font.pixelSize: 13
        }

        // Outdated warning
        Text {
            visible: detailWindow.addonIsOutdated
            text: {
                var api = detailWindow.lexiconController.getAddonApiVersion(detailWindow.addonId)
                if (api === 0)
                    return "No API version data \u2014 this addon may be outdated"
                return "Outdated addon (API " + api + ") \u2014 may not function correctly"
            }
            color: "#e8a838"
            font.family: detailWindow.appFontFamily
            font.pixelSize: 13
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
                detailWindow.addonInstallStatus = ""
                detailWindow.addonDependencyTitle = ""
                detailWindow.addonIsInstalled = true
            }
        }

        function onAddonInstallFailed(modId, error) {
            if (modId === detailWindow.addonId) {
                detailWindow.addonIsInstalling = false
                detailWindow.addonInstallStatus = ""
                detailWindow.addonDependencyTitle = ""
                console.error("Install failed:", error)
            }
        }

        function onAddonUninstallFinished(modId) {
            if (modId === detailWindow.addonId) {
                detailWindow.addonIsInstalled = false
            }
        }

        function onAddonInstallStatusChanged(modId, status) {
            if (modId === detailWindow.addonId) {
                detailWindow.addonInstallStatus = status
            }
        }

        function onAddonDependencyInstallStarted(modId, depTitle) {
            if (modId === detailWindow.addonId) {
                detailWindow.addonDependencyTitle = depTitle
                detailWindow.addonInstallPercent = 0
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
