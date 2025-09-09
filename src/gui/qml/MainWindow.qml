import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "." as Dialogs
import "."

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1024
    height: 768
    title: qsTr("Fire★ - ") + mainController.userName
    
    // Menu Bar
    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Quit")
                shortcut: StandardKey.Quit
                onTriggered: mainWindow.close()
            }
        }
        
        Menu {
            title: qsTr("&Identity")
            Action {
                text: qsTr("Show &Identity...")
                onTriggered: showIdentityDialog.open()
            }
            Action {
                text: qsTr("&Add Contact...")
                onTriggered: addContactDialog.open()
            }
            Action {
                text: qsTr("&Email Invite...")
                onTriggered: mainController.emailInvite()
            }
        }
        
        Menu {
            title: qsTr("&Conversation")
            Action {
                text: qsTr("&Start Conversation...")
                enabled: mainController.hasContacts
                onTriggered: startConversationDialog.open()
            }
        }
        
        Menu {
            title: qsTr("&Apps")
            Action {
                text: qsTr("&App Editor...")
                onTriggered: mainController.createAppEditor()
            }
            Action {
                text: qsTr("&Install App...")
                onTriggered: mainController.installApp()
            }
            MenuSeparator {}
            Action {
                text: qsTr("Install &Example Apps")
                onTriggered: mainController.installExampleApps()
            }
        }
        
        Menu {
            title: qsTr("&Help")
            Action {
                text: qsTr("&About Fire★...")
                onTriggered: aboutDialog.open()
            }
        }
    }
    
    // Main content
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // Tab Bar
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            
            // Dynamic tabs will be added here
            Repeater {
                model: mainController.tabs
                TabButton {
                    text: modelData.title
                    width: implicitWidth
                    
                    // Visual indicator for alerts
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        opacity: 0.3
                        visible: modelData.hasAlert
                        
                        SequentialAnimation on color {
                            running: modelData.hasAlert
                            loops: Animation.Infinite
                            ColorAnimation { to: "#ff9800"; duration: 500 }
                            ColorAnimation { to: "transparent"; duration: 500 }
                        }
                    }
                }
            }
        }
        
        // Content area
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex
            
            // Welcome Screen
            Item {
                visible: mainController.showWelcome
                
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 30
                    width: parent.width * 0.8
                    
                    Text {
                        text: qsTr("Welcome to Fire★")
                        font.pixelSize: 32
                        font.bold: true
                        color: "#4CAF50"
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Text {
                        text: qsTr("The Grass Computing Platform")
                        font.pixelSize: 20
                        color: "#666666"
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Rectangle {
                        Layout.preferredHeight: 1
                        Layout.fillWidth: true
                        color: "#dddddd"
                    }
                    
                    Column {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 10
                        
                        Text {
                            text: qsTr("Getting Started:")
                            font.pixelSize: 18
                            font.bold: true
                            color: "#333333"
                        }
                        
                        Text {
                            text: qsTr("1. Add contacts using Identity → Add Contact")
                            font.pixelSize: 14
                            color: "#666666"
                        }
                        
                        Text {
                            text: qsTr("2. Start a conversation with your contacts")
                            font.pixelSize: 14
                            color: "#666666"
                        }
                        
                        Text {
                            text: qsTr("3. Share apps and collaborate in real-time")
                            font.pixelSize: 14
                            color: "#666666"
                        }
                    }
                    
                    Button {
                        text: qsTr("Add Your First Contact")
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 45
                        
                        background: Rectangle {
                            color: parent.pressed ? "#388E3C" : (parent.hovered ? "#66BB6A" : "#4CAF50")
                            radius: 4
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.pixelSize: 14
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        onClicked: addContactDialog.open()
                    }
                }
            }
            
            // People/Contacts Screen
            Item {
                visible: mainController.showContacts
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10
                    
                    // Toolbar
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Text {
                            text: qsTr("Contacts")
                            font.pixelSize: 20
                            font.bold: true
                            color: "#333333"
                        }
                        
                        Item { Layout.fillWidth: true }
                        
                        Button {
                            text: qsTr("Add Contact")
                            
                            background: Rectangle {
                                color: parent.pressed ? "#1976D2" : (parent.hovered ? "#42A5F5" : "#2196F3")
                                radius: 4
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            onClicked: addContactDialog.open()
                        }
                        
                        Button {
                            text: qsTr("Start Conversation")
                            enabled: contactList.currentIndex >= 0
                            
                            background: Rectangle {
                                color: {
                                    if (!parent.enabled) return "#cccccc"
                                    return parent.pressed ? "#388E3C" : (parent.hovered ? "#66BB6A" : "#4CAF50")
                                }
                                radius: 4
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                color: parent.enabled ? "white" : "#999999"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            onClicked: mainController.startConversationWith(contactList.currentIndex)
                        }
                    }
                    
                    // Contact List
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "white"
                        border.color: "#dddddd"
                        border.width: 1
                        radius: 4
                        
                        ListView {
                            id: contactList
                            anchors.fill: parent
                            anchors.margins: 5
                            model: mainController.contacts
                            spacing: 5
                            clip: true
                            
                            delegate: Rectangle {
                                width: parent.width
                                height: 60
                                color: ListView.isCurrentItem ? "#e3f2fd" : (mouseArea.containsMouse ? "#f5f5f5" : "white")
                                radius: 4
                                
                                MouseArea {
                                    id: mouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: contactList.currentIndex = index
                                    onDoubleClicked: mainController.startConversationWith(index)
                                }
                                
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10
                                    
                                    // Avatar placeholder
                                    Rectangle {
                                        width: 40
                                        height: 40
                                        radius: 20
                                        color: modelData.online ? "#4CAF50" : "#9E9E9E"
                                        
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.name.charAt(0).toUpperCase()
                                            color: "white"
                                            font.pixelSize: 18
                                            font.bold: true
                                        }
                                    }
                                    
                                    ColumnLayout {
                                        spacing: 2
                                        
                                        Text {
                                            text: modelData.name
                                            font.pixelSize: 14
                                            font.bold: true
                                            color: "#333333"
                                        }
                                        
                                        Text {
                                            text: modelData.online ? qsTr("Online") : qsTr("Offline")
                                            font.pixelSize: 12
                                            color: modelData.online ? "#4CAF50" : "#999999"
                                        }
                                    }
                                    
                                    Item { Layout.fillWidth: true }
                                }
                            }
                        }
                    }
                }
            }
            
            // Conversation tabs (dynamic)
            Repeater {
                model: mainController.conversations
                
                ConversationCanvas {
                    conversationId: modelData.conversationId
                }
            }
        }
    }
    
    // Dialogs
    Dialogs.AddContactDialog {
        id: addContactDialog
        
        onAddContact: function(identity) {
            mainController.processAddContact(identity)
            close()
        }
    }
    
    Dialogs.ShowIdentityDialog {
        id: showIdentityDialog
        
        greeters: mainController.getGreeters()
        
        onSelectedGreeterChanged: {
            identity = mainController.getUserIdentity(selectedGreeter)
        }
        
        Component.onCompleted: {
            identity = mainController.getUserIdentity(0)
        }
    }
    
    Dialogs.AboutDialog {
        id: aboutDialog
    }
    
    Dialogs.StartConversationDialog {
        id: startConversationDialog
        
        onStartConversation: function(contactIds) {
            mainController.createConversation(contactIds)
        }
    }
}