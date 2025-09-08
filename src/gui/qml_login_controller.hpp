/*
 * Copyright (C) 2025  Maxim Noah Khailo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FIRESTR_GUI_QML_LOGIN_CONTROLLER_H
#define FIRESTR_GUI_QML_LOGIN_CONTROLLER_H

#include "user/user.hpp"
#include "security/security.hpp"

#include <QObject>
#include <QString>
#include <memory>

namespace fire
{
    namespace gui
    {
        class QmlLoginController : public QObject
        {
            Q_OBJECT
            Q_PROPERTY(bool isNewUser READ isNewUser NOTIFY isNewUserChanged)
            Q_PROPERTY(bool isRetry READ isRetry WRITE setIsRetry NOTIFY isRetryChanged)
            
        public:
            explicit QmlLoginController(const std::string& home, QObject* parent = nullptr);
            
            bool isNewUser() const;
            bool isRetry() const;
            void setIsRetry(bool retry);
            
            user::local_user_ptr getUser() const;
            bool wasSuccessful() const;
            
        public slots:
            void setName(const QString& name);
            void setPassword(const QString& password);
            void setPasswordConfirm(const QString& passwordConfirm);
            
            void createUser();
            void login();
            void cancel();
            
        signals:
            void isNewUserChanged();
            void isRetryChanged();
            void loginSuccessful();
            void loginFailed(const QString& error);
            void cancelled();
            
        private:
            std::string _home;
            std::string _name;
            std::string _password;
            std::string _password_confirm;
            bool _is_new_user;
            bool _is_retry;
            bool _was_successful;
            user::local_user_ptr _user;
        };
    }
}

#endif // FIRESTR_GUI_QML_LOGIN_CONTROLLER_H