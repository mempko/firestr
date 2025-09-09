import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: chatApp
    color: "#2b2b2b"
    
    property string conversationId: ""
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 5
        
        // Messages area
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            ListView {
                id: messagesList
                spacing: 5
                model: ListModel {
                    id: messagesModel
                }
                
                delegate: Rectangle {
                    width: ListView.view ? ListView.view.width - 10 : 300
                    height: messageContent.height + 20
                    color: model.fromMe ? "#1976D2" : "#424242"
                    radius: 5
                    
                    Column {
                        id: messageContent
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 10
                        spacing: 5
                        
                        Row {
                            spacing: 10
                            
                            Text {
                                text: model.sender
                                color: "#e0e0e0"
                                font.bold: true
                                font.pixelSize: 12
                            }
                            
                            Text {
                                text: model.time
                                color: "#999999"
                                font.pixelSize: 10
                            }
                        }
                        
                        Text {
                            text: model.message
                            color: "#ffffff"
                            font.pixelSize: 14
                            width: parent.width
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
        
        // Input area
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            color: "#3c3c3c"
            radius: 5
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    
                    TextArea {
                        id: messageInput
                        placeholderText: qsTr("Type a message...")
                        color: "#e0e0e0"
                        font.pixelSize: 14
                        wrapMode: TextArea.Wrap
                        selectByMouse: true
                        
                        background: Rectangle {
                            color: "transparent"
                        }
                        
                        Keys.onReturnPressed: {
                            if (event.modifiers & Qt.ShiftModifier) {
                                // Shift+Enter adds a newline
                                event.accepted = false
                            } else {
                                // Enter sends the message
                                sendMessage()
                                event.accepted = true
                            }
                        }
                    }
                }
                
                Button {
                    text: qsTr("Send")
                    Layout.preferredWidth: 80
                    Layout.fillHeight: true
                    enabled: messageInput.text.trim().length > 0
                    
                    background: Rectangle {
                        color: parent.enabled ? (parent.pressed ? "#1976D2" : (parent.hovered ? "#42A5F5" : "#2196F3")) : "#555555"
                        radius: 3
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        font.pixelSize: 14
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: sendMessage()
                }
            }
        }
    }
    
    function sendMessage() {
        if (messageInput.text.trim().length === 0) return
        
        // Add message to list (temporary - should be handled by backend)
        messagesModel.append({
            sender: "Me",
            message: messageInput.text.trim(),
            time: Qt.formatTime(new Date(), "hh:mm"),
            fromMe: true
        })
        
        // TODO: Send message through conversation model
        
        // Clear input
        messageInput.text = ""
        
        // Scroll to bottom
        messagesList.positionViewAtEnd()
    }
    
    Component.onCompleted: {
        // Add welcome message
        messagesModel.append({
            sender: "System",
            message: "Chat started",
            time: Qt.formatTime(new Date(), "hh:mm"),
            fromMe: false
        })
    }
}