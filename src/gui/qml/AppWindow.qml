import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: appWindow
    
    property string appId: ""
    property string appTitle: "App"
    property alias content: contentArea.children
    property int minimumWidth: 200
    property int minimumHeight: 150
    
    signal closed()
    
    color: "#2b2b2b"
    border.color: activeFocus ? "#4CAF50" : "#3c3c3c"
    border.width: activeFocus ? 2 : 1
    radius: 4
    clip: true
    
    // Click to focus
    MouseArea {
        anchors.fill: parent
        onPressed: {
            appWindow.forceActiveFocus()
            mouse.accepted = false
        }
    }
    
    // Title bar
    Rectangle {
        id: titleBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 30
        color: appWindow.activeFocus ? "#3c3c3c" : "#333333"
        radius: 4
        
        // Title bar drag area
        MouseArea {
            id: dragArea
            anchors.fill: parent
            
            property real startX: 0
            property real startY: 0
            
            onPressed: {
                appWindow.forceActiveFocus()
                startX = mouse.x
                startY = mouse.y
            }
            
            onPositionChanged: {
                if (pressed) {
                    var newX = appWindow.x + (mouse.x - startX)
                    var newY = appWindow.y + (mouse.y - startY)
                    
                    // Keep window within bounds
                    newX = Math.max(0, Math.min(newX, appWindow.parent.width - appWindow.width))
                    newY = Math.max(0, Math.min(newY, appWindow.parent.height - appWindow.height))
                    
                    appWindow.x = newX
                    appWindow.y = newY
                }
            }
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 5
            
            // App icon/title
            Text {
                text: appTitle
                color: "#e0e0e0"
                font.pixelSize: 12
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            
            // Window controls
            Row {
                spacing: 5
                
                // Minimize button (placeholder)
                Rectangle {
                    width: 16
                    height: 16
                    radius: 8
                    color: minimizeMouseArea.containsMouse ? "#FFC107" : "#666666"
                    
                    MouseArea {
                        id: minimizeMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        
                        onClicked: {
                            // TODO: Implement minimize
                        }
                    }
                }
                
                // Maximize button (placeholder)
                Rectangle {
                    width: 16
                    height: 16
                    radius: 8
                    color: maximizeMouseArea.containsMouse ? "#4CAF50" : "#666666"
                    
                    MouseArea {
                        id: maximizeMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        
                        onClicked: {
                            // TODO: Implement maximize
                        }
                    }
                }
                
                // Close button
                Rectangle {
                    width: 16
                    height: 16
                    radius: 8
                    color: closeMouseArea.containsMouse ? "#F44336" : "#666666"
                    
                    MouseArea {
                        id: closeMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        
                        onClicked: {
                            appWindow.closed()
                        }
                    }
                    
                    Text {
                        anchors.centerIn: parent
                        text: "×"
                        color: "white"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }
            }
        }
    }
    
    // Content area
    Item {
        id: contentArea
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 1
    }
    
    // Resize handles
    
    // Right edge
    MouseArea {
        anchors.right: parent.right
        anchors.top: titleBar.bottom
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        width: 5
        cursorShape: Qt.SizeHorCursor
        
        onPositionChanged: {
            if (pressed) {
                var newWidth = appWindow.width + mouse.x
                appWindow.width = Math.max(minimumWidth, newWidth)
            }
        }
    }
    
    // Bottom edge
    MouseArea {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 10
        height: 5
        cursorShape: Qt.SizeVerCursor
        
        onPositionChanged: {
            if (pressed) {
                var newHeight = appWindow.height + mouse.y
                appWindow.height = Math.max(minimumHeight, newHeight)
            }
        }
    }
    
    // Bottom-right corner
    MouseArea {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 10
        height: 10
        cursorShape: Qt.SizeFDiagCursor
        
        onPositionChanged: {
            if (pressed) {
                var newWidth = appWindow.width + mouse.x
                var newHeight = appWindow.height + mouse.y
                appWindow.width = Math.max(minimumWidth, newWidth)
                appWindow.height = Math.max(minimumHeight, newHeight)
            }
        }
        
        // Visual indicator
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            
            Canvas {
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.strokeStyle = "#666666"
                    ctx.lineWidth = 1
                    
                    // Draw resize grip lines
                    ctx.beginPath()
                    ctx.moveTo(width - 2, height - 8)
                    ctx.lineTo(width - 8, height - 2)
                    ctx.stroke()
                    
                    ctx.beginPath()
                    ctx.moveTo(width - 2, height - 4)
                    ctx.lineTo(width - 4, height - 2)
                    ctx.stroke()
                }
            }
        }
    }
}