import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    anchors.fill: parent

    // ------------------------------------------------------------------
    // Status model - holds results for each check
    // ------------------------------------------------------------------
    ListModel {
        id: statusModel
        ListElement { name: "safemon-app";     ok: false; output: ""; checked: false }
        ListElement { name: "safemon-display"; ok: false; output: ""; checked: false }
        ListElement { name: "redis";           ok: false; output: ""; checked: false }
        ListElement { name: "vcan";            ok: false; output: ""; checked: false }
    }

    // ------------------------------------------------------------------
    // Backend connections
    // ------------------------------------------------------------------
    Connections {
        target: deviceStatusBackend

        function onCheckStarted() {
            checkBtn.enabled = false
            checkBtn.text = "Checking..."
            logBox.text = ""
            for (var i = 0; i < statusModel.count; i++) {
                statusModel.setProperty(i, "checked", false)
            }
        }

        function onCheckFinished() {
            checkBtn.enabled = true
            checkBtn.text = "Check All"
        }

        function onStatusResult(name, ok, output) {
            for (var i = 0; i < statusModel.count; i++) {
                if (statusModel.get(i).name === name) {
                    statusModel.setProperty(i, "ok", ok)
                    statusModel.setProperty(i, "checked", true)
                }
            }
            logBox.text += "--- " + name + " ---\n"
            logBox.text += output + "\n\n"
        }

        function onConnectionError(message) {
            logBox.text += "ERROR: " + message + "\n"
            checkBtn.enabled = true
            checkBtn.text = "Check All"
        }
    }

    // ------------------------------------------------------------------
    // Layout
    // ------------------------------------------------------------------
    Row {
        anchors.fill: parent
        spacing: 0

        // Left panel
        Rectangle {
            width: 280
            height: parent.height
            color: "#1c1d20"

            Column {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Text {
                    text: "Device Checks"
                    color: "#c3cbdd"
                    font.pixelSize: 14
                    font.bold: true
                }

                Button {
                    id: checkBtn
                    text: "Check All"
                    width: parent.width
                    height: 36
                    onClicked: deviceStatusBackend.checkAll(
                        currentTarget,
                        currentUsername,
                        currentPassword
                    )

                    background: Rectangle {
                        color: checkBtn.pressed ? "#18527e"
                             : checkBtn.hovered ? "#2a7ab8"
                             : checkBtn.enabled ? "#1f6aa5" : "#4a4a4a"
                        radius: 4
                    }

                    contentItem: Text {
                        text: checkBtn.text
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 13
                    }
                }

                // Status indicators
                Repeater {
                    model: statusModel

                    delegate: Row {
                        spacing: 10
                        width: parent.width

                        Rectangle {
                            width: 12
                            height: 12
                            radius: 6
                            anchors.verticalCenter: parent.verticalCenter
                            color: !model.checked ? "#5f6a82"
                                 : model.ok       ? "#4caf50"
                                 :                  "#c62828"
                        }

                        Text {
                            text: model.name
                            color: "#c3cbdd"
                            font.pixelSize: 13
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }

        // Right panel - log output
        Rectangle {
            width: parent.width - 280
            height: parent.height
            color: "#2c313c"

            Column {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8

                Text {
                    text: "Check Results"
                    color: "#5f6a82"
                    font.pixelSize: 13
                }

                Rectangle {
                    width: parent.width
                    height: parent.height - 30
                    color: "#1c1c1c"
                    radius: 4
                    border.color: "#383e4c"

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 8
                        clip: true

                        TextArea {
                            id: logBox
                            readOnly: true
                            wrapMode: TextArea.Wrap
                            color: "#c3cbdd"
                            font.family: "Consolas"
                            font.pixelSize: 12
                            background: Rectangle { color: "transparent" }
                            placeholderText: "Click 'Check All' to run checks..."
                            placeholderTextColor: "#5f6a82"
                        }
                    }
                }
            }
        }
    }
}