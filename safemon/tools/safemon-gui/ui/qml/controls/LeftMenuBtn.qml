import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: btnLeftMenu
    text: qsTr("Menu Item")

    property url btnIconSource: "../images/svg_images/home_icon.svg"
    property color btnColorDefault: "#1c1d20"
    property color btnColorMouseOver: "#23272E"
    property color btnColorClicked: "#00a1f1"
    property int iconWidth: 18
    property int iconHeight: 18
    property color activeMenuColor: "#1f6aa5"
    property color activeMenuColorRight: "#2c313c"
    property bool isActiveMenu: false

    QtObject {
        id: internal
        property var dynamicColor: if(btnLeftMenu.down) {
            btnLeftMenu.down ? btnColorClicked : btnColorDefault
        } else {
            btnLeftMenu.hovered ? btnColorMouseOver : btnColorDefault
        }
    }

    implicitWidth: 250
    implicitHeight: 60

    background: Rectangle {
        color: internal.dynamicColor

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            color: activeMenuColor
            width: 3
            visible: isActiveMenu
        }

        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            color: activeMenuColorRight
            width: 5
            visible: isActiveMenu
        }
    }

    contentItem: Item {
        anchors.fill: parent

        Image {
            id: iconBtn
            source: btnLeftMenu.btnIconSource
            anchors.left: parent.left
            anchors.leftMargin: 26
            anchors.verticalCenter: parent.verticalCenter
            sourceSize.width: iconWidth
            sourceSize.height: iconHeight
            width: iconWidth
            height: iconHeight
            fillMode: Image.PreserveAspectFit
            antialiasing: true
        }

        Text {
            color: "#ffffff"
            text: btnLeftMenu.text
            font: btnLeftMenu.font
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 75
        }
    }
}