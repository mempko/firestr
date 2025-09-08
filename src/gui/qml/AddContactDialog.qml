import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Dialog {
    id: addContactDialog
    title: qsTr("Add Contact")
    modal: true
    width: 500
    height: 400
    
    property alias identityText: identityTextArea.text
    
    signal addContact(string identity)
    
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
            text: qsTr("Paste the contact's identity below:")
            font.pixelSize: 16
            font.bold: true
            color: "#e0e0e0"  // Light text for dark background
        }
        
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            TextArea {
                id: identityTextArea
                placeholderText: qsTr("Contact identity...")
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
        
        Text {
            text: qsTr("To get someone's identity, they need to use Identity → Show Identity")
            font.pixelSize: 13
            color: "#b0b0b0"  // Light gray for dark background
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
    
    footer: DialogButtonBox {
        Button {
            text: qsTr("Add")
            enabled: identityTextArea.text.length > 0
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            
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
                font.pixelSize: 14
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
                color: "#e0e0e0"  // Light text for dark background
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
    
    onAccepted: {
        if (identityTextArea.text.length > 0) {
            addContact(identityTextArea.text)
        }
    }
    
    onRejected: {
        identityTextArea.text = ""
    }
}