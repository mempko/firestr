import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Dialog {
    id: aboutDialog
    title: qsTr("About Fire★")
    modal: true
    width: 450
    height: 400
    
    background: Rectangle {
        color: "#2b2b2b"  // Dark grey background
        border.color: "#555555"
        border.width: 1
        radius: 4
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 20
        
        // Logo/Icon area
        Rectangle {
            id: logo
            color: "#4CAF50"
            Layout.alignment: Qt.AlignHCenter
            width: 80
            height: 80
            radius: 40
            
            Text {
                anchors.centerIn: parent
                text: "Fire★"
                color: "white"
                font.pixelSize: 20
                font.bold: true
            }
        }
        
        // Title
        Text {
            text: qsTr("Fire★ 0.11.1")
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: 24
            font.bold: true
            color: "#4CAF50"
        }
        
        Text {
            text: qsTr("The Grass Computing Platform")
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: 16
            color: "#e0e0e0"
        }
        
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#555555"
        }
        
        // Description
        Text {
            text: qsTr("A P2P platform for creating and sharing distributed applications.")
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: 14
            color: "#b0b0b0"
        }
        
        // Spacer
        Item {
            Layout.fillHeight: true
        }
        
        // Copyright and License
        Column {
            Layout.alignment: Qt.AlignHCenter
            spacing: 5
            
            Text {
                text: qsTr("Copyright © 2025 Maxim Noah Khailo")
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: 12
                color: "#999999"
            }
            
            Text {
                text: qsTr("Licensed under GPLv3")
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: 12
                color: "#999999"
            }
        }
        
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#555555"
        }
        
        // Links
        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: 20
            
            Text {
                text: qsTr("Website")
                font.pixelSize: 13
                color: "#4CAF50"
                font.underline: mouseAreaWebsite.containsMouse
                
                MouseArea {
                    id: mouseAreaWebsite
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: Qt.openUrlExternally("http://firestr.com")
                }
            }
            
            Text {
                text: "|"
                font.pixelSize: 13
                color: "#555555"
            }
            
            Text {
                text: qsTr("GitHub")
                font.pixelSize: 13
                color: "#4CAF50"
                font.underline: mouseAreaGithub.containsMouse
                
                MouseArea {
                    id: mouseAreaGithub
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: Qt.openUrlExternally("https://github.com/mempko/firestr")
                }
            }
        }
    }
    
    footer: DialogButtonBox {
        Button {
            text: qsTr("OK")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            
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
        }
    }
}