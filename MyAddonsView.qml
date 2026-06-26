import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Item {
    id: root

    property string appFontFamily: "Segoe UI"
    property var lexiconController

    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    clip: true

    ListView {
        anchors.fill: parent
        anchors.margins: 10
        model: root.lexiconController ? root.lexiconController.installedAddonsFilter : null
        spacing: 5
        clip: true
        reuseItems: true
        cacheBuffer: 500

        delegate: ListItemDelegate {
            appFontFamily: root.appFontFamily
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
