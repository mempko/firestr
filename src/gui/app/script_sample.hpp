/*
 * Copyright (C) 2012  Maxim Noah Khailo
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

#ifndef FIRESTR_APP_SCRIPT_SAMPLE_H
#define FIRESTR_APP_SCRIPT_SAMPLE_H

#include "gui/list.hpp"
#include "gui/message.hpp"
#include "session/session.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"

#include <QObject>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSignalMapper>

#include "slb/SLB.hpp"

#include <string>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            class api_impl;
            struct button_ref
            {
                std::string id;
                std::string text;
                std::string callback;

                const std::string& get_text() const { return text;}
                void set_text(const std::string&);

                const std::string& get_callback() const { return callback;}
                void set_callback(const std::string&);  

                bool enabled(); 
                void enable();
                void disable();

                api_impl* api;
            };

            typedef std::map<std::string, button_ref> button_ref_map;
            typedef std::map<std::string, QPushButton*> button_widget_map;
            typedef std::shared_ptr<SLB::Script> script_ptr;

            class api_impl : public QObject
            {
                Q_OBJECT

                public:
                    gui::list* output;
                    QWidget* canvas;
                    QGridLayout* layout;
                    SLB::Manager manager;
                    script_ptr state;

                    session::session_ptr session;
                    messages::sender_ptr sender;

                    button_ref_map button_refs;
                    button_widget_map button_widgets;
                    QSignalMapper* button_mapper;

                    void bind();
                    std::string execute(const std::string&);
                    void run(const std::string name, const std::string&);

                    //exposed functions
                    void print(const std::string& a);
                    button_ref button(const std::string& title, const std::string& callback, int r = 0, int c = 0);

                    public slots:
                        void button_clicked(QString id);
            };

            typedef std::shared_ptr<api_impl> api_impl_ptr;

            class script_sample : public message
            {
                Q_OBJECT

                public:
                    script_sample(session::session_ptr);
                    script_sample(const std::string& id, session::session_ptr);
                    ~script_sample();

                public:
                    const std::string& id();
                    const std::string& type();
                    fire::message::mailbox_ptr mail();

                public slots:
                    void send_script();
                    void check_mail();
                    void scroll_to_bottom(int min, int max);

                private:
                    void init();

                private:
                    std::string _id;
                    session::session_ptr _session;
                    fire::message::mailbox_ptr _mail;
                    messages::sender_ptr _sender;

                    QTextEdit* _script;
                    QPushButton* _run;

                    api_impl_ptr _api;
            };
            extern const std::string SCRIPT_SAMPLE;

        }
    }
}

#endif

