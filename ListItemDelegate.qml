import QtQuick

Rectangle {
    required property string title
    required property string author
    required property string appFontFamily
    required property url iconSource

    width: ListView.view.width
    height: 60
    color: "#2a2a2a"
    radius: 6

    Row {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 12

        Image {
            source: iconSource
            width: 36
            height: 36
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            width: Math.max(0, parent.width - 48)
            text: title + "\nby " + author
            color: "white"
            font.family: appFontFamily
            font.pixelSize: 16
            maximumLineCount: 2
            elide: Text.ElideRight
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
        }
    }
}
