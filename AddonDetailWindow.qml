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

    width: 480
    height: 560
    minimumWidth: 400
    minimumHeight: 400

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

    color: "#1e1e1e"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // Header: icon + title
        RowLayout {
            spacing: 14

            Image {
                source: detailWindow.addonIconSource
                width: 48
                height: 48
                fillMode: Image.PreserveAspectFit
                Layout.alignment: Qt.AlignTop
            }

            ColumnLayout {
                spacing: 2

                Text {
                    text: detailWindow.addonTitle || "Unknown Addon"
                    color: "white"
                    font.family: detailWindow.appFontFamily
                    font.pixelSize: 20
                    font.bold: true
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Text {
                    text: "by " + (detailWindow.addonAuthor || "Unknown")
                    color: "#aaaaaa"
                    font.family: detailWindow.appFontFamily
                    font.pixelSize: 13
                }
            }
        }

        // Divider
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#444"
        }

        // Info grid
        GridLayout {
            columns: 2
            rowSpacing: 10
            columnSpacing: 16

            InfoLabel { label: "Version"; value: detailWindow.addonVersion }
            InfoLabel { label: "Updated"; value: detailWindow.addonLastUpdated }
            InfoLabel { label: "Category"; value: detailWindow.addonCategory }
            InfoLabel { label: "Downloads"; value: detailWindow.addonDownloads }
            InfoLabel {
                label: "Monthly"
                value: detailWindow.addonDownloadsMonthly
            }
            InfoLabel { label: "Favorites"; value: detailWindow.addonFavorites }
            InfoLabel {
                label: "Installed"
                value: detailWindow.addonIsInstalled ? "Yes" : "No"
            }
        }

        // Divider
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#444"
        }

        // Description placeholder
        Text {
            text: "Description"
            color: "white"
            font.family: detailWindow.appFontFamily
            font.pixelSize: 14
            font.bold: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Text {
                text: detailWindow.addonDescription || "No description available."
                color: "#cccccc"
                font.family: detailWindow.appFontFamily
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                width: detailWindow.width - 60
            }
        }
    }

    component InfoLabel: ColumnLayout {
        property string label: ""
        property string value: ""

        spacing: 1

        Text {
            text: label
            color: "#888"
            font.family: detailWindow.appFontFamily
            font.pixelSize: 11
        }

        Text {
            text: value || "-"
            color: "white"
            font.family: detailWindow.appFontFamily
            font.pixelSize: 13
        }
    }
}
