import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import ModdingLexicon   

ApplicationWindow {
    id: window
    width: 1280
    height: 720
    visible: true
    title: qsTr("Modding Lexicon")
    minimumWidth: 640
    minimumHeight: 480
    flags: Qt.FramelessWindowHint | Qt.Window
    color: "#1e1e1e"

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

    property string appFontFamily: "Segoe UI"
    property string selectedCategoryId: ""
    property string selectedCategoryName: ""
    property bool viewingCategoryAddons: false
    property var activeDetailWindow: null

    // Custom title bar height
    readonly property int titleBarHeight: 32

    function showAddonDetail(data) {
        if (activeDetailWindow !== null) {
            var old = activeDetailWindow
            activeDetailWindow = null
            lexicon.fetchAddonDescription("")  // cancel old image downloads
            old.destroy()
        }

        var comp = Qt.createComponent("AddonDetailWindow.qml")
        if (comp.status === Component.Error) {
            console.error("Failed to create AddonDetailWindow:", comp.errorString())
            return
        }
        var detailWin = comp.createObject(window, {
            "title": qsTr("Addon Details"),
            "addonTitle": data.title || "",
            "addonAuthor": data.author || "",
            "addonVersion": data.version || "",
            "addonLastUpdated": data.formattedDate || data.lastUpdated || "",
            "addonCategory": data.categoryName || "",
            "addonDescription": data.description || "",
            "addonFileInfoUri": data.fileInfoUri || "",
            "addonDownloads": data.downloads || 0,
            "addonDownloadsMonthly": data.downloadsMonthly || 0,
            "addonFavorites": data.favorites || 0,
            "addonIsInstalled": data.isInstalled || false,
            "addonIconSource": data.iconSource || "",
            "lexiconController": lexicon
        })

        if (data.fileInfoUri) {
            lexicon.fetchAddonDescription(data.fileInfoUri)
        }

        detailWin.closing.connect(function() {
            if (activeDetailWindow === detailWin) {
                activeDetailWindow = null
                detailWin.destroy()
            }
        })

        activeDetailWindow = detailWin
        detailWin.show()
    }

    Lexicon {
        id: lexicon
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 4
        anchors.bottomMargin: 4
        spacing: 1

        Rectangle {
            id: titleBar
            Layout.fillWidth: true
            Layout.preferredHeight: window.titleBarHeight
            color: "#141414"

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onPressed: window.startSystemMove()
                onDoubleClicked: window.visibility === Window.Maximized
                    ? window.showNormal() : window.showMaximized()
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 4
                spacing: 0

                Image {
                    source: "qrc:/icons/app_icon.png"
                    width: 16
                    height: 16
                    fillMode: Image.PreserveAspectFit
                }

                Text {
                    text: window.title
                    color: "#cccccc"
                    font.family: window.appFontFamily
                    font.pixelSize: 12
                    leftPadding: 8
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                // Minimize
                TitleBarButton {
                    text: "\u2212"
                    onClicked: window.showMinimized()
                }

                // Maximize
                TitleBarButton {
                    text: window.visibility === Window.Maximized ? "\u29C9" : "\u25A1"
                    onClicked: window.visibility === Window.Maximized
                        ? window.showNormal() : window.showMaximized()
                }

                // Close
                TitleBarButton {
                    text: "\u00D7"
                    isClose: true
                    onClicked: window.close()
                }
            }
        }

        MainTabBar {
            id: bar
            Layout.fillWidth: true
            appFontFamily: window.appFontFamily

            onBrowseTabReclicked: {
                browseAddonsView.goBack()
            }
        }

    SwipeView {
        id: swipeView
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: bar.currentIndex

        interactive: false

        onCurrentIndexChanged: {
            console.log("Tab switched to index:", currentIndex)
            if (currentIndex !== 1) {
                window.viewingCategoryAddons = false
                lexicon.installedAddonsFilter.setCategoryFilter("")
            }
            lexicon.installedAddonsFilter.setSearchText("")
        }

        MyAddonsView {
            id: myAddonsView
            width: swipeView.width
            height: swipeView.height
            appFontFamily: window.appFontFamily
            lexiconController: lexicon

            onAddonDetailRequested: function(data) {
                window.showAddonDetail(data)
            }
        }

        BrowseAddonsView {
            id: browseAddonsView
            width: swipeView.width
            height: swipeView.height
            appFontFamily: window.appFontFamily
            appWindow: window
            lexiconController: lexicon

            onAddonDetailRequested: function(data) {
                window.showAddonDetail(data)
            }
        }

        SettingsView {
            width: swipeView.width
            height: swipeView.height
            appFontFamily: window.appFontFamily
        }
    }
    }

    // Window resize handles
    MouseArea {
        id: resizeLeft
        anchors.top: parent.top; anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 4; cursorShape: Qt.SizeHorCursor
        onPressed: window.startSystemResize(Qt.LeftEdge)
    }
    MouseArea {
        id: resizeRight
        anchors.top: parent.top; anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 4; cursorShape: Qt.SizeHorCursor
        onPressed: window.startSystemResize(Qt.RightEdge)
    }
    MouseArea {
        id: resizeBottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left; anchors.right: parent.right
        height: 4; cursorShape: Qt.SizeVerCursor
        onPressed: window.startSystemResize(Qt.BottomEdge)
    }
    MouseArea {
        id: resizeTopLeft
        anchors.top: parent.top; anchors.left: parent.left
        width: 8; height: 8; cursorShape: Qt.SizeFDiagCursor
        onPressed: window.startSystemResize(Qt.TopEdge | Qt.LeftEdge)
    }
    MouseArea {
        id: resizeTopRight
        anchors.top: parent.top; anchors.right: parent.right
        width: 8; height: 8; cursorShape: Qt.SizeBDiagCursor
        onPressed: window.startSystemResize(Qt.TopEdge | Qt.RightEdge)
    }
    MouseArea {
        id: resizeBottomLeft
        anchors.bottom: parent.bottom; anchors.left: parent.left
        width: 8; height: 8; cursorShape: Qt.SizeBDiagCursor
        onPressed: window.startSystemResize(Qt.BottomEdge | Qt.LeftEdge)
    }
    MouseArea {
        id: resizeBottomRight
        anchors.bottom: parent.bottom; anchors.right: parent.right
        width: 8; height: 8; cursorShape: Qt.SizeFDiagCursor
        onPressed: window.startSystemResize(Qt.BottomEdge | Qt.RightEdge)
    }

    component TitleBarButton: Rectangle {
        property string text: ""
        property bool isClose: false

        width: 46
        height: window.titleBarHeight
        color: mouseArea.containsMouse
            ? (isClose ? "#c42b1c" : "#383838")
            : "transparent"

        Text {
            anchors.centerIn: parent
            text: parent.text
            color: mouseArea.containsMouse || parent.isClose ? "white" : "#cccccc"
            font.pixelSize: 14
            font.family: window.appFontFamily
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: parent.onClicked()
        }

        signal clicked()
    }
}