import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: conversationCanvas
    
    property string conversationId: ""
    property var conversationModel: null
    
    Component.onCompleted: {
        console.log("ConversationCanvas onCompleted - conversationId:", conversationId)
        if (conversationId && mainController) {
            conversationModel = mainController.getConversationModel(conversationId)
            console.log("Got conversation model:", conversationModel)
            if (conversationModel) {
                console.log("Setting models...")
                console.log("Available apps:", conversationModel.availableApps)
                participantsList.model = conversationModel.participants
                appsList.model = conversationModel.availableApps
                activeAppsRepeater.model = conversationModel.activeApps
            }
        }
    }
    
    Connections {
        target: conversationModel
        function onActiveAppsChanged() {
            console.log("Active apps changed, count:", conversationModel.activeApps.length)
            activeAppsRepeater.model = conversationModel.activeApps
        }
    }
    
    // Dark background
    Rectangle {
        anchors.fill: parent
        color: "#1e1e1e"
    }
    
    // Main split view - participants on left, canvas on right
    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal
        
        // Left panel - Participants and Apps
        Rectangle {
            SplitView.preferredWidth: 200
            SplitView.minimumWidth: 150
            SplitView.maximumWidth: 300
            color: "#2b2b2b"
            border.color: "#3c3c3c"
            border.width: 1
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                // Participants section
                Text {
                    text: qsTr("Participants")
                    font.pixelSize: 14
                    font.bold: true
                    color: "#e0e0e0"
                }
                
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#3c3c3c"
                }
                
                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    
                    ListView {
                        id: participantsList
                        model: conversationModel ? conversationModel.participants : []
                        spacing: 3
                        
                        delegate: Rectangle {
                            width: parent ? parent.width : 180
                            height: 30
                            color: mouseArea.containsMouse ? "#3c3c3c" : "transparent"
                            radius: 3
                            
                            MouseArea {
                                id: mouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 5
                                spacing: 5
                                
                                Rectangle {
                                    width: 6
                                    height: 6
                                    radius: 3
                                    color: modelData.online ? "#4CAF50" : "#666666"
                                }
                                
                                Text {
                                    text: modelData.name
                                    color: "#e0e0e0"
                                    font.pixelSize: 12
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
                
                // Apps section
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#3c3c3c"
                }
                
                Text {
                    text: qsTr("Apps")
                    font.pixelSize: 14
                    font.bold: true
                    color: "#e0e0e0"
                }
                
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    
                    ListView {
                        id: appsList
                        spacing: 5
                        
                        model: conversationModel ? conversationModel.availableApps : []
                        
                        delegate: Rectangle {
                            width: parent ? parent.width : 180
                            height: 36
                            color: appMouseArea.pressed ? "#4a4a4a" : (appMouseArea.containsMouse ? "#3c3c3c" : "transparent")
                            radius: 3
                            
                            MouseArea {
                                id: appMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                
                                onDoubleClicked: {
                                    if (conversationModel) {
                                        if (modelData.id === "builtin_chat") {
                                            conversationModel.launchChatApp()
                                        } else if (modelData.id === "builtin_editor") {
                                            conversationModel.launchAppEditor()
                                        } else {
                                            conversationModel.launchScriptApp(modelData.id)
                                        }
                                    }
                                }
                            }
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 6
                                spacing: 8
                                
                                Text {
                                    text: modelData.icon || "📦"
                                    font.pixelSize: 16
                                }
                                
                                Text {
                                    text: modelData.name || "Unknown"
                                    color: "#e0e0e0"
                                    font.pixelSize: 12
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                
                                Text {
                                    text: modelData.type === "builtin" ? qsTr("built-in") : ""
                                    color: "#666666"
                                    font.pixelSize: 10
                                    font.italic: true
                                }
                            }
                        }
                    }
                }
                
                // Buttons
                Button {
                    text: qsTr("Add Contact")
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    
                    background: Rectangle {
                        color: parent.pressed ? "#388E3C" : (parent.hovered ? "#66BB6A" : "#4CAF50")
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
                        // TODO: Show add contact dialog
                    }
                }
                
                Button {
                    text: qsTr("Install App")
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    
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
                    
                    onClicked: {
                        // TODO: Show install app dialog
                    }
                }
            }
        }
        
        // Right panel - Canvas area for apps
        Rectangle {
            SplitView.fillWidth: true
            color: "#1e1e1e"
            clip: true
            
            // Drop area for apps
            DropArea {
                anchors.fill: parent
                
                Rectangle {
                    anchors.fill: parent
                    color: parent.containsDrag ? "#2a2a2a" : "transparent"
                    
                    // Canvas for app windows
                    Item {
                        id: appCanvas
                        anchors.fill: parent
                        
                        // Grid pattern background
                        Canvas {
                            anchors.fill: parent
                            opacity: 0.1
                            
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.strokeStyle = "#ffffff"
                                ctx.lineWidth = 0.5
                                
                                var gridSize = 20
                                
                                // Draw vertical lines
                                for (var x = 0; x <= width; x += gridSize) {
                                    ctx.beginPath()
                                    ctx.moveTo(x, 0)
                                    ctx.lineTo(x, height)
                                    ctx.stroke()
                                }
                                
                                // Draw horizontal lines
                                for (var y = 0; y <= height; y += gridSize) {
                                    ctx.beginPath()
                                    ctx.moveTo(0, y)
                                    ctx.lineTo(width, y)
                                    ctx.stroke()
                                }
                            }
                        }
                        
                        // Container for app windows
                        Repeater {
                            id: activeAppsRepeater
                            model: conversationModel ? conversationModel.activeApps : []
                            
                            onItemAdded: {
                                console.log("App window added at index", index)
                                console.log("Model data:", model[index])
                            }
                            
                            AppWindow {
                                id: appWindow
                                appId: modelData.instanceId
                                appTitle: modelData.name
                                x: modelData.x || 50 + index * 30
                                y: modelData.y || 50 + index * 30
                                width: modelData.width || 400
                                height: modelData.height || 300
                                
                                onClosed: {
                                    if (conversationModel) {
                                        conversationModel.closeApp(appId)
                                    }
                                }
                                
                                // App content loader
                                Loader {
                                    anchors.fill: parent
                                    anchors.margins: 1
                                    
                                    property var appModelData: modelData
                                    
                                    sourceComponent: {
                                        if (modelData.type === "chat") {
                                            return chatComponent
                                        } else if (modelData.type === "editor") {
                                            return editorComponent
                                        } else if (modelData.type === "script") {
                                            return scriptComponent
                                        } else {
                                            return placeholderComponent
                                        }
                                    }
                                    
                                    onLoaded: {
                                        // Pass the model data to the loaded component
                                        if (item && appModelData) {
                                            item.conversationId = conversationCanvas.conversationId
                                            
                                            if (appModelData.type === "script") {
                                                item.appName = appModelData.name || "Script App"
                                                item.appId = appModelData.instanceId || ""
                                                item.scriptAppModel = appModelData.appModel || null
                                            } else if (appModelData.type === "chat") {
                                                // Pass chat app model if needed
                                                if (appModelData.appModel) {
                                                    item.chatModel = appModelData.appModel
                                                }
                                            } else if (appModelData.type === "editor") {
                                                // Pass editor app model if needed
                                                if (appModelData.appModel) {
                                                    item.editorModel = appModelData.appModel
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        // Help text when no apps are open
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Double-click an app in the sidebar to launch it")
                            color: "#666666"
                            font.pixelSize: 14
                            visible: !conversationModel || conversationModel.activeApps.length === 0
                        }
                    }
                }
            }
        }
    }
    
    // App components
    Component {
        id: chatComponent
        
        ChatApp {
            // Properties will be set by the parent Loader's onLoaded handler
        }
    }
    
    Component {
        id: editorComponent
        
        AppEditor {
            // Properties will be set by the parent Loader's onLoaded handler
        }
    }
    
    Component {
        id: scriptComponent
        
        ScriptApp {
            // Properties will be set by the parent Loader's onLoaded handler
        }
    }
    
    Component {
        id: placeholderComponent
        
        Rectangle {
            color: "#2b2b2b"
            
            Text {
                anchors.centerIn: parent
                text: qsTr("App content")
                color: "#666666"
                font.pixelSize: 14
            }
        }
    }
}