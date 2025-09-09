import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: appEditor
    color: "#2b2b2b"
    
    property string conversationId: ""
    property string appName: "New App"
    property bool isRunning: false
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            
            TextField {
                id: appNameField
                Layout.fillWidth: true
                text: appName
                placeholderText: qsTr("App Name")
                color: "#e0e0e0"
                font.pixelSize: 14
                
                background: Rectangle {
                    color: "#3c3c3c"
                    radius: 3
                    border.color: parent.focus ? "#2196F3" : "#555555"
                    border.width: 1
                }
                
                onTextChanged: appName = text
            }
            
            Button {
                text: isRunning ? qsTr("Stop") : qsTr("Run")
                
                background: Rectangle {
                    color: isRunning ? 
                        (parent.pressed ? "#C62828" : (parent.hovered ? "#EF5350" : "#F44336")) :
                        (parent.pressed ? "#388E3C" : (parent.hovered ? "#66BB6A" : "#4CAF50"))
                    radius: 3
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
                    if (isRunning) {
                        stopApp()
                    } else {
                        runApp()
                    }
                }
            }
            
            Button {
                text: qsTr("Save")
                
                background: Rectangle {
                    color: parent.pressed ? "#1976D2" : (parent.hovered ? "#42A5F5" : "#2196F3")
                    radius: 3
                }
                
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: saveApp()
            }
            
            Button {
                text: qsTr("Export")
                
                background: Rectangle {
                    color: parent.pressed ? "#5D4037" : (parent.hovered ? "#8D6E63" : "#795548")
                    radius: 3
                }
                
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: exportApp()
            }
        }
        
        // Code editor
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: parent.height * 0.6
            
            TextArea {
                id: codeEditor
                text: "-- Fire★ App\n-- Write your Lua code here\n\nfunction main()\n    print('Hello from Fire★!')\nend\n\nmain()"
                color: "#e0e0e0"
                font.family: "Consolas, Monaco, monospace"
                font.pixelSize: 12
                selectByMouse: true
                wrapMode: TextArea.NoWrap
                
                background: Rectangle {
                    color: "#1e1e1e"
                    radius: 3
                    border.color: "#3c3c3c"
                    border.width: 1
                }
            }
        }
        
        // Output console
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: parent.height * 0.3
            color: "#1e1e1e"
            radius: 3
            border.color: "#3c3c3c"
            border.width: 1
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 5
                spacing: 5
                
                RowLayout {
                    Layout.fillWidth: true
                    
                    Text {
                        text: qsTr("Output")
                        color: "#999999"
                        font.pixelSize: 11
                        font.bold: true
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    Button {
                        text: qsTr("Clear")
                        Layout.preferredHeight: 20
                        
                        background: Rectangle {
                            color: parent.pressed ? "#555555" : (parent.hovered ? "#666666" : "transparent")
                            radius: 2
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            color: "#999999"
                            font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        onClicked: outputConsole.text = ""
                    }
                }
                
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    
                    TextArea {
                        id: outputConsole
                        color: "#00FF00"
                        font.family: "Consolas, Monaco, monospace"
                        font.pixelSize: 11
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextArea.Wrap
                        
                        background: Rectangle {
                            color: "transparent"
                        }
                    }
                }
            }
        }
        
        // Status bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 25
            color: "#3c3c3c"
            radius: 3
            
            Row {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 10
                spacing: 20
                
                Text {
                    text: isRunning ? qsTr("Running") : qsTr("Ready")
                    color: isRunning ? "#4CAF50" : "#999999"
                    font.pixelSize: 11
                }
                
                Text {
                    text: qsTr("Lua")
                    color: "#999999"
                    font.pixelSize: 11
                }
            }
        }
    }
    
    function runApp() {
        isRunning = true
        outputConsole.text = "Starting app...\n"
        // TODO: Execute through conversation model
        outputConsole.text += "Hello from Fire★!\n"
        outputConsole.text += "App completed.\n"
        isRunning = false
    }
    
    function stopApp() {
        isRunning = false
        outputConsole.text += "App stopped.\n"
    }
    
    function saveApp() {
        outputConsole.text += "App saved: " + appName + "\n"
        // TODO: Save through conversation model
    }
    
    function exportApp() {
        outputConsole.text += "Exporting app...\n"
        // TODO: Export through conversation model
    }
}