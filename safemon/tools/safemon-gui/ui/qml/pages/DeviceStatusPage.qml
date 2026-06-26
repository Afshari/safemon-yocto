import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    anchors.fill: parent

    property int refreshInterval: 10000

    Component.onCompleted: {
        try {
            // refreshInterval set from app.json via backend if needed
            // default 10 seconds
        } catch(e) {}
    }

    ListModel {
        id: statusModel
        ListElement { name: "safemon-app";     ok: false; checked: false }
        ListElement { name: "safemon-display"; ok: false; checked: false }
        ListElement { name: "redis";           ok: false; checked: false }
        ListElement { name: "vcan";            ok: false; checked: false }
    }

    Connections {
        target: deviceStatusBackend

        function onCheckStarted() {
            checkBtn.enabled = false
            checkBtn.text = "Checking..."
            checkLogBox.text = ""
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
            checkLogBox.text += "--- " + name + " ---\n"
            checkLogBox.text += output + "\n\n"
        }

        function onConnectionError(message) {
            checkLogBox.text += "ERROR: " + message + "\n"
            checkBtn.enabled = true
            checkBtn.text = "Check All"
        }

        function onJournalLoaded(output) {
            journalBox.text = output
        }

        function onJournalFailed(error) {
            journalBox.text = "ERROR: " + error
        }
    }

    // Auto-refresh timer
    Timer {
        id: refreshTimer
        interval: refreshInterval
        repeat: true
        running: false
        onTriggered: deviceStatusBackend.fetchJournal(
            serviceCombo.currentText,
            currentTarget, currentUsername, currentPassword
        )
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // ------------------------------------------------------------------
        // Top half - status checks
        // ------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1c1d20"
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                // Left - controls and indicators
                ColumnLayout {
                    width: 200
                    spacing: 10

                    Text {
                        text: "Service Checks"
                        color: "#c3cbdd"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    Button {
                        id: checkBtn
                        text: "Check All"
                        height: 32
                        width: 160
                        onClicked: deviceStatusBackend.checkAll(
                            currentTarget, currentUsername, currentPassword
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

                    Repeater {
                        model: statusModel
                        delegate: Row {
                            spacing: 10

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

                // Right - check results log
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
                            id: checkLogBox
                            readOnly: true
                            wrapMode: TextArea.Wrap
                            color: "#c3cbdd"
                            font.family: "Consolas"
                            font.pixelSize: 11
                            background: Rectangle { color: "transparent" }
                            placeholderText: "Click 'Check All' to run checks..."
                            placeholderTextColor: "#5f6a82"
                        }
                    }
                }
            }
        }

        // ------------------------------------------------------------------
        // Bottom half - journal viewer
        // ------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1c1d20"
            radius: 4

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                // Journal controls
                RowLayout {
                    spacing: 10
                    Layout.fillWidth: true

                    Text {
                        text: "Journal Log"
                        color: "#c3cbdd"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    ComboBox {
                        id: serviceCombo
                        width: 180
                        height: 28
                        model: ["safemon-app", "safemon-display"]
                        onCurrentValueChanged: {
                            if (refreshTimer.running) {
                                deviceStatusBackend.fetchJournal(
                                    currentValue,
                                    currentTarget, currentUsername, currentPassword
                                )
                            }
                        }
                        background: Rectangle {
                            color: "#2c313c"
                            radius: 4
                            border.color: "#383e4c"
                        }
                        contentItem: Text {
                            leftPadding: 8
                            text: serviceCombo.displayText
                            color: "#c3cbdd"
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Button {
                        id: fetchOnceBtn
                        text: "Fetch Now"
                        height: 28
                        width: 90
                        onClicked: deviceStatusBackend.fetchJournal(
                            serviceCombo.currentText,
                            currentTarget, currentUsername, currentPassword
                        )
                        background: Rectangle {
                            color: parent.pressed ? "#333"
                                 : parent.hovered ? "#4a4a4a" : "#3c3c3c"
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
                        id: autoRefreshBtn
                        text: refreshTimer.running ? "Stop Auto-Refresh" : "Start Auto-Refresh"
                        height: 28
                        width: 150
                        onClicked: {
                            if (refreshTimer.running) {
                                refreshTimer.stop()
                            } else {
                                refreshTimer.start()
                                deviceStatusBackend.fetchJournal(
                                    serviceCombo.currentText,
                                    currentTarget, currentUsername, currentPassword
                                )
                            }
                        }
                        background: Rectangle {
                            color: refreshTimer.running
                                 ? (parent.pressed ? "#8a0606" : parent.hovered ? "#c62828" : "#c62828")
                                 : (parent.pressed ? "#18527e" : parent.hovered ? "#2a7ab8" : "#1f6aa5")
                            radius: 4
                        }
                        contentItem: Text {
                            text: autoRefreshBtn.text
                            color: "#fff"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 12
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: refreshTimer.running ? "Auto-refreshing..." : ""
                        color: "#4caf50"
                        font.pixelSize: 11
                    }
                }

                // Journal output
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
                            id: journalBox
                            readOnly: true
                            wrapMode: TextArea.NoWrap
                            color: "#c3cbdd"
                            font.family: "Consolas"
                            font.pixelSize: 11
                            background: Rectangle { color: "transparent" }
                            placeholderText: "Select a service and click 'Fetch Now' or 'Start Auto-Refresh'..."
                            placeholderTextColor: "#5f6a82"
                        }
                    }
                }
            }
        }
    }
}