import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.AboutPage {
    objectName: "aboutPikaTalkPage"
    title: i18nc("@title:window", "About PikaTalk")
    aboutData: {
        "displayName": "PikaTalk",
        "productName": "pikatalk",
        "componentName": "pikatalk",
        "shortDescription": i18n("Plasma desktop client for PicoClaw"),
        "homepage": "",
        "bugAddress": "",
        "version": Qt.application.version,
        "otherText": "",
        "authors": [
            {
                "name": "Radilabs",
                "task": "",
                "emailAddress": "",
                "webAddress": "",
                "ocsUsername": ""
            }
        ],
        "credits": [],
        "translators": [],
        "licenses": [],
        "copyrightStatement": i18n("© Radilabs"),
        "desktopFileName": "org.radilabs.pikatalk"
    }
}
