import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: scriptApp
    color: "#2b2b2b"
    
    property string conversationId: ""
    property string appName: "Script App"
    property string appId: ""
    property bool micEnabled: false
    property var scriptAppModel: null
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            
            Text {
                text: appName
                color: "#e0e0e0"
                font.pixelSize: 16
                font.bold: true
                Layout.fillWidth: true
            }
            
            Button {
                text: qsTr("Clone")
                visible: false // TODO: Enable when clone is implemented
                
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
                
                onClicked: cloneApp()
            }
            
            Button {
                text: micEnabled ? qsTr("Mute Mic") : qsTr("Unmute Mic")
                visible: false // TODO: Show when app uses microphone
                
                background: Rectangle {
                    color: micEnabled ? 
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
                
                onClicked: toggleMic()
            }
        }
        
        // App content area - this is where the Lua app's UI will be rendered
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1e1e1e"
            radius: 3
            border.color: "#3c3c3c"
            border.width: 1
            
            // Container for Lua app UI components
            Item {
                id: appContent
                anchors.fill: parent
                anchors.margins: 10
                
                // The QML frontend root item will be loaded here
                Item {
                    id: luaAppContainer
                    anchors.fill: parent
                    
                    // Fallback UI shown before Lua app loads
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 10
                        visible: luaAppContainer.children.length <= 1 // Only this column layout
                        
                        Text {
                            text: qsTr("Loading Script App...")
                            color: "#e0e0e0"
                            font.pixelSize: 14
                            Layout.alignment: Qt.AlignHCenter
                        }
                        
                        Text {
                            text: appName
                            color: "#999999"
                            font.pixelSize: 12
                            font.italic: true
                            Layout.alignment: Qt.AlignHCenter
                        }
                        
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: "#3c3c3c"
                        }
                        
                        // Output area for debug messages
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            
                            TextArea {
                                id: outputArea
                                color: "#00FF00"
                                font.family: "Consolas, Monaco, monospace"
                                font.pixelSize: 11
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextArea.Wrap
                                text: scriptAppModel ? scriptAppModel.output : "Initializing...\n"
                                
                                background: Rectangle {
                                    color: "#0a0a0a"
                                    radius: 3
                                }
                            }
                        }
                        
                        Item {
                            Layout.fillHeight: true
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
                    text: qsTr("Running")
                    color: "#4CAF50"
                    font.pixelSize: 11
                }
                
                Text {
                    text: qsTr("Lua Script")
                    color: "#999999"
                    font.pixelSize: 11
                }
                
                Text {
                    text: "ID: " + appId
                    color: "#666666"
                    font.pixelSize: 10
                    visible: false // Debug only
                }
            }
        }
    }
    
    function cloneApp() {
        console.log("Clone app:", appName)
        // TODO: Implement through conversation model
    }
    
    function toggleMic() {
        micEnabled = !micEnabled
        console.log("Mic enabled:", micEnabled)
        // TODO: Implement through conversation model
    }
    
    function appendOutput(text) {
        outputArea.text += text
    }
    
    Component.onCompleted: {
        console.log("ScriptApp loaded - name:", appName, "id:", appId)
        console.log("ScriptApp model:", scriptAppModel)
        connectToModel()
        
        // Start the app if we have a model
        console.log("Checking if we can start app...")
        console.log("scriptAppModel:", scriptAppModel)
        if (scriptAppModel) {
            console.log("Have script model")
            console.log("runApp function:", scriptAppModel.runApp)
            if (scriptAppModel.runApp) {
                console.log("Starting script app...")
                scriptAppModel.runApp()
            } else {
                console.log("runApp method not available")
            }
        } else {
            console.log("No script model")
        }
    }
    
    function connectToModel() {
        // Check if we already have a luaRootItem from the model
        if (scriptAppModel) {
            console.log("Script app model status:", scriptAppModel.status)
            console.log("Script app model output:", scriptAppModel.output)
            
            if (scriptAppModel.luaRootItem) {
                console.log("Setting Lua root item on connect")
                console.log("Lua root item:", scriptAppModel.luaRootItem)
                var rootItem = scriptAppModel.luaRootItem
                rootItem.parent = luaAppContainer
                rootItem.anchors.fill = luaAppContainer
                rootItem.visible = true
                console.log("Root item parent set to:", rootItem.parent)
                console.log("Root item visible:", rootItem.visible)
                console.log("Root item size:", rootItem.width, "x", rootItem.height)
            } else {
                console.log("No luaRootItem yet")
            }
        }
    }
    
    property bool modelConnected: false
    
    onScriptAppModelChanged: {
        console.log("Script app model changed to:", scriptAppModel)
        if (scriptAppModel) {
            modelConnected = true
            console.log("Connected to script app model")
            console.log("Initial status:", scriptAppModel.status)
            console.log("Initial output:", scriptAppModel.output)
            console.log("runApp method available:", scriptAppModel.runApp)
            connectToModel()
            
            // Try to start the app when model changes
            if (scriptAppModel.runApp) {
                console.log("Starting script app from model change...")
                scriptAppModel.runApp()
            }
        }
    }
    
    Connections {
        target: scriptAppModel
        enabled: scriptAppModel !== null
        
        function onOutputChanged() {
            console.log("Output changed:", scriptAppModel.output)
            if (scriptAppModel) {
                outputArea.text = scriptAppModel.output
            }
        }
        
        function onStatusChanged() {
            console.log("Script app status:", scriptAppModel.status)
        }
        
        function onLuaRootItemChanged() {
            console.log("Lua root item changed")
            if (scriptAppModel && scriptAppModel.luaRootItem) {
                console.log("Adding Lua UI to container")
                var rootItem = scriptAppModel.luaRootItem
                rootItem.parent = luaAppContainer
                rootItem.anchors.fill = luaAppContainer
                rootItem.visible = true
                console.log("Root item added - visible:", rootItem.visible, "size:", rootItem.width, "x", rootItem.height)
            }
        }
    }
}