import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: conversationView
    
    property string conversationId: ""
    property var participants: []
    property var messages: []
    property var apps: []
    
    Rectangle {
        anchors.fill: parent
        color: "#2b2b2b"  // Dark background
        
        SplitView {
            anchors.fill: parent
            anchors.margins: 10
            orientation: Qt.Horizontal
            
            // Left panel - Participants
            Rectangle {
                SplitView.preferredWidth: 200
                SplitView.minimumWidth: 150
                color: "#3c3c3c"
                border.color: "#555555"
                border.width: 1
                radius: 4
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10
                    
                    Text {
                        text: qsTr("Participants")
                        font.pixelSize: 16
                        font.bold: true
                        color: "#e0e0e0"
                    }
                    
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#555555"
                    }
                    
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        
                        ListView {
                            id: participantsList
                            model: participants
                            spacing: 5
                            
                            delegate: Rectangle {
                                width: parent ? parent.width : 180
                                height: 40
                                color: mouseArea.containsMouse ? "#4a4a4a" : "transparent"
                                radius: 4
                                
                                MouseArea {
                                    id: mouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                }
                                
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 8
                                    
                                    // Online indicator
                                    Rectangle {
                                        width: 8
                                        height: 8
                                        radius: 4
                                        color: modelData.online ? "#4CAF50" : "#666666"
                                    }
                                    
                                    // Name
                                    Text {
                                        text: modelData.name
                                        color: "#e0e0e0"
                                        font.pixelSize: 13
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }
                    
                    // Add participant button
                    Button {
                        text: qsTr("Add Contact")
                        Layout.fillWidth: true
                        Layout.preferredHeight: 35
                        
                        background: Rectangle {
                            color: parent.pressed ? "#388E3C" : (parent.hovered ? "#66BB6A" : "#4CAF50")
                            radius: 4
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.pixelSize: 12
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        onClicked: {
                            // TODO: Show add contact to conversation dialog
                        }
                    }
                }
            }
            
            // Middle panel - Messages and apps
            Rectangle {
                SplitView.fillWidth: true
                color: "#3c3c3c"
                border.color: "#555555"
                border.width: 1
                radius: 4
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10
                    
                    // App area
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.maximumHeight: parent.height * 0.6
                        color: "#2b2b2b"
                        border.color: "#555555"
                        border.width: 1
                        radius: 4
                        
                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: 10
                            
                            Flow {
                                id: appsFlow
                                width: parent.width
                                spacing: 10
                                
                                // App widgets would go here
                                Repeater {
                                    model: apps
                                    
                                    Rectangle {
                                        width: 200
                                        height: 150
                                        color: "#4a4a4a"
                                        border.color: "#666666"
                                        border.width: 1
                                        radius: 4
                                        
                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 10
                                            
                                            Text {
                                                text: modelData.name
                                                color: "#e0e0e0"
                                                font.pixelSize: 14
                                                font.bold: true
                                            }
                                            
                                            Rectangle {
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                color: "#2b2b2b"
                                                
                                                // App content would go here
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: qsTr("App content")
                                                    color: "#999999"
                                                    font.pixelSize: 12
                                                }
                                            }
                                        }
                                    }
                                }
                                
                                // Add app button
                                Rectangle {
                                    width: 200
                                    height: 150
                                    color: "transparent"
                                    border.color: "#555555"
                                    border.width: 2
                                    radius: 4
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        
                                        onClicked: {
                                            // TODO: Show add app dialog
                                        }
                                    }
                                    
                                    Column {
                                        anchors.centerIn: parent
                                        spacing: 10
                                        
                                        Text {
                                            text: "+"
                                            color: "#666666"
                                            font.pixelSize: 36
                                            font.bold: true
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }
                                        
                                        Text {
                                            text: qsTr("Add App")
                                            color: "#666666"
                                            font.pixelSize: 14
                                            anchors.horizontalCenter: parent.horizontalCenter
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
                    
                    // Message area
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#2b2b2b"
                        border.color: "#555555"
                        border.width: 1
                        radius: 4
                        
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10
                            
                            // Messages list
                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                
                                ListView {
                                    id: messagesList
                                    model: messages
                                    spacing: 10
                                    verticalLayoutDirection: ListView.BottomToTop
                                    
                                    delegate: Rectangle {
                                        width: parent ? parent.width : 100
                                        height: messageContent.height + 20
                                        color: modelData.fromMe ? "#4a4a4a" : "#3c3c3c"
                                        radius: 4
                                        
                                        ColumnLayout {
                                            id: messageContent
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.top: parent.top
                                            anchors.margins: 10
                                            spacing: 5
                                            
                                            Text {
                                                text: modelData.sender
                                                color: "#4CAF50"
                                                font.pixelSize: 12
                                                font.bold: true
                                            }
                                            
                                            Text {
                                                text: modelData.text
                                                color: "#e0e0e0"
                                                font.pixelSize: 13
                                                wrapMode: Text.WordWrap
                                                Layout.fillWidth: true
                                            }
                                            
                                            Text {
                                                text: modelData.time
                                                color: "#999999"
                                                font.pixelSize: 11
                                            }
                                        }
                                    }
                                }
                            }
                            
                            // Message input
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10
                                
                                TextField {
                                    id: messageInput
                                    Layout.fillWidth: true
                                    placeholderText: qsTr("Type a message...")
                                    font.pixelSize: 13
                                    color: "#e0e0e0"
                                    
                                    background: Rectangle {
                                        color: "#4a4a4a"
                                        border.color: messageInput.focus ? "#4CAF50" : "#555555"
                                        border.width: messageInput.focus ? 2 : 1
                                        radius: 4
                                    }
                                    
                                    onAccepted: {
                                        if (text.trim().length > 0) {
                                            // TODO: Send message
                                            text = ""
                                        }
                                    }
                                }
                                
                                Button {
                                    text: qsTr("Send")
                                    Layout.preferredWidth: 80
                                    enabled: messageInput.text.trim().length > 0
                                    
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
                                        font.pixelSize: 13
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    
                                    onClicked: {
                                        if (messageInput.text.trim().length > 0) {
                                            // TODO: Send message
                                            messageInput.text = ""
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // Right panel - App library
            Rectangle {
                SplitView.preferredWidth: 180
                SplitView.minimumWidth: 150
                color: "#3c3c3c"
                border.color: "#555555"
                border.width: 1
                radius: 4
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10
                    
                    Text {
                        text: qsTr("Apps")
                        font.pixelSize: 16
                        font.bold: true
                        color: "#e0e0e0"
                    }
                    
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#555555"
                    }
                    
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        
                        ListView {
                            id: appLibraryList
                            spacing: 5
                            
                            // TODO: Model should come from app service
                            model: ListModel {
                                ListElement { name: "Chat"; icon: "💬" }
                                ListElement { name: "Whiteboard"; icon: "✏️" }
                                ListElement { name: "Code Editor"; icon: "📝" }
                                ListElement { name: "File Share"; icon: "📁" }
                            }
                            
                            delegate: Rectangle {
                                width: parent ? parent.width : 160
                                height: 40
                                color: appMouseArea.containsMouse ? "#4a4a4a" : "transparent"
                                radius: 4
                                
                                MouseArea {
                                    id: appMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    
                                    onClicked: {
                                        // TODO: Add app to conversation
                                    }
                                }
                                
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 8
                                    
                                    Text {
                                        text: model.icon
                                        font.pixelSize: 16
                                    }
                                    
                                    Text {
                                        text: model.name
                                        color: "#e0e0e0"
                                        font.pixelSize: 13
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }
                    
                    Button {
                        text: qsTr("Install App")
                        Layout.fillWidth: true
                        Layout.preferredHeight: 35
                        
                        background: Rectangle {
                            color: parent.pressed ? "#1976D2" : (parent.hovered ? "#42A5F5" : "#2196F3")
                            radius: 4
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.pixelSize: 12
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        onClicked: {
                            // TODO: Show install app dialog
                        }
                    }
                }
            }
        }
    }
}