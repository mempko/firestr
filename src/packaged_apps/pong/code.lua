
W = 500
H = 500
PW = 10
PH = 100
PHH = PH / 2
PDW=20
PDH=110
BW = 30 
BH = 30
BDW = 50
BDH = 50
BHW = BW / 2
BHH = BH / 2
SPEEDUP = 1.1

V_BUFFER = 15
TOP=V_BUFFER
BOTTOM= H-V_BUFFER
lc = app:i_started()

ball = {x = W / 2, y = H/2, vx=0, vy=0, px = 0, py = 0, mc = 0}
pdl = app:image(data:get_bin("paddle.png"))
bg = app:image(data:get_bin("background.png"))
ball_image = app:image(data:get_bin("ball.png"))
d = app:draw(W,H)
app:height(H + 20)

d:when_mouse_moved("mouse_moved")
d:when_mouse_pressed("mouse_pressed")

paddles = {}
paddle = {}

function make_paddle(x, type)
	local p = {}
	p.x = x
	p.y = H / 2
	p.py = y
	p.type = type
	paddles[type] = p
end

function reset_game()
	ball = {x = W / 2, y = H/2, vx=0, vy=0, px = 0, py = 0, mc = 0}
	send_reset()
end

function draw()
	d:clear()
	d:image(bg, 0, 0, W, H)
	for k,p in pairs(paddles) do
		d:image(pdl, p.x, p.y-PHH, PDW, PDH) 
	end
	d:image(ball_image, ball.x-BHW, ball.y-BHH, BDW, BDH)
end

function update()
	send_ball()
	update_ball()
	draw()
end

function update_ball()
	ball.px = ball.x
	ball.py = ball.y
	ball.x = ball.x + ball.vx
	ball.y = ball.y + ball.vy
	if ball.y - BHH < TOP then
		ball.y = TOP + BHH
		ball.vy = -ball.vy
	elseif ball.y + BHH > BOTTOM then
		ball.y = BOTTOM - BHH
		ball.vy = -ball.vy
	end
	local lp = paddles.l
	local rp = paddles.r

	if ball.x -BHW < PW and ball.y > lp.y - PHH and ball.y < lp.y + PHH then
		ball.x = PW + BHW
		ball.vx = -ball.vx * SPEEDUP
		local dy = (ball.y - lp.y) / PHH
		ball.vy = ball.vy + dy
	elseif ball.x + BHW > rp.x - PW and ball.y > rp.y - PHH and ball.y < rp.y + PHH then
		ball.x = rp.x - PW - BHW
		ball.vx = -ball.vx * SPEEDUP
		local dy = (ball.y - rp.y) / PHH
		ball.vy = ball.vy + dy
	end 

	if ball.x -BHW < 0 or ball.x +BHW> W and lc then
		reset_game()
	end
end

function init_game()
	app:place(d, 0, 0)

	make_paddle(10, "l" )
	make_paddle(W-PW-10, "r")
	if lc then
		paddle = paddles.l
	else
		paddle = paddles.r
	end
	t = app:timer(33, "update()")

end

function mouse_moved(x, y)
	update_paddle(paddle, y)
	send_paddle()
end

function update_paddle(p, y)
	local ny = y
	if ny - PHH < TOP then
		ny = PHH + TOP
	end
	if ny + PHH > BOTTOM then
		ny =  BOTTOM - PHH
	end
	p.y = ny
end

function mouse_pressed(button, x, y)
	if not lc then return end
	ball.x = W/2
	ball.y = H/2
	ball.vx = 2;
	ball.vy = 5;
end

function send_paddle()
	if paddle.py == paddle.y then  return end
	local m = app:message()
	m:not_robust()
	m:set_type("p")
	m:set("pos", {t = paddle.type, y = paddle.y})
	app:send(m)
	paddle.py = y
end

function send_ball()
	
	if not lc then return end
	if ball.px == ball.x and ball.py == ball.y then return end
	ball.mc = ball.mc + 1
	local m  = app:message()
	m:not_robust()
	m:set_type("b")
	m:set("b" , ball)
	app:send(m)
end

function send_reset()
	if not lc then return end
	local m = app:message()
	m:set_type("r")
	app:send(m)
end

app:when_message("p", "got_paddle")
app:when_message("b", "got_ball")
app:when_message("r", "got_reset")


function got_paddle(m)

		local pos = m:get("pos")
		update_paddle(paddles[pos.t], pos.y)
end

function got_ball(m)
		local rball = m:get("b")
		if rball.mc <= ball.mc then return end
		local px = ball.x
		local py = ball.y
		rball.x = rball.x + rball.vx
		rball.y = rball.y + rball.vy
		ball = rball
		ball.x = (ball.x + px) / 2
		ball.y = (ball.y  + py) / 2
		ball.vx = (ball.vx + rball.vx) / 2
		ball.vy = (ball.vy + rball.vy) / 2
end		

function got_reset(m)
	reset_game()
end

if app:total_contacts() > 2 then
	l = app:label("<big>This is only a 2 player game</big>")
	app:place(l, 0, 0)
else
	init_game()
end


