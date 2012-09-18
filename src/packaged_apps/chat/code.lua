m = app:list()
app:place_across(m, 0, 0, 1, 2)

e = app:edit("")
app:place(e, 1, 0)

s = app:button("send")
app:place(s, 1, 1)

s:when_clicked("send_message")
e:when_finished("send_message")

app:when_message_received("got")

function send_message()
    local text = e:text()
    if #text== 0 then
        return
    end
    msg = app:message()
    msg:set("text", text)
    app:send(msg)
    add_message("me", text)

    e:set_text("")
end

function got(msg)
    local from = msg:from()
    local name = from:name()
    add_message(name, msg:get("text"))
end

function add_message(name, text)
    local t = "<b>"..name..":</b> "..text
    local nl = app:label(t)
    m:add(nl)
end
