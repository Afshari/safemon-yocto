import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs

Item {
    anchors.fill: parent

    property string signFilePath: ""
    property string signKeyPath: ""
    property string verifyFilePath: ""
    property string verifySigPath: ""
    property string verifyPubPath: ""

    Connections {
        target: signVerifyBackend

        function onSignSuccess(sigPath) {
            logBox.text += "OK: Signature written -> " + sigPath + "\n"
            resultLabel.text = "SIGNED"
            resultLabel.color = "#4caf50"
        }

        function onSignFailed(error) {
            logBox.text += "ERROR: " + error + "\n"
            resultLabel.text = "SIGN FAILED"
            resultLabel.color = "#c62828"
        }

        function onVerifySuccess(isValid) {
            if (isValid) {
                logBox.text += "OK: Signature is VALID.\n"
                resultLabel.text = "VALID"
                resultLabel.color = "#4caf50"
            } else {
                logBox.text += "FAIL: Signature is INVALID.\n"
                resultLabel.text = "INVALID"
                resultLabel.color = "#c62828"
            }
        }

        function onVerifyFailed(error) {
            logBox.text += "ERROR: " + error + "\n"
            resultLabel.text = "VERIFY FAILED"
            resultLabel.color = "#c62828"
        }
    }

    FileDialog {
        id: signFileDialog
        onAccepted: signFilePath = selectedFile.toString().replace("file:///", "")
    }

    FileDialog {
        id: signKeyDialog
        nameFilters: ["Key files (*.key)"]
        onAccepted: signKeyPath = selectedFile.toString().replace("file:///", "")
    }

    FileDialog {
        id: verifyFileDialog
        onAccepted: verifyFilePath = selectedFile.toString().replace("file:///", "")
    }

    FileDialog {
        id: verifySigDialog
        nameFilters: ["Signature files (*.sig)"]
        onAccepted: verifySigPath = selectedFile.toString().replace("file:///", "")
    }

    FileDialog {
        id: verifyPubDialog
        nameFilters: ["Public key files (*.pub)"]
        onAccepted: verifyPubPath = selectedFile.toString().replace("file:///", "")
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
            height: 220
            color: "#1c1d20"
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 16

                // Sign section
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        text: "Sign File"
                        color: "#c3cbdd"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    RowLayout {
                        spacing: 6
                        Layout.fillWidth: true

                        TextField {
                            Layout.fillWidth: true
                            height: 32
                            text: signFilePath
                            placeholderText: "File to sign"
                            color: "#c3cbdd"
                            font.pixelSize: 12
                            readOnly: true
                            background: Rectangle { color: "#2c313c"; radius: 4 }
                        }

                        Button {
                            text: "Browse"
                            height: 32
                            width: 70
                            onClicked: signFileDialog.open()
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

                    RowLayout {
                        spacing: 6
                        Layout.fillWidth: true

                        TextField {
                            Layout.fillWidth: true
                            height: 32
                            text: signKeyPath
                            placeholderText: "Private key (.key)"
                            color: "#c3cbdd"
                            font.pixelSize: 12
                            readOnly: true
                            background: Rectangle { color: "#2c313c"; radius: 4 }
                        }

                        Button {
                            text: "Browse"
                            height: 32
                            width: 70
                            onClicked: signKeyDialog.open()
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

                    Button {
                        text: "Sign"
                        height: 32
                        width: 100
                        onClicked: signVerifyBackend.signFile(signFilePath, signKeyPath)
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

                // Divider
                Rectangle {
                    width: 1
                    Layout.fillHeight: true
                    color: "#383e4c"
                }

                // Verify section
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        text: "Verify File"
                        color: "#c3cbdd"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    RowLayout {
                        spacing: 6
                        Layout.fillWidth: true

                        TextField {
                            Layout.fillWidth: true
                            height: 32
                            text: verifyFilePath
                            placeholderText: "File to verify"
                            color: "#c3cbdd"
                            font.pixelSize: 12
                            readOnly: true
                            background: Rectangle { color: "#2c313c"; radius: 4 }
                        }

                        Button {
                            text: "Browse"
                            height: 32
                            width: 70
                            onClicked: verifyFileDialog.open()
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

                    RowLayout {
                        spacing: 6
                        Layout.fillWidth: true

                        TextField {
                            Layout.fillWidth: true
                            height: 32
                            text: verifySigPath
                            placeholderText: "Signature (.sig)"
                            color: "#c3cbdd"
                            font.pixelSize: 12
                            readOnly: true
                            background: Rectangle { color: "#2c313c"; radius: 4 }
                        }

                        Button {
                            text: "Browse"
                            height: 32
                            width: 70
                            onClicked: verifySigDialog.open()
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

                    RowLayout {
                        spacing: 6
                        Layout.fillWidth: true

                        TextField {
                            Layout.fillWidth: true
                            height: 32
                            text: verifyPubPath
                            placeholderText: "Public key (.pub)"
                            color: "#c3cbdd"
                            font.pixelSize: 12
                            readOnly: true
                            background: Rectangle { color: "#2c313c"; radius: 4 }
                        }

                        Button {
                            text: "Browse"
                            height: 32
                            width: 70
                            onClicked: verifyPubDialog.open()
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

                    RowLayout {
                        spacing: 12

                        Button {
                            text: "Verify"
                            height: 32
                            width: 100
                            onClicked: signVerifyBackend.verifyFile(
                                verifyFilePath, verifySigPath, verifyPubPath
                            )
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

                        Text {
                            id: resultLabel
                            text: ""
                            font.pixelSize: 16
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
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
                    placeholderText: "Sign/verify results will appear here..."
                    placeholderTextColor: "#5f6a82"
                }
            }
        }
    }
}