import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Dialog {
    id: startConversationDialog
    title: qsTr("Start Conversation")
    modal: true
    width: 500
    height: 500
    
    property var selectedContacts: []
    
    signal startConversation(var contactIds)
    
    background: Rectangle {
        color: "#2b2b2b"  // Dark grey background
        border.color: "#555555"
        border.width: 1
        radius: 4
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 15
        
        Label {
            text: qsTr("Select contacts to include in the conversation:")
            font.pixelSize: 16
            font.bold: true
            color: "#e0e0e0"
        }
        
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#555555"
        }
        
        // Contact list with checkboxes
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            ListView {
                id: contactListView
                model: mainController.contacts
                spacing: 5
                clip: true
                
                delegate: Rectangle {
                    width: parent.width
                    height: 50
                    color: checkBox.checked ? "#3c3c3c" : "transparent"
                    radius: 4
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: checkBox.checked = !checkBox.checked
                    }
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10
                        
                        CheckBox {
                            id: checkBox
                            
                            indicator: Rectangle {
                                implicitWidth: 20
                                implicitHeight: 20
                                radius: 3
                                color: checkBox.checked ? "#4CAF50" : "#3c3c3c"
                                border.color: checkBox.checked ? "#4CAF50" : "#555555"
                                border.width: 1
                                
                                Rectangle {
                                    visible: checkBox.checked
                                    anchors.centerIn: parent
                                    width: 10
                                    height: 10
                                    radius: 2
                                    color: "white"
                                }
                            }
                            
                            onCheckedChanged: {
                                if (checked) {
                                    if (selectedContacts.indexOf(modelData.id) === -1) {
                                        selectedContacts.push(modelData.id)
                                    }
                                } else {
                                    var index = selectedContacts.indexOf(modelData.id)
                                    if (index !== -1) {
                                        selectedContacts.splice(index, 1)
                                    }
                                }
                            }
                        }
                        
                        // Contact avatar
                        Rectangle {
                            width: 30
                            height: 30
                            radius: 15
                            color: modelData.online ? "#4CAF50" : "#666666"
                            
                            Text {
                                anchors.centerIn: parent
                                text: modelData.name.charAt(0).toUpperCase()
                                color: "white"
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }
                        
                        // Contact name and status
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            
                            Text {
                                text: modelData.name
                                font.pixelSize: 14
                                font.bold: true
                                color: "#e0e0e0"
                            }
                            
                            Text {
                                text: modelData.online ? qsTr("Online") : qsTr("Offline")
                                font.pixelSize: 11
                                color: modelData.online ? "#4CAF50" : "#999999"
                            }
                        }
                    }
                }
            }
        }
        
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#555555"
        }
        
        Text {
            text: {
                if (selectedContacts.length === 0) {
                    return qsTr("No contacts selected (solo conversation)")
                } else if (selectedContacts.length === 1) {
                    return qsTr("1 contact selected")
                } else {
                    return qsTr("%1 contacts selected").arg(selectedContacts.length)
                }
            }
            font.pixelSize: 13
            color: "#4CAF50"
            Layout.alignment: Qt.AlignHCenter
        }
    }
    
    footer: DialogButtonBox {
        Button {
            text: qsTr("Start")
            enabled: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            
            background: Rectangle {
                color: {
                    if (!parent.enabled) return "#3c3c3c"
                    return parent.pressed ? "#388E3C" : (parent.hovered ? "#66BB6A" : "#4CAF50")
                }
                radius: 4
            }
            
            contentItem: Text {
                text: parent.text
                color: parent.enabled ? "white" : "#666666"
                font.pixelSize: 14
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            
            background: Rectangle {
                color: parent.pressed ? "#4a4a4a" : (parent.hovered ? "#404040" : "#3c3c3c")
                border.color: "#555555"
                border.width: 1
                radius: 4
            }
            
            contentItem: Text {
                text: parent.text
                color: "#e0e0e0"
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
    
    onAccepted: {
        startConversation(selectedContacts)
    }
    
    onRejected: {
        selectedContacts = []
        // Reset all checkboxes
        for (var i = 0; i < contactListView.count; i++) {
            var item = contactListView.itemAtIndex(i)
            if (item) {
                item.children[1].children[0].checked = false
            }
        }
    }
    
    onOpened: {
        selectedContacts = []
    }
}