import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: btnToggle
    property url btnIconSource: "../images/svg_images/menu_icon.svg"
    property color btnColorDefault: "#1c1d20"
    property color btnColorMouseOver: "#23272E"
    property color btnColorClicked: "#00a1f1"

    QtObject {
        id: internal
        property var dynamicColor: if(btnToggle.down) {
            btnToggle.down ? btnColorClicked : btnColorDefault
        } else {
            btnToggle.hovered ? btnColorMouseOver : btnColorDefault
        }
    }

    implicitWidth: 70
    implicitHeight: 60

    background: Rectangle {
        color: internal.dynamicColor
        Image {
            id: iconBtn
            source: btnIconSource
            anchors.centerIn: parent
            height: 25
            width: 25
            fillMode: Image.PreserveAspectFit
        }
    }
}