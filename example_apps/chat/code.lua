
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
