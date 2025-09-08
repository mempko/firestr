import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: loginWindow
    visible: true
    width: 500
    height: 600
    title: qsTr("Fire★ - Identity Setup")
    color: "#f0f0f0"
    
    property bool isNewUser: loginController.isNewUser
    property bool isRetry: loginController.isRetry
    
    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width * 0.8
        spacing: 20
        
        // Logo placeholder
        Rectangle {
            id: logo
            color: "#4CAF50"
            Layout.alignment: Qt.AlignHCenter
            width: 100
            height: 100
            radius: 50
            
            Text {
                anchors.centerIn: parent
                text: "Fire★"
                color: "white"
                font.pixelSize: 24
                font.bold: true
            }
        }
        
        // Welcome text
        Text {
            text: qsTr("The Grass Computing Platform")
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: 24
            font.bold: true
            color: "#333333"
        }
        
        Text {
            text: qsTr("Fire★")
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: 18
            color: "#4CAF50"
        }
        
        // Dynamic title based on mode
        Text {
            text: {
                if (isRetry) {
                    return qsTr("Invalid Password")
                } else if (isNewUser) {
                    return qsTr("Create Your Identity")
                } else {
                    return qsTr("Enter Your Password")
                }
            }
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: 16
            font.bold: true
            color: isRetry ? "#f44336" : "#2196F3"
        }
        
        // Info points for new user
        Column {
            visible: isNewUser
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: parent.width
            spacing: 5
            
            Text {
                text: "• The identity is stored on this computer"
                color: "#666666"
                font.pixelSize: 12
            }
            Text {
                text: "• Your data is not stored anywhere else"
                color: "#666666"
                font.pixelSize: 12
            }
            Text {
                text: "• The password protects your identity"
                color: "#666666"
                font.pixelSize: 12
            }
        }
        
        // Form fields
        Column {
            Layout.alignment: Qt.AlignHCenter
            spacing: 15
            width: 300
            
            // Name field (only for new users)
            Column {
                visible: isNewUser
                width: parent.width
                spacing: 5
                
                Text {
                    text: qsTr("Name")
                    color: "#666666"
                    font.pixelSize: 12
                }
                
                TextField {
                    id: nameField
                    width: parent.width
                    height: 40
                    placeholderText: qsTr("Enter your name")
                    selectByMouse: true
                    font.pixelSize: 14
                    color: "#333333"
                    
                    background: Rectangle {
                        color: "white"
                        border.color: {
                            if (nameField.activeFocus) {
                                return "#2196F3"
                            } else if (nameField.text.length > 0) {
                                return "#4CAF50"
                            } else {
                                return "#dddddd"
                            }
                        }
                        border.width: nameField.activeFocus ? 2 : 1
                        radius: 4
                    }
                    
                    onTextChanged: {
                        loginController.setName(text)
                        validateInput()
                    }
                }
            }
            
            // Password field
            Column {
                width: parent.width
                spacing: 5
                
                Text {
                    text: qsTr("Password")
                    color: "#666666"
                    font.pixelSize: 12
                }
                
                TextField {
                    id: passwordField
                    width: parent.width
                    height: 40
                    echoMode: TextInput.Password
                    placeholderText: qsTr("Enter password")
                    selectByMouse: true
                    font.pixelSize: 14
                    color: "#333333"
                    
                    background: Rectangle {
                        color: "white"
                        border.color: {
                            if (passwordField.activeFocus) {
                                return "#2196F3"
                            } else if (passwordField.text.length > 0) {
                                return "#4CAF50"
                            } else {
                                return "#dddddd"
                            }
                        }
                        border.width: passwordField.activeFocus ? 2 : 1
                        radius: 4
                    }
                    
                    onTextChanged: {
                        loginController.setPassword(text)
                        if (isNewUser) {
                            passwordConfirmField.text = ""
                        }
                        validateInput()
                    }
                    
                    Keys.onReturnPressed: {
                        if (!isNewUser && passwordField.text.length > 0) {
                            loginController.login()
                        }
                    }
                }
            }
            
            // Confirm password field (only for new users)
            Column {
                visible: isNewUser
                width: parent.width
                spacing: 5
                
                Text {
                    text: qsTr("Confirm Password")
                    color: "#666666"
                    font.pixelSize: 12
                }
                
                TextField {
                    id: passwordConfirmField
                    width: parent.width
                    height: 40
                    echoMode: TextInput.Password
                    placeholderText: qsTr("Confirm password")
                    selectByMouse: true
                    font.pixelSize: 14
                    color: "#333333"
                    
                    background: Rectangle {
                        color: "white"
                        border.color: {
                            if (passwordConfirmField.activeFocus) {
                                return "#2196F3"
                            } else if (passwordConfirmField.text === passwordField.text && 
                                passwordConfirmField.text.length > 0) {
                                return "#4CAF50"
                            } else if (passwordConfirmField.text.length > 0) {
                                return "#f44336"
                            } else {
                                return "#dddddd"
                            }
                        }
                        border.width: passwordConfirmField.activeFocus ? 2 : 1
                        radius: 4
                    }
                    
                    onTextChanged: {
                        loginController.setPasswordConfirm(text)
                        validateInput()
                    }
                    
                    Keys.onReturnPressed: {
                        if (isValidInput()) {
                            loginController.createUser()
                        }
                    }
                }
            }
        }
        
        // Spacer
        Item {
            Layout.fillHeight: true
            Layout.minimumHeight: 20
        }
        
        // Action buttons
        Column {
            Layout.alignment: Qt.AlignHCenter
            spacing: 10
            width: 300
            
            Button {
                id: actionButton
                text: isNewUser ? qsTr("Create Identity") : qsTr("Login")
                width: parent.width
                height: 45
                enabled: isValidInput()
                font.pixelSize: 14
                font.bold: true
                
                background: Rectangle {
                    color: {
                        if (!actionButton.enabled) {
                            return "#cccccc"
                        } else if (actionButton.pressed) {
                            return "#1976D2"
                        } else if (actionButton.hovered) {
                            return "#42A5F5"
                        } else {
                            return "#2196F3"
                        }
                    }
                    radius: 4
                }
                
                contentItem: Text {
                    text: actionButton.text
                    color: actionButton.enabled ? "white" : "#999999"
                    font: actionButton.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    if (isNewUser) {
                        loginController.createUser()
                    } else {
                        loginController.login()
                    }
                }
            }
            
            Button {
                id: cancelButton
                text: qsTr("Cancel")
                width: parent.width
                height: 35
                font.pixelSize: 12
                
                background: Rectangle {
                    color: {
                        if (cancelButton.pressed) {
                            return "#d32f2f"
                        } else if (cancelButton.hovered) {
                            return "#ef5350"
                        } else {
                            return "transparent"
                        }
                    }
                    border.color: "#ef5350"
                    border.width: 1
                    radius: 4
                }
                
                contentItem: Text {
                    text: cancelButton.text
                    color: cancelButton.hovered ? "white" : "#ef5350"
                    font: cancelButton.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: loginController.cancel()
            }
        }
    }
    
    function isValidInput() {
        if (isNewUser) {
            return nameField.text.length > 0 &&
                   passwordField.text.length > 0 &&
                   passwordField.text === passwordConfirmField.text
        } else {
            return passwordField.text.length > 0
        }
    }
    
    function validateInput() {
        // This triggers re-evaluation of button enabled state
        actionButton.enabled = isValidInput()
    }
}