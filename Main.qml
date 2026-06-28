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
    property var activeDetailWindow: null

    function showAddonDetail(data) {
        if (activeDetailWindow !== null) {
            var old = activeDetailWindow
            activeDetailWindow = null
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

    MainTabBar {
        id: bar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        appFontFamily: window.appFontFamily

        onBrowseTabReclicked: {
            browseAddonsView.goBack()
        }
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