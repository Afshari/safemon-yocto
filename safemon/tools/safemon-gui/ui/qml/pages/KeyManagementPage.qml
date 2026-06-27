import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs

Item {
    Layout.fillWidth: true
    Layout.fillHeight: true

    Connections {
        target: keyManagementBackend

        function onKeyGenSuccess(keyPath, pubPath) {
            logBox.text += "OK: Private key -> " + keyPath + "\n"
            logBox.text += "OK: Public key  -> " + pubPath + "\n"
            pubKeyField.text = pubPath
        }

        function onKeyGenFailed(error) {
            logBox.text += "ERROR: " + error + "\n"
        }

        function onScpSuccess(remotePath) {
            logBox.text += "OK: Public key copied to device -> " + remotePath + "\n"
        }

        function onScpFailed(error) {
            logBox.text += "ERROR: " + error + "\n"
        }
    }

    FolderDialog {
        id: folderDialog
        onAccepted: {
            var path = selectedFolder.toString().replace("file:///", "")
            keyDirField.text = path
        }
    }

    FileDialog {
        id: pubKeyDialog
        nameFilters: ["Public key files (*.pub)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            pubKeyField.text = path
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // ------------------------------------------------------------------
        // Top controls
        // ------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 180
            color: "#1c1d20"
            radius: 4

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Text {
                    text: "Key Generation"
                    color: "#c3cbdd"
                    font.pixelSize: 13
                    font.bold: true
                }

                RowLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    TextField {
                        id: keyDirField
                        Layout.fillWidth: true
                        height: 32
                        text: (Qt.platform.os === "windows")
                            ? "C:/Users/" + "root" + "/.safemon"
                            : "~/.safemon"
                        placeholderText: "Key output directory"
                        color: "#c3cbdd"
                        font.pixelSize: 12
                        background: Rectangle { color: "#2c313c"; radius: 4 }
                    }

                    Button {
                        text: "Browse"
                        height: 32
                        width: 80
                        onClicked: folderDialog.open()
                        background: Rectangle {
                            color: parent.pressed ? "#18527e"
                                 : parent.hovered ? "#2a7ab8" : "#1f6aa5"
                            radius: 4
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#fff"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 12
                        }
                    }

                    Button {
                        text: "Generate Key Pair"
                        height: 32
                        width: 140
                        onClicked: keyManagementBackend.generateKeys(keyDirField.text)
                        background: Rectangle {
                            color: parent.pressed ? "#18527e"
                                 : parent.hovered ? "#2a7ab8" : "#1f6aa5"
                            radius: 4
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#fff"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 12
                        }
                    }
                }

                Text {
                    text: "Copy Public Key to Device"
                    color: "#c3cbdd"
                    font.pixelSize: 13
                    font.bold: true
                }

                RowLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    TextField {
                        id: pubKeyField
                        Layout.fillWidth: true
                        height: 32
                        placeholderText: "Public key (.pub) - auto-filled after generate"
                        color: "#c3cbdd"
                        font.pixelSize: 12
                        background: Rectangle { color: "#2c313c"; radius: 4 }
                    }

                    Button {
                        text: "Browse"
                        height: 32
                        width: 80
                        onClicked: pubKeyDialog.open()
                        background: Rectangle {
                            color: parent.pressed ? "#18527e"
                                 : parent.hovered ? "#2a7ab8" : "#1f6aa5"
                            radius: 4
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#fff"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 12
                        }
                    }

                    Button {
                        text: "Copy to Device"
                        height: 32
                        width: 130
                        onClicked: {
                            confirmDialog.open()
                        }
                        background: Rectangle {
                            color: parent.pressed ? "#18527e"
                                 : parent.hovered ? "#2a7ab8" : "#1f6aa5"
                            radius: 4
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#fff"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }

        // ------------------------------------------------------------------
        // Output log
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
                    placeholderText: "Output will appear here..."
                    placeholderTextColor: "#5f6a82"
                }
            }
        }
    }

    // Confirmation dialog
    Dialog {
        id: confirmDialog
        title: "Confirm Copy to Device"
        modal: true
        anchors.centerIn: parent
        width: 380

        background: Rectangle {
            color: "#2c313c"
            border.color: "#383e4c"
            radius: 6
        }

        Text {
            text: "This will overwrite:\n\n/etc/safemon/pki/safemon.pub\n\non " +
                  currentTarget + ".\n\nAre you sure?"
            color: "#c3cbdd"
            font.pixelSize: 12
            wrapMode: Text.Wrap
            width: parent.width
        }

        standardButtons: Dialog.Yes | Dialog.No

        onAccepted: keyManagementBackend.copyPublicKey(
            pubKeyField.text,
            currentTarget,
            currentUsername,
            currentPassword
        )
    }
}