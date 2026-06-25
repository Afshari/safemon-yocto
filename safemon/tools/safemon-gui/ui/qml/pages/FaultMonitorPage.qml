import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    anchors.fill: parent

    property bool streaming: false

    Connections {
        target: faultMonitorBackend

        function onConnected() {
            streaming = true
            connectBtn.text = "Disconnect"
            statusLabel.text = "Connected"
            statusLabel.color = "#4caf50"
        }

        function onDisconnected() {
            streaming = false
            connectBtn.text = "Connect"
            statusLabel.text = "Disconnected"
            statusLabel.color = "#5f6a82"
        }

        function onConnectionError(message) {
            streaming = false
            connectBtn.text = "Connect"
            statusLabel.text = "Error"
            statusLabel.color = "#c62828"
            logAppend("ERROR: " + message)
        }

        function onEventReceived(timestamp, status, source, signature) {
            var color = "#c3cbdd"
            if (status.startsWith("OK"))    color = "#4caf50"
            if (status.startsWith("WARN"))  color = "#ffcc80"
            if (status.startsWith("ERROR")) color = "#ef9a9a"

            eventModel.append({
                "timestamp": timestamp,
                "status":    status,
                "source":    source,
                "signature": signature,
                "rowColor":  color
            })

            eventView.positionViewAtEnd()
        }
    }

    function logAppend(msg) {
        logBox.text += msg + "\n"
    }

    ListModel {
        id: eventModel
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
            height: 80
            color: "#1c1d20"
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                Text {
                    text: "Host:"
                    color: "#5f6a82"
                    font.pixelSize: 12
                }

                TextField {
                    id: hostField
                    width: 200
                    height: 32
                    placeholderText: "IP or hostname"
                    color: "#c3cbdd"
                    font.pixelSize: 12
                    background: Rectangle { color: "#2c313c"; radius: 4 }

                    Component.onCompleted: {
                        try {
                            // auto-fill from platform config not available
                            // directly in QML - user fills manually
                        } catch(e) {}
                    }
                }

                Text {
                    text: "Port:"
                    color: "#5f6a82"
                    font.pixelSize: 12
                }

                TextField {
                    id: portField
                    width: 80
                    height: 32
                    text: "50051"
                    color: "#c3cbdd"
                    font.pixelSize: 12
                    background: Rectangle { color: "#2c313c"; radius: 4 }
                }

                Button {
                    id: connectBtn
                    text: "Connect"
                    height: 32
                    width: 110
                    onClicked: {
                        if (streaming) {
                            faultMonitorBackend.disconnectFromDevice()
                        } else {
                            faultMonitorBackend.connectToDevice(
                                hostField.text, parseInt(portField.text)
                            )
                        }
                    }
                    background: Rectangle {
                        color: streaming
                             ? (parent.pressed ? "#8a0606" : parent.hovered ? "#c62828" : "#c62828")
                             : (parent.pressed ? "#18527e" : parent.hovered ? "#2a7ab8" : "#1f6aa5")
                        radius: 4
                    }
                    contentItem: Text {
                        text: connectBtn.text
                        color: "#fff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 12
                    }
                }

                Button {
                    text: "Clear"
                    height: 32
                    width: 70
                    onClicked: eventModel.clear()
                    background: Rectangle {
                        color: parent.pressed ? "#333" : parent.hovered ? "#4a4a4a" : "#3c3c3c"
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
                    id: statusLabel
                    text: "Disconnected"
                    color: "#5f6a82"
                    font.pixelSize: 12
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: eventModel.count + " events"
                    color: "#5f6a82"
                    font.pixelSize: 11
                }
            }
        }

        // ------------------------------------------------------------------
        // Event table
        // ------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1c1c1c"
            radius: 4
            border.color: "#383e4c"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Header
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    color: "#2c313c"
                    radius: 4

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 8

                        Text {
                            width: 200
                            height: parent.height
                            text: "Timestamp"
                            color: "#5f6a82"
                            font.pixelSize: 11
                            verticalAlignment: Text.AlignVCenter
                        }

                        Text {
                            width: 280
                            height: parent.height
                            text: "Status"
                            color: "#5f6a82"
                            font.pixelSize: 11
                            verticalAlignment: Text.AlignVCenter
                        }

                        Text {
                            width: 150
                            height: parent.height
                            text: "Source"
                            color: "#5f6a82"
                            font.pixelSize: 11
                            verticalAlignment: Text.AlignVCenter
                        }

                        Text {
                            width: 150
                            height: parent.height
                            text: "Signature"
                            color: "#5f6a82"
                            font.pixelSize: 11
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                // Rows
                ListView {
                    id: eventView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: eventModel
                    clip: true

                    ScrollBar.vertical: ScrollBar {}

                    delegate: Rectangle {
                        width: eventView.width
                        height: 28
                        color: index % 2 === 0 ? "#1e1e1e" : "#242424"

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 8

                            Text {
                                width: 200
                                height: parent.height
                                text: model.timestamp
                                color: "#c3cbdd"
                                font.pixelSize: 11
                                font.family: "Consolas"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                width: 280
                                height: parent.height
                                text: model.status
                                color: model.rowColor
                                font.pixelSize: 11
                                font.family: "Consolas"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                width: 150
                                height: parent.height
                                text: model.source
                                color: "#c3cbdd"
                                font.pixelSize: 11
                                font.family: "Consolas"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                width: 150
                                height: parent.height
                                text: model.signature !== "" ? model.signature : "-"
                                color: "#5f6a82"
                                font.pixelSize: 11
                                font.family: "Consolas"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }

        // ------------------------------------------------------------------
        // Log box
        // ------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 80
            color: "#1c1c1c"
            radius: 4
            border.color: "#383e4c"

            ScrollView {
                anchors.fill: parent
                anchors.margins: 6
                clip: true

                TextArea {
                    id: logBox
                    readOnly: true
                    wrapMode: TextArea.Wrap
                    color: "#c3cbdd"
                    font.family: "Consolas"
                    font.pixelSize: 11
                    background: Rectangle { color: "transparent" }
                    placeholderText: "Connection log..."
                    placeholderTextColor: "#5f6a82"
                }
            }
        }
    }
}