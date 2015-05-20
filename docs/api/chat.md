Simple Chat
================

This tutorial will show you how to program a simple chat App with Fire★. 
We will create an edit box where you can create your message and a button to send it.

If you have not created an App before, please see the [Hello World](hello_world.md) tutorial.

Setup
---------------

Create a new conversation and start the App Editor via the App menu.
The first thing that needs to be defined is the minimum size of our App. Type the following
into the App Editor.

    app:height(200)
    app:width(300)

Making the Widgets
---------------

The size is specified in pixels. There needs to be a place where the messages will be.
There are many approaches we can try but for now lets use the `list` object.

    msgs = app:list()
    app:place_across(msgs, 0, 0, 1, 2)

Widgets are placed in an invisible grid. I used `app:place_across` here because I want
the `messages` widget to span across 1 row and 2 columns. Lets define an edit box where
the user can type in their messages and a button that they can click to send the message.

    ed = app:edit("")
    app:place(ed, 1, 0)

    snd = app:button("send")
    app:place(snd, 1, 1)

Handling The Events
---------------

Now that we have an edit box and button, we need them to do something. What we want is when 
the user clicks the button or presses enter on the edit box, the message that is typed in
is sent.

    snd:when_clicked("clicked_send_message()")
    ed:when_finished("send_message")

In this case, we will have both actions do the same thing, which is send a message. Lets
define that function now.

    function clicked_send_message()
        send_message(ed:text())
    end

    function send_message(text)
        if #text== 0 then
            return
        end

        msg = app:message()
        msg:set("text", text)
        app:send(msg)
        add_message("me", text)
        ed:set_text("")
    end

The important part is that within the `send_message` function, we create a `message` object
and use the `app:send` function to send it. A `message` object can Lua table objects which
can be sent along with the message. You can set as many key/value pairs as you want within
a message. 

The `app:send` function will send the message to everyone in the conversation. Along with
sending the message, we want to add the same message into our message list.

    function add_message(name, text)
        local t = "<b>"..name..":</b> "..text
        local nl = app:label(t)
        msgs:add(nl)
        app:alert()
    end

Here a label is created with the name of the person who sent the message and the text
of the message. You can use primitive HTML inside a label. The label is added to the 
`list` object we defined earlier. There is also a call to `app:alert`, which will highlight
the tab to alert the user when a new message is there when they don't have the app focused.

Handling Messages
---------------

The last bit is handling what we do when we get the message.

    app:when_message_received("got")

    function got(msg)
        local from = msg:from()
        local name = from:name()
        add_message(name, msg:get("text"))
    end

The `app:when_message_received` call registers the function that is called when a message
is received. Messages can have types, but in this case since we only have one type of message
`app:when_message_received` is a catch all for all messages.

Code Listing
-----------------------

The full App code is listed below.

    app:height(200)
    app:width(300)

    msgs = app:list()
    app:place_across(msgs, 0, 0, 1, 2)

    ed = app:edit("")
    app:place(ed, 1, 0)

    snd = app:button("send")
    app:place(snd, 1, 1)

    snd:when_clicked("clicked_send_message()")
    ed:when_finished("send_message")

    app:when_message_received("got")

    function clicked_send_message()
        send_message(ed:text())
    end

    function send_message(text)
        if #text== 0 then
            return
        end

        msg = app:message()
        msg:set("text", text)
        app:send(msg)
        add_message("me", text)

        ed:set_text("")
    end

    function got(msg)
        local from = msg:from()
        local name = from:name()
        add_message(name, msg:get("text"))
    end

    function add_message(name, text)
        local t = "<b>"..name..":</b> "..text
        local nl = app:label(t)
        msgs:add(nl)
        app:alert()
    end
