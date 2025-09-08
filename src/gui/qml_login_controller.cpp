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

#include "gui/qml_login_controller.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

namespace us = fire::user;
namespace sc = fire::security;

namespace fire
{
    namespace gui
    {
        QmlLoginController::QmlLoginController(const std::string& home, QObject* parent)
            : QObject(parent),
              _home(home),
              _is_new_user(!us::user_created(home)),
              _is_retry(false),
              _was_successful(false)
        {
            REQUIRE_FALSE(home.empty());
        }
        
        bool QmlLoginController::isNewUser() const
        {
            return _is_new_user;
        }
        
        bool QmlLoginController::isRetry() const
        {
            return _is_retry;
        }
        
        void QmlLoginController::setIsRetry(bool retry)
        {
            if (_is_retry != retry)
            {
                _is_retry = retry;
                emit isRetryChanged();
            }
        }
        
        user::local_user_ptr QmlLoginController::getUser() const
        {
            return _user;
        }
        
        bool QmlLoginController::wasSuccessful() const
        {
            return _was_successful;
        }
        
        void QmlLoginController::setName(const QString& name)
        {
            _name = name.toStdString();
        }
        
        void QmlLoginController::setPassword(const QString& password)
        {
            _password = password.toStdString();
        }
        
        void QmlLoginController::setPasswordConfirm(const QString& passwordConfirm)
        {
            _password_confirm = passwordConfirm.toStdString();
        }
        
        void QmlLoginController::createUser()
        {
            try
            {
                if (_name.empty())
                {
                    emit loginFailed(tr("Name cannot be empty"));
                    return;
                }
                
                if (_password.empty())
                {
                    emit loginFailed(tr("Password cannot be empty"));
                    return;
                }
                
                if (_password != _password_confirm)
                {
                    emit loginFailed(tr("Passwords do not match"));
                    return;
                }
                
                auto key = std::make_shared<sc::private_key>(_password);
                _user = std::make_shared<us::local_user>(_name, key);
                us::save_user(_home, *_user);
                
                _was_successful = true;
                emit loginSuccessful();
                
                ENSURE(_user);
            }
            catch (const std::exception& e)
            {
                LOG << "Error creating user: " << e.what() << std::endl;
                emit loginFailed(tr("Failed to create user: %1").arg(e.what()));
            }
        }
        
        void QmlLoginController::login()
        {
            try
            {
                if (_password.empty())
                {
                    emit loginFailed(tr("Password cannot be empty"));
                    return;
                }
                
                _user = us::load_user(_home, _password);
                _was_successful = true;
                emit loginSuccessful();
                
                ENSURE(_user);
            }
            catch (const std::exception& e)
            {
                LOG << "Error loading user: " << e.what() << std::endl;
                setIsRetry(true);
                emit loginFailed(tr("Invalid password"));
            }
        }
        
        void QmlLoginController::cancel()
        {
            _was_successful = false;
            _user.reset();
            emit cancelled();
        }
    }
}