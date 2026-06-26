import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string appFontFamily: "Segoe UI"

    Text {
        anchors.centerIn: parent
        text: "Settings Page"
        color: "white"
        font.family: root.appFontFamily
        font.pixelSize: 16
    }
}
