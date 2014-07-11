m = app:mic("got_sound", "opus")
s = app:speaker("opus")
m:start()

muteb = app:button("mute mic")
app:place(muteb, 0, 0)
muteb:when_clicked("mute()")

smuteb = app:button("mute speaker")
app:place(smuteb, 0,1)
smuteb:when_clicked("smute()")

talkers = {}
row = 2
update_timer = app:timer(1000, "update()")

function init_talker(id, name)
	if talkers[id] == nil then  
		local l = app:label(name.." talking")
		app:place(l, row, 0)
		row = row + 1
		talkers[id] = {n=name, ticks = 0, label=l}
	end
end


function got_sound(d)
	local m = app:message()
	m:set_type("s")
	m:set_bin("d", d)
	app:send(m)
end

app:when_message("s", "play_sound")
function play_sound(m)
	s:play(m:get_bin("d"))
	talked(m:from():id(), m:from():name())
end

function mute()
	m:stop()
	muteb:when_clicked("unmute()")
	muteb:set_text("unmute mic")
end

function unmute()
	m:start()
	muteb:when_clicked("mute()")
	muteb:set_text("mute mic")
end

function smute()
	s:mute()
	smuteb:when_clicked("sunmute()")
	smuteb:set_text("unmute speaker")
end

function sunmute()
	s:unmute()
	smuteb:when_clicked("smute()")
	smuteb:set_text("mute speaker")
end

function talked(id, name)
	if #id == 0 then return end
	init_talker(id, name)
	local u = talkers[id]
	if u.ticks > 1 then
		u.label:set_text(u.n.." talking")
	end
	u.ticks = 0
end

function update()
	for id,u in pairs(talkers) do
		u.ticks = u.ticks + 1
		if u.ticks > 1 then
			u.label:set_text(u.n.." not talking")
		end
	end
end
