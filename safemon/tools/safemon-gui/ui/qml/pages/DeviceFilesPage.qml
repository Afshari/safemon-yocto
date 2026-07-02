import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs

Item {
    Layout.fillWidth: true
    Layout.fillHeight: true

    property var knownFileNames: []
    property string selectedLocalPath: ""
    property string transferLocalPath: ""

    Component.onCompleted: deviceFilesBackend.loadKnownFiles()

    Connections {
        target: deviceFilesBackend

        function onKnownFilesLoaded(names) {
            knownFileNames = names
            knownFileCombo.model = names
        }

        function onFileLoaded(name, content) {
            previewBox.text = content
            logBox.text += "OK: Loaded " + name + " from device.\n"
        }

        function onCopySuccess(remotePath) {
            logBox.text += "OK: File copied -> " + remotePath + "\n"
        }

        function onCopyFailed(error) {
            logBox.text += "ERROR: " + error + "\n"
        }

        function onLoadFailed(error) {
            logBox.text += "ERROR: " + error + "\n"
        }
    }

    FileDialog {
        id: knownFileLocalDialog
        onAccepted: {
            var path = selectedFile.toString()
            selectedLocalPath = Qt.platform.os === "windows"
                ? path.replace("file:///", "")
                : path.replace("file://", "")
            logBox.text += "Selected local file: " + selectedLocalPath + "\n"
            deviceFilesBackend.loadKnownFiles()
        }
    }

    FileDialog {
        id: transferFileDialog
        onAccepted: {
            var path = selectedFile.toString()
            transferLocalPath = Qt.platform.os === "windows"
                ? path.replace("file:///", "")
                : path.replace("file://", "")
            logBox.text += "Selected for transfer: " + transferLocalPath + "\n"
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
            height: 230
            color: "#1c1d20"
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 0

                // Known Files section
                ColumnLayout {
                    Layout.preferredWidth: parent.width / 2 - 8
                    Layout.fillWidth: false
                    Layout.rightMargin: 8

                    Text {
                        text: "Known Files"
                        color: "#c3cbdd"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    ComboBox {
                        id: knownFileCombo
                        Layout.fillWidth: true
                        height: 32
                        model: knownFileNames
                        background: Rectangle {
                            color: "#2c313c"
                            radius: 4
                            border.color: "#383e4c"
                        }
                        contentItem: Text {
                            leftPadding: 8
                            text: knownFileCombo.displayText
                            color: "#c3cbdd"
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Text {
                        text: "Device path: " + (knownFileCombo.currentText !== "" && deviceFilesBackend
                            ? (deviceFilesBackend.devicePathForFile(knownFileCombo.currentText) || "-")
                            : "-")
                        color: "#c3cbdd"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: "Local: " + (selectedLocalPath !== "" ? selectedLocalPath : "-")
                        color: "#c3cbdd"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        spacing: 6

                        Button {
                            text: "Browse Local"
                            height: 32
                            onClicked: knownFileLocalDialog.open()
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
                            text: "Load from Device"
                            height: 32
                            onClicked: deviceFilesBackend.loadFromDevice(
                                knownFileCombo.currentText,
                                currentTarget, currentUsername, currentPassword
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

                        Button {
                            text: "Copy to Device"
                            height: 32
                            onClicked: knownFilesConfirmDialog.open()
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

                Rectangle {
                    width: 1
                    Layout.fillHeight: true
                    color: "#383e4c"
                }

                // File Transfer section
                ColumnLayout {
                    Layout.preferredWidth: parent.width / 2 - 8
                    Layout.fillWidth: false
                    Layout.leftMargin: 12

                    Text {
                        text: "File Transfer"
                        color: "#c3cbdd"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    Text {
                        text: "Local: " + (transferLocalPath !== "" ? transferLocalPath : "-")
                        color: "#c3cbdd"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Button {
                        text: "Browse Local File"
                        height: 32
                        onClicked: transferFileDialog.open()
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

                    TextField {
                        id: destField
                        Layout.fillWidth: true
                        height: 32
                        placeholderText: "Destination on device (e.g. /home/root/)"
                        placeholderTextColor: "#5f6a82"
                        color: "#c3cbdd"
                        font.pixelSize: 12
                        background: Rectangle { color: "#2c313c"; radius: 4 }
                    }

                    Button {
                        text: "Copy to Device"
                        height: 32
                        onClicked: transferConfirmDialog.open()
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
        // Preview box
        // ------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 120
            color: "#1c1c1c"
            radius: 4
            border.color: "#383e4c"

            ScrollView {
                anchors.fill: parent
                anchors.margins: 8
                clip: true

                TextArea {
                    id: previewBox
                    readOnly: true
                    wrapMode: TextArea.Wrap
                    color: "#c3cbdd"
                    font.family: "Consolas"
                    font.pixelSize: 12
                    background: Rectangle { color: "transparent" }
                    placeholderText: "File preview (read-only)..."
                    placeholderTextColor: "#5f6a82"
                }
            }
        }

        // ------------------------------------------------------------------
        // Log box
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
                    placeholderText: "Status messages will appear here..."
                    placeholderTextColor: "#5f6a82"
                }
            }
        }
    }

    // Confirmation dialogs
    Dialog {
        id: knownFilesConfirmDialog
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
            text: "Overwrite " +
                (deviceFilesBackend ? deviceFilesBackend.devicePathForFile(knownFileCombo.currentText) : "") +
                " on " + currentTarget + "?"
            color: "#c3cbdd"
            font.pixelSize: 12
            wrapMode: Text.Wrap
            width: parent.width
        }
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: deviceFilesBackend.copyKnownToDevice(
            knownFileCombo.currentText,
            selectedLocalPath,
            currentTarget, currentUsername, currentPassword
        )
    }

    Dialog {
        id: transferConfirmDialog
        title: "Confirm File Transfer"
        modal: true
        anchors.centerIn: parent
        width: 380
        background: Rectangle {
            color: "#2c313c"
            border.color: "#383e4c"
            radius: 6
        }
        Text {
            text: "Copy " + transferLocalPath + "\nto " +
                  currentTarget + ":" + destField.text + "?"
            color: "#c3cbdd"
            font.pixelSize: 12
            wrapMode: Text.Wrap
            width: parent.width
        }
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: deviceFilesBackend.copyFileToDevice(
            transferLocalPath, destField.text,
            currentTarget, currentUsername, currentPassword
        )
    }
}