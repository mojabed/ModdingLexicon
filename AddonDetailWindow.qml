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
    property int addonDownloads: 0
    property int addonDownloadsMonthly: 0
    property int addonFavorites: 0
    property bool addonIsInstalled: false
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

        // Header: icon + title
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

                Text {
                    id: descriptionText
                    width: scrollView.width - 20
                    textFormat: Text.RichText
                    color: "#cccccc"
                    linkColor: "#8ab4f8"
                    font.family: detailWindow.appFontFamily
                    font.pixelSize: 15
                    wrapMode: Text.Wrap
                    text: detailWindow.descriptionHtml || "Loading description..."
                    leftPadding: 10
                    rightPadding: 10
                    topPadding: 10

                    onLinkActivated: function(link) {
                        Qt.openUrlExternally(link)
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

    component InfoBadge: Text {
        property string label: ""
        property string badgeColor: "#cccccc"

        text: label
        color: badgeColor
        font.family: detailWindow.appFontFamily
        font.pixelSize: 14
    }
}
