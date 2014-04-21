function init_color(col, pt)
	local c = {}
	c.name=col
	c.button = app:button("")
	c.button:set_image(app:image(data:get_bin(col..".png")))
	c.pen = app:pen(col, 2)
	pens[pt] = c.pen
	c.button:when_clicked("set_color('"..pt.."' )")
	return c
end

d = app:draw(300,300)
pens={}
black = init_color("black", "bl")
red = init_color("red", "r")
green = init_color("green", "g")
blue = init_color("blue", "b")
brown = init_color("brown", "br")

cl = app:button("clear")

app:place_across(d,0,0,1,6)
app:place(black.button, 1,0)
app:place(red.button,1,1)
app:place(green.button,1,2)
app:place(blue.button,1,3)
app:place(brown.button,1,4)
app:place(cl,1,5)

d:when_mouse_pressed("pressed")
d:when_mouse_dragged("dragged")
cl:when_clicked("clear()")


px = -1
py = -1

cur_pen = ""
function set_color(pt)
	d:pen(pens[pt])
	cur_pen = pt
end
set_color("bl")

function pressed(b, x, y)
	px = x
	py = y
end

function dragged(b, x, y)
	d:line(px, py, x , y)
	send_line(px, py, x, y,cur_pen)
	px = x
	py = y
end

function clear()
	d:clear()
	send_clear()
end

app:when_message("l", "got_line")
app:when_message("cl", "got_clear")

function got_line(m)
	local l = m:get("ln")
	local prev_pen = d:get_pen()
	local cp = pens[l.p]
	d:pen(cp)
	d:line(l.xs, l.ys, l.xe, l.ye)
	d:pen(prev_pen)
end

function got_clear(m)
	d:clear()		
end

function send_line(x1,y1,x2,y2, pen)
	local m = app:message()
	m:set_type("l")
	m:set("ln", {xs=x1,ys=y1,xe=x2,ye=y2,p=pen})
	app:send(m)
end

function send_clear()
	local m = app:message()
	m:set_type("cl")
	app:send(m)
end


