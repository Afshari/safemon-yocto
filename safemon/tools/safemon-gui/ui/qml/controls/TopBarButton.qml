import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: btnTopBar
    property url btnIconSource: "../images/svg_images/minimize_icon.svg"
    property color btnColorDefault: "#1c1d20"
    property color btnColorMouseOver: "#23272E"
    property color btnColorClicked: "#00a1f1"

    QtObject {
        id: internal
        property var dynamicColor: if(btnTopBar.down) {
            btnTopBar.down ? btnColorClicked : btnColorDefault
        } else {
            btnTopBar.hovered ? btnColorMouseOver : btnColorDefault
        }
    }

    width: 35
    height: 35

    background: Rectangle {
        color: internal.dynamicColor
        Image {
            id: iconBtn
            source: btnIconSource
            anchors.centerIn: parent
            height: 16
            width: 16
            fillMode: Image.PreserveAspectFit
            antialiasing: false
        }
    }
}