import QtQuick

Rectangle {
    property string title: ""
    property string author: ""
    property string version: ""
    property string appFontFamily: "Segoe UI"
    property url iconSource

    signal addonClicked()

    width: ListView.view.width
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
        onClicked: parent.addonClicked()
    }

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

        Column {
            width: Math.max(0, parent.width - 48)
            anchors.verticalCenter: parent.verticalCenter
            spacing: 1

            Text {
                width: parent.width
                text: title || "No data"
                color: "white"
                font.family: appFontFamily
                font.pixelSize: 16
                font.bold: true
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            Text {
                width: parent.width
                text: version || ""
                color: "#aaaaaa"
                font.family: appFontFamily
                font.pixelSize: 13
                elide: Text.ElideRight
                maximumLineCount: 1
                visible: version !== ""
            }

            Text {
                width: parent.width
                text: "by " + author
                color: "#aaaaaa"
                font.family: appFontFamily
                font.pixelSize: 13
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }
    }
}
