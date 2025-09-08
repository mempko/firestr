import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Dialog {
    id: showIdentityDialog
    title: qsTr("Your Identity")
    modal: true
    width: 500
    height: 450
    
    property alias identity: identityTextArea.text
    property var greeters: []
    property int selectedGreeter: 0
    
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
            text: qsTr("Share this identity with others so they can add you as a contact:")
            font.pixelSize: 16
            font.bold: true
            color: "#e0e0e0"  // Light text for dark background
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        
        RowLayout {
            Layout.fillWidth: true
            
            Label {
                text: qsTr("Greeter:")
                font.pixelSize: 14
                font.bold: true
                color: "#e0e0e0"  // Light text for dark background
            }
            
            ComboBox {
                id: greeterCombo
                Layout.fillWidth: true
                model: greeters
                currentIndex: selectedGreeter
                
                // Dark theme styling for ComboBox
                background: Rectangle {
                    color: "#3c3c3c"
                    border.color: greeterCombo.pressed ? "#4CAF50" : "#555555"
                    border.width: 1
                    radius: 4
                }
                
                contentItem: Text {
                    text: greeterCombo.displayText
                    color: "#e0e0e0"
                    font: greeterCombo.font
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 10
                }
                
                onCurrentIndexChanged: {
                    selectedGreeter = currentIndex
                }
            }
        }
        
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            TextArea {
                id: identityTextArea
                readOnly: true
                selectByMouse: true
                wrapMode: TextArea.Wrap
                font.family: "monospace"
                font.pixelSize: 13
                color: "#e0e0e0"  // Light text for dark background
                
                background: Rectangle {
                    color: "#3c3c3c"  // Dark input background
                    border.color: identityTextArea.focus ? "#4CAF50" : "#555555"
                    border.width: identityTextArea.focus ? 2 : 1
                    radius: 4
                }
            }
        }
        
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#555555"  // Darker line for dark theme
        }
        
        Button {
            text: qsTr("Copy to Clipboard")
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 180
            Layout.preferredHeight: 40
            
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
            
            onClicked: {
                identityTextArea.selectAll()
                identityTextArea.copy()
                copiedNotification.visible = true
                copiedTimer.restart()
            }
        }
        
        Text {
            id: copiedNotification
            text: qsTr("✓ Identity copied to clipboard")
            font.pixelSize: 13
            font.bold: true
            color: "#4CAF50"
            visible: false
            Layout.alignment: Qt.AlignHCenter
            
            Timer {
                id: copiedTimer
                interval: 2000
                onTriggered: copiedNotification.visible = false
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
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}