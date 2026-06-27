import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs

Item {
    Layout.fillWidth: true
    Layout.fillHeight: true

    Component.onCompleted: settingsBackend.loadAll()

    Connections {
        target: settingsBackend

        function onConfigLoaded(key, jsonData) {
            var data = JSON.parse(jsonData)
            if (key === "raspberry_pi") {
                rpiHost.text  = data.host  || ""
                rpiPort.text  = String(data.port  || 22)
                rpiUser.text  = data.user  || "root"
            } else if (key === "jetson_orin_nano") {
                jetsonHost.text = data.host || ""
                jetsonPort.text = String(data.port || 22)
                jetsonUser.text = data.user || "root"
            } else if (key === "qemu") {
                qemuHost.text = data.host || ""
                qemuPort.text = String(data.port || 2222)
                qemuUser.text = data.user || "root"
            } else if (key === "app") {
                journalLines.text   = String(data.journal_lines || 50)
                journalRefresh.text = String(data.journal_refresh_seconds || 10)
                keyDir.text         = data.key_dir || ""
            }
        }

        function onSaveSuccess(message) {
            logBox.text += "OK: " + message + "\n"
            // Reload top bar target info
            settingsBackend.loadAll()
        }

        function onSaveFailed(message) {
            logBox.text += "ERROR: " + message + "\n"
        }
    }

    FolderDialog {
        id: keyDirDialog
        onAccepted: keyDir.text = selectedFolder.toString().replace("file:///", "")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // ------------------------------------------------------------------
        // Platform configs
        // ------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 220
            color: "#1c1d20"
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                // Raspberry Pi
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "Raspberry Pi"
                        color: "#c3cbdd"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    Text { text: "Host:"; color: "#5f6a82"; font.pixelSize: 11 }
                    TextField {
                        id: rpiHost
                        Layout.fillWidth: true
                        height: 30
                        placeholderText: "e.g. 192.168.1.42"
                        color: "#c3cbdd"
                        font.pixelSize: 12
                        background: Rectangle { color: "#2c313c"; radius: 4 }
                    }

                    Text { text: "Port:"; color: "#5f6a82"; font.pixelSize: 11 }
                    TextField {
                        id: rpiPort
                        Layout.fillWidth: true
                        height: 30
                        color: "#c3cbdd"
                        font.pixelSize: 12
                        background: Rectangle { color: "#2c313c"; radius: 4 }
                    }

                    Text { text: "User:"; color: "#5f6a82"; font.pixelSize: 11 }
                    TextField {
                        id: rpiUser
                        Layout.fillWidth: true
                        height: 30
                        color: "#c3cbdd"
                        font.pixelSize: 12
                        background: Rectangle { color: "#2c313c"; radius: 4 }
                    }

                    RowLayout {
                        spacing: 6
                        Button {
                            text: "Save"
                            height: 28
                            Layout.fillWidth: true
                            onClicked: settingsBackend.savePlatform(
                                "raspberry_pi", rpiHost.text,
                                parseInt(rpiPort.text), rpiUser.text
                            )
                            background: Rectangle {
                                color: parent.pressed ? "#18527e"
                                     : parent.hovered ? "#2a7ab8" : "#1f6aa5"
                                radius: 4
                            }
                            contentItem: Text {
                                text: parent.text; color: "#fff"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 12
                            }
                        }
                        Button {
                            text: "Reset"
                            height: 28
                            Layout.fillWidth: true
                            onClicked: settingsBackend.resetPlatform("raspberry_pi")
                            background: Rectangle {
                                color: parent.pressed ? "#333"
                                     : parent.hovered ? "#4a4a4a" : "#3c3c3c"
                                radius: 4
                            }
                            contentItem: Text {
                                text: parent.text; color: "#fff"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 12
                            }
                        }
                    }
                }

                Rectangle { width: 1; Layout.fillHeight: true; color: "#383e4c" }

                // Jetson Orin Nano
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "Jetson Orin Nano"
                        color: "#c3cbdd"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    Text { text: "Host:"; color: "#5f6a82"; font.pixelSize: 11 }
                    TextField {
                        id: jetsonHost
                        Layout.fillWidth: true
                        height: 30
                        placeholderText: "e.g. jetson-orin-nano-devkit.local"
                        color: "#c3cbdd"
                        font.pixelSize: 12
                        background: Rectangle { color: "#2c313c"; radius: 4 }
                    }

                    Text { text: "Port:"; color: "#5f6a82"; font.pixelSize: 11 }
                    TextField {
                        id: jetsonPort
                        Layout.fillWidth: true
                        height: 30
                        color: "#c3cbdd"
                        font.pixelSize: 12
                        background: Rectangle { color: "#2c313c"; radius: 4 }
                    }

                    Text { text: "User:"; color: "#5f6a82"; font.pixelSize: 11 }
                    TextField {
                        id: jetsonUser
                        Layout.fillWidth: true
                        height: 30
                        color: "#c3cbdd"
                        font.pixelSize: 12
                        background: Rectangle { color: "#2c313c"; radius: 4 }
                    }

                    RowLayout {
                        spacing: 6
                        Button {
                            text: "Save"
                            height: 28
                            Layout.fillWidth: true
                            onClicked: settingsBackend.savePlatform(
                                "jetson_orin_nano", jetsonHost.text,
                                parseInt(jetsonPort.text), jetsonUser.text
                            )
                            background: Rectangle {
                                color: parent.pressed ? "#18527e"
                                     : parent.hovered ? "#2a7ab8" : "#1f6aa5"
                                radius: 4
                            }
                            contentItem: Text {
                                text: parent.text; color: "#fff"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 12
                            }
                        }
                        Button {
                            text: "Reset"
                            height: 28
                            Layout.fillWidth: true
                            onClicked: settingsBackend.resetPlatform("jetson_orin_nano")
                            background: Rectangle {
                                color: parent.pressed ? "#333"
                                     : parent.hovered ? "#4a4a4a" : "#3c3c3c"
                                radius: 4
                            }
                            contentItem: Text {
                                text: parent.text; color: "#fff"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 12
                            }
                        }
                    }
                }

                Rectangle { width: 1; Layout.fillHeight: true; color: "#383e4c" }

                // QEMU
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "QEMU"
                        color: "#c3cbdd"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    Text { text: "Host:"; color: "#5f6a82"; font.pixelSize: 11 }
                    TextField {
                        id: qemuHost
                        Layout.fillWidth: true
                        height: 30
                        placeholderText: "e.g. 172.19.7.64"
                        color: "#c3cbdd"
                        font.pixelSize: 12
                        background: Rectangle { color: "#2c313c"; radius: 4 }
                    }

                    Text { text: "Port:"; color: "#5f6a82"; font.pixelSize: 11 }
                    TextField {
                        id: qemuPort
                        Layout.fillWidth: true
                        height: 30
                        color: "#c3cbdd"
                        font.pixelSize: 12
                        background: Rectangle { color: "#2c313c"; radius: 4 }
                    }

                    Text { text: "User:"; color: "#5f6a82"; font.pixelSize: 11 }
                    TextField {
                        id: qemuUser
                        Layout.fillWidth: true
                        height: 30
                        color: "#c3cbdd"
                        font.pixelSize: 12
                        background: Rectangle { color: "#2c313c"; radius: 4 }
                    }

                    RowLayout {
                        spacing: 6
                        Button {
                            text: "Save"
                            height: 28
                            Layout.fillWidth: true
                            onClicked: settingsBackend.savePlatform(
                                "qemu", qemuHost.text,
                                parseInt(qemuPort.text), qemuUser.text
                            )
                            background: Rectangle {
                                color: parent.pressed ? "#18527e"
                                     : parent.hovered ? "#2a7ab8" : "#1f6aa5"
                                radius: 4
                            }
                            contentItem: Text {
                                text: parent.text; color: "#fff"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 12
                            }
                        }
                        Button {
                            text: "Reset"
                            height: 28
                            Layout.fillWidth: true
                            onClicked: settingsBackend.resetPlatform("qemu")
                            background: Rectangle {
                                color: parent.pressed ? "#333"
                                     : parent.hovered ? "#4a4a4a" : "#3c3c3c"
                                radius: 4
                            }
                            contentItem: Text {
                                text: parent.text; color: "#fff"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 12
                            }
                        }
                    }
                }
            }
        }

        // ------------------------------------------------------------------
        // App config
        // ------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 140
            color: "#1c1d20"
            radius: 4

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Text {
                    text: "App Settings"
                    color: "#c3cbdd"
                    font.pixelSize: 13
                    font.bold: true
                }

                RowLayout {
                    spacing: 16
                    Layout.fillWidth: true

                    ColumnLayout {
                        spacing: 4
                        Text { text: "Journal Lines:"; color: "#5f6a82"; font.pixelSize: 11 }
                        TextField {
                            id: journalLines
                            width: 80
                            height: 30
                            color: "#c3cbdd"
                            font.pixelSize: 12
                            background: Rectangle { color: "#2c313c"; radius: 4 }
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        Text { text: "Refresh Seconds:"; color: "#5f6a82"; font.pixelSize: 11 }
                        TextField {
                            id: journalRefresh
                            width: 80
                            height: 30
                            color: "#c3cbdd"
                            font.pixelSize: 12
                            background: Rectangle { color: "#2c313c"; radius: 4 }
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        Layout.fillWidth: true
                        Text { text: "Key Directory:"; color: "#5f6a82"; font.pixelSize: 11 }
                        RowLayout {
                            spacing: 6
                            Layout.fillWidth: true
                            TextField {
                                id: keyDir
                                Layout.fillWidth: true
                                height: 30
                                color: "#c3cbdd"
                                font.pixelSize: 12
                                background: Rectangle { color: "#2c313c"; radius: 4 }
                            }
                            Button {
                                text: "Browse"
                                height: 30
                                width: 70
                                onClicked: keyDirDialog.open()
                                background: Rectangle {
                                    color: parent.pressed ? "#18527e"
                                         : parent.hovered ? "#2a7ab8" : "#1f6aa5"
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text; color: "#fff"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 12
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        Text { text: " "; font.pixelSize: 11 }
                        RowLayout {
                            spacing: 6
                            Button {
                                text: "Save"
                                height: 30
                                width: 70
                                onClicked: settingsBackend.saveApp(
                                    parseInt(journalLines.text),
                                    parseInt(journalRefresh.text),
                                    keyDir.text
                                )
                                background: Rectangle {
                                    color: parent.pressed ? "#18527e"
                                         : parent.hovered ? "#2a7ab8" : "#1f6aa5"
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text; color: "#fff"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 12
                                }
                            }
                            Button {
                                text: "Reset"
                                height: 30
                                width: 70
                                onClicked: settingsBackend.resetApp()
                                background: Rectangle {
                                    color: parent.pressed ? "#333"
                                         : parent.hovered ? "#4a4a4a" : "#3c3c3c"
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text; color: "#fff"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 12
                                }
                            }
                        }
                    }
                }
            }
        }

        // ------------------------------------------------------------------
        // Log
        // ------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
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
                    placeholderText: "Save results will appear here..."
                    placeholderTextColor: "#5f6a82"
                }
            }
        }
    }
}