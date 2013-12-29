
s = app:button("send")
app:place(s, 0,0)
app:height(100)
row=1
send_id = 1
CHUNK=1024 * 40

sfiles = {}
gfiles = {}

s:when_clicked("send()")


function send()
	local file = app:open_bin_file()

	if not file:good() then
		return
	end

	local nf = { 
		id = send_id,
		name=file:name(), 
		data=file:data(), 
		size=file:size(), 
		chunk=0,
		chunks = math.ceil(file:size() / CHUNK),
		ch = {},
		mode=1}
	send_id = send_id + 1
	add_sfile(nf)
	send_start_file(nf)
end

function format_percent(p)
	p = p * 1000
	p = math.ceil(p)
	p = p / 10
	return p .. "%"
end

function g_percent(f)
	return format_percent(f.chunk / f.chunks)
end

function s_percent(f)
	local sc = f.chunks
	for k, v in pairs(f.ch) do
		if v < sc then
			sc = v
		end
	end
	return format_percent( sc / f.chunks)
end

function s_status(f)
	local s = ""
	local mode = f.mode
	local p = s_percent(f)
	if mode == 1 then
		s = "sending..."
	elseif mode == 2 then
		s = "... " .. p
	elseif mode == 3 then
		s = "done"
	end
	return f.name .. " ".. s
end

function g_status(f)
	local s = ""
	local mode = f.mode
	local p = g_percent(f)
	if mode == 1 then
		s = "getting..."
	elseif mode == 2 then
		s = "... " .. p
	elseif mode == 3 then
		s = "done"
	end
	return f.name .. " ".. s
end

function update_s_status(fd)
	local lb = fd.label
	local bt = fd.bt
	lb:set_text(s_status(fd.file))
end

function update_g_status(fd)
	local lb = fd.label
	local bt = fd.bt
	lb:set_text(g_status(fd.file))
	if fd.file.mode == 3 then
		bt:enable()
		bt:set_text("save")
		bt:when_clicked("save_file_by_id(\"".. fd.id .."\")")
	end
end

function add_sfile(f)
	local i = f.id
	row = row + 1

	local cv = app:grid()
	app:place(cv, row, 0)
	local fl= app:label(s_status(f))
	cv:place(fl, 0, 0)
	local bt = nil

	app:grow()
	local fd = {id=i, file=f, label=fl, cv=cv}
	sfiles[i] = fd
end


function add_gfile(f)
	local i = f.id
	row = row + 1

	local cv = app:grid()
	app:place(cv, row, 0)
	local fl= app:label(g_status(f))
	cv:place(fl, 0, 0)
	
	local bt= app:button("save")
	bt:disable()
	cv:place(bt, 0, 1)
	
	app:grow()
	local fd = {id=i, file=f, label=fl, bt=bt, cv=cv}
	gfiles[i] = fd
end


function send_start_file(f)
	local m = app:message()
	m:set("t","sf")
	m:set("name", f.name )
	m:set("id", f.id)
	m:set("size", f.size)
	m:set("chunks", f.chunks)
	app:send(m)
end

function got_start_file(m)
	
	local n = m:get("name")
	local orig_id = m:get("id") + 0
	local from = m:from()
	local id = from:id() .. "_" .. orig_id
	local chunks = m:get("chunks")  + 0
	local size = m:get("size")
	local nf = {
		from = from,
		orig_id = orig_id,
		id=id,
		name=n,
		data=d, 
		chunks=chunks,
		chunk=-1,
		size = size,
		mode=4}

	add_gfile(nf)
	send_get_chunk(nf)
end

function send_get_chunk(f)
	local m = app:message()
	local chunk = f.chunk + 1
	m:set("t","gc")
	m:set("id", f.orig_id)
	m:set("chunk", chunk)

	app:send_to(f.from, m)
end

function got_get_chunk(m)
	local id = m:get("id") + 0
	local chunk = m:get("chunk") + 0
	local f = sfiles[id].file

	send_chunk(m:from(), f, chunk)
end

function sent_all(f)
	local all = true
	for k,v in pairs(f.ch) do
		if v < (f.chunks - 1) then
			all = false
		end
	end
	return all
end

function send_chunk(to, f, c)
	local b = c * CHUNK
	if b >= f.size then return end
	local s = math.min(CHUNK, (f.size - b))
	local ch = f.data:sub(b, s)

	f.ch[to:id()] = c
	if sent_all(f) then
		f.mode = 3
	else
		f.mode = 2
	end

	local m = app:message()
	m:set("t", "sc")
	m:set("id", f.id)
	m:set("chunk", c)
	m:set_bin("data", ch)
	app:send_to(to, m)
	update_s_status(sfiles[f.id])
end

function got_chunk(m)
	local orig_id = m:get("id") + 0
	local id = m:from():id() .. "_" .. orig_id
	local chunk = m:get("chunk") + 0

	local chunk_data = m:get_bin("data")

	local fd = gfiles[id]
	local file = fd.file

	if chunk < file.chunk then
		return
	end

	if file.data == nil then
		file.data = chunk_data
	else
		file.data:append(chunk_data)
	end
	file.chunk = chunk

	local last_chunk = file.chunks  - 1

	if file.chunk == last_chunk then
		file.mode = 3
		update_g_status(fd)
		return
	end

	file.mode = 2
	update_g_status(fd)

	send_get_chunk(file)
	
end

app:when_message_received("got")
function got(m)
	
	local t = m:get("t")

	if t == "sf" then
		got_start_file(m)
	elseif t == "gc" then
		got_get_chunk(m)
	elseif t == "sc" then
		got_chunk(m)
	end
end

function save_file_by_id(gid)
	local f = gfiles[gid].file
	save_file(f)
end

function save_file(f)
	f.saved = 1
	app:save_bin_file(f.name, f.data)
end

