import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "controls"
import "pages"

Window {
    id: mainWindow
    width: 1100
    height: 700
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    title: "Safemon"
    color: "#00000000"
    flags: Qt.Window | Qt.FramelessWindowHint

    property int windowStatus: 0
    property int windowMargin: 10
    property int currentPage: 0

    property string currentTarget: "raspberry_pi"
    property string currentUsername: "root"
    property string currentPassword: ""

    QtObject {
        id: internal

        function maximizeRestore() {
            if (windowStatus === 0) {
                mainWindow.showMaximized()
                windowStatus = 1
                windowMargin = 0
                resizeLeft.visible = false
                resizeRight.visible = false
                resizeBottom.visible = false
                btnMaximizeRestore.btnIconSource = "../images/svg_images/restore_icon.svg"
            } else {
                mainWindow.showNormal()
                windowStatus = 0
                windowMargin = 10
                resizeLeft.visible = true
                resizeRight.visible = true
                resizeBottom.visible = true
                btnMaximizeRestore.btnIconSource = "../images/svg_images/maximize_icon.svg"
            }
        }

        function ifMaximizedWindowRestore() {
            if (windowStatus === 1) {
                mainWindow.showNormal()
                windowStatus = 0
                windowMargin = 10
                resizeLeft.visible = true
                resizeRight.visible = true
                resizeBottom.visible = true
                btnMaximizeRestore.btnIconSource = "../images/svg_images/maximize_icon.svg"
            }
        }
    }

    Rectangle {
        id: bg
        color: "#2c313c"
        border.color: "#383e4c"
        border.width: 1
        anchors.fill: parent
        anchors.margins: windowMargin

        // ------------------------------------------------------------------
        // Top bar
        // ------------------------------------------------------------------
        Rectangle {
            id: topBar
            height: 60
            color: "#1c1d20"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            ToggleButton {
                id: toggleBtn
                anchors.left: parent.left
                anchors.top: parent.top
                onClicked: animationMenu.running = true
            }

            DragHandler {
                onActiveChanged: if (active) {
                    mainWindow.startSystemMove()
                    internal.ifMaximizedWindowRestore()
                }
            }

            Image {
                id: iconApp
                width: 22
                height: 22
                anchors.left: toggleBtn.right
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -8
                source: "images/svg_images/icon_app_top.svg"
                fillMode: Image.PreserveAspectFit
                visible: false
            }

            Text {
                anchors.left: iconApp.right
                anchors.leftMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -8
                text: "Safemon"
                color: "#c3cbdd"
                font.pixelSize: 13
            }

            // Page title bar
            Rectangle {
                height: 25
                color: "#282c34"
                anchors.left: toggleBtn.right
                anchors.right: parent.right
                anchors.bottom: parent.bottom

                Row {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 10
                    spacing: 8

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#5f6a82"
                        font.pixelSize: 11
                        text: ["Key Management", "Sign / Verify", "Fault Monitor",
                            "Device Files", "Device Status"][currentPage]
                    }
                }

                Row {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 10
                    spacing: 8

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#5f6a82"
                        font.pixelSize: 11
                        text: "Target:"
                    }

                    ComboBox {
                        id: targetCombo
                        height: 20
                        width: 140
                        model: ["raspberry_pi", "jetson_orin_nano", "qemu"]
                        onCurrentValueChanged: currentTarget = currentValue
                        background: Rectangle {
                            color: "#3c3c3c"
                            radius: 3
                        }
                        contentItem: Text {
                            leftPadding: 6
                            text: targetCombo.displayText
                            color: "#c3cbdd"
                            font.pixelSize: 11
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#5f6a82"
                        font.pixelSize: 11
                        text: "User:"
                    }

                    TextField {
                        id: usernameField
                        height: 20
                        width: 70
                        text: "root"
                        color: "#c3cbdd"
                        font.pixelSize: 11
                        onTextChanged: currentUsername = text
                        background: Rectangle {
                            color: "#3c3c3c"
                            radius: 3
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#5f6a82"
                        font.pixelSize: 11
                        text: "Pass:"
                    }

                    TextField {
                        id: passwordField
                        height: 20
                        width: 80
                        echoMode: TextInput.Password
                        placeholderText: "empty = none"
                        color: "#c3cbdd"
                        font.pixelSize: 11
                        onTextChanged: currentPassword = text
                        background: Rectangle {
                            color: "#3c3c3c"
                            radius: 3
                        }
                    }
                }
            }

            // Window control buttons
            Row {
                anchors.right: parent.right
                anchors.top: parent.top
                height: 35

                TopBarButton {
                    id: btnMinimize
                    btnIconSource: "../images/svg_images/minimize_icon.svg"
                    onClicked: mainWindow.showMinimized()
                }

                TopBarButton {
                    id: btnMaximizeRestore
                    btnIconSource: "../images/svg_images/maximize_icon.svg"
                    onClicked: internal.maximizeRestore()
                }

                TopBarButton {
                    id: btnClose
                    btnColorClicked: "#c0392b"
                    btnIconSource: "../images/svg_images/close_icon.svg"
                    onClicked: mainWindow.close()
                }
            }
        }

        // ------------------------------------------------------------------
        // Content area
        // ------------------------------------------------------------------
        Rectangle {
            id: contentArea
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: topBar.bottom
            anchors.bottom: statusBar.top
            color: "transparent"

            // Sidebar
            Rectangle {
                id: leftMenu
                width: 70
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                color: "#1c1d20"
                clip: true

                PropertyAnimation {
                    id: animationMenu
                    target: leftMenu
                    property: "width"
                    to: leftMenu.width === 70 ? 250 : 70
                    duration: 500
                    easing.type: Easing.InOutQuint
                }

                Column {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    clip: true

                    LeftMenuBtn {
                        id: btnKeys
                        text: "Key Management"
                        btnIconSource: "../images/svg_images/home_icon.svg"
                        isActiveMenu: currentPage === 0
                        onClicked: currentPage = 0
                    }

                    LeftMenuBtn {
                        id: btnSign
                        text: "Sign / Verify"
                        btnIconSource: "../images/svg_images/save_icon.svg"
                        isActiveMenu: currentPage === 1
                        onClicked: currentPage = 1
                    }

                    LeftMenuBtn {
                        id: btnFaults
                        text: "Fault Monitor"
                        btnIconSource: "../images/svg_images/open_icon.svg"
                        isActiveMenu: currentPage === 2
                        onClicked: currentPage = 2
                    }

                    LeftMenuBtn {
                        id: btnFiles
                        text: "Device Files"
                        btnIconSource: "../images/svg_images/save_icon.svg"
                        isActiveMenu: currentPage === 3
                        onClicked: currentPage = 3
                    }

                    LeftMenuBtn {
                        id: btnStatus
                        text: "Device Status"
                        btnIconSource: "../images/svg_images/settings_icon.svg"
                        isActiveMenu: currentPage === 4
                        onClicked: currentPage = 4
                    }
                }
            }

            // Page area
            Rectangle {
                id: pageArea
                anchors.left: leftMenu.right
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                color: "#2c313c"

                StackLayout {
                    anchors.fill: parent
                    currentIndex: currentPage

                    KeyManagementPage { }
                    SignVerifyPage { }
                    FaultMonitorPage { }
                    DeviceFilesPage { }
                    DeviceStatusPage { }
                }
            }
        }

        // ------------------------------------------------------------------
        // Status bar
        // ------------------------------------------------------------------
        Rectangle {
            id: statusBar
            height: 28
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            color: "#1c1d20"

            Text {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 16
                text: "Safemon GUI"
                color: "#5f6a82"
                font.pixelSize: 11
            }

            Image {
                id: resizeImg
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 4
                width: 16
                height: 16
                source: "images/svg_images/resize_icon.svg"
                fillMode: Image.PreserveAspectFit
                visible: false
            }

            DragHandler {
                target: null
                onActiveChanged: if (active)
                    mainWindow.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
            }
        }
    }

    // Resize edges
    MouseArea {
        id: resizeLeft
        width: 10
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.topMargin: 10
        cursorShape: Qt.SizeHorCursor
        DragHandler {
            target: null
            onActiveChanged: if (active) mainWindow.startSystemResize(Qt.LeftEdge)
        }
    }

    MouseArea {
        id: resizeRight
        width: 10
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.topMargin: 10
        cursorShape: Qt.SizeHorCursor
        DragHandler {
            target: null
            onActiveChanged: if (active) mainWindow.startSystemResize(Qt.RightEdge)
        }
    }

    MouseArea {
        id: resizeBottom
        height: 10
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 10
        anchors.leftMargin: 10
        cursorShape: Qt.SizeVerCursor
        DragHandler {
            target: null
            onActiveChanged: if (active) mainWindow.startSystemResize(Qt.BottomEdge)
        }
    }
}