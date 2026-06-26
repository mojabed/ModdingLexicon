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

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

    property string appFontFamily: "Segoe UI"
    property string selectedCategoryId: ""
    property string selectedCategoryName: ""
    property bool viewingCategoryAddons: false

    Lexicon {
        id: lexicon
    }

    MainTabBar {
        id: bar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        appFontFamily: window.appFontFamily
    }

    SwipeView {
        id: swipeView
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        currentIndex: bar.currentIndex

        interactive: false

        onCurrentIndexChanged: {
            console.log("Tab switched to index:", currentIndex)
            // Reset category view when leaving Browse Addons tab
            if (currentIndex !== 1) {
                window.viewingCategoryAddons = false
                lexicon.installedAddonsFilter.setCategoryFilter("")
            }
        }

        MyAddonsView {
            width: swipeView.width
            height: swipeView.height
            appFontFamily: window.appFontFamily
            lexiconController: lexicon
        }

        BrowseAddonsView {
            width: swipeView.width
            height: swipeView.height
            appFontFamily: window.appFontFamily
            appWindow: window
            lexiconController: lexicon
        }

        SettingsView {
            width: swipeView.width
            height: swipeView.height
            appFontFamily: window.appFontFamily
        }
    }
}