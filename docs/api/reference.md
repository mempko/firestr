API Reference
=====

Fire★ Apps are built using the [Lua](http://lua.org) programming language. Fire★ provides a
custom object oriented API for building Apps. The main object you interact with is the [app](reference.md#app) object

app object
=====

The **app** object is a global object and provides functions for creating all other objects in an app. 
It also provides other functions such as sending messages and handling them. This is the
entry point to the Fire★ API.

There is only one **app** object.

app:print
-----

    print(message:string) : nil

Prints a string to the app editor log and Fire★ log. Newline is added automatically.

    app:print("this is a debug message")

app:alert
-----

    alert() : nil

If the user does not have focus in Fire★ or they are in another conversation, then the
conversation label turns red and the user is alerted through their operating system that
there is something they need to pay attention to.

app:button
-----

    button(label:string) : button

Creates a [button](reference.md#button-object) with text specified. The button must be 
placed on the canvas using the [place](reference.md#appplace) function.

    b = app:button("hello world")
    app:place(b, 0, 0)

app:label
-----

    label(text:string) : label

Creates a [label](reference.md#label-object) with text specified. The label must be 
placed on the canvas using the [place](reference.md#appplace) function.

    lb = app:label("cool app")
    app:place(lb, 0, 0)

app:edit
-----

    edit(text:string) : edit

Creates a single line [edit](reference.md#edit-object) object with text specified. 
The edit object must be placed on the canvas using the [place](reference.md#appplace) function.

    e = app:edit("type here")
    app:place(e, 0, 0)

app:text_edit
-----

    text_edit(text:string) : text_edit

Creates a multi-line [text_edit](reference.md#text_edit-object) object with text specified. 
The text_edit object must be placed on the canvas using the [place](reference.md#appplace) function.

    t = app:text_edit("type many things here")
    app:place(t, 0, 0)

app:list
-----

    list() : list

Creates an empty [list](reference.md#list-object) which you can use to add other widgets to. 
The list must be placed on the canvas using the [place](reference.md#appplace) function.

    lst = app:list()
    app:place(lst, 0, 0)

app:grid
-----

    grid() : grid

Creates an empty [grid](reference.md#grid-object) which you can use to add other widgets to. 
The grid must be placed on the canvas using the [place](reference.md#appplace) function.

The grid is similar to the canvas and allows you nest grids for laying out widgets.

    g = app:grid()
    app:place(g, 0, 0)

app:draw
-----

    draw() : draw

Creates a [draw](reference.md#draw-object) object which can be used to draw shapes to. 
The draw object must be placed on the canvas using the [place](reference.md#appplace) function.

    d = app:draw()
    app:place(d, 0, 0)

app:pen
-----

    pen(color:string, width:int) : pen

Creates a [pen](reference.md#pen-object) which is used by the [draw](reference.md#draw-object] object. 
The first parameter is a color. This can either be a color name or a hex rgb value. The second
parameter is the width of the pen in pixels.

    d = app:draw()

    r = app:pen("red", 3)
    d:set_pen(r)

app:timer
-----

    timer(msec:int, callback:string) : timer

Creates a [timer](reference.md#timer-object) which can be used to call a function every x milliseconds. 
The timer starts right away.

    t = app:timer(500, "foo()")

    function foo()
        app:print("foo")
    end

app:image
-----

    image(data:bin_data) : image

Creates an [image](reference.md#image-object) which can be used by various widgets to display pictures. 
An image takes binary image data as [bin_data](reference.md#bin_data-object). 
This can be JPEG, PNG, BMP, and many other formats.

    file = app:open_bin_file()
    img = app:image(file:data())

    app:place(img, 0,0)

app:mic
-----

    mic(callback:string, codec:string) : mic

Creates a [mic](reference.md#mic-object) object which can be used to get sound from a microphone. 
The callback must be a function of type

    callback(bin_data)

Where the sound data is passed in as a [bin_data](reference.md#bin_data) object.
The codec currently can either be "pcm" or "opus". "pcm" provides the callback raw audio
data as single channel with a sample rate of 12k and a sample size of 16 bytes. 
"opus" provides compressed audio data using the opus codec.

The mic and speaker object must match codecs if you want to hear sound. Otherwise you can
use the audio encoder and decoder to convert from/to pcm and opus.

    m = app:mic("got_sound", "opus")
    m:start()

    function got_sound(data)

    end

app:speaker
-----

    speaker(codec:string) : speaker

Creates a [speaker](reference.md#speaker-object) object which can be used to play sound. 

The codec currently can either be "pcm" or "opus". "pcm" provides the callback raw audio
data as single channel with a sample rate of 12k and a sample size of 16 bytes. 
"opus" provides compressed audio data using the opus codec.

The mic and speaker object must match codecs if you want to hear sound. Otherwise you can
use the audio encoder and decoder to convert from/to pcm and opus.
speaker

    s = app:speaker("opus") 

app:audio_encoder
-----

    audio_encoder() : audio_encoder

Creates an [audio_encoder](reference.md#audio_encoder-object) which can be used to convert pcm audio to opus.

    enc = app:audio_encoder()

app:audio_decoder
-----

    audio_decoder() : audio_decoder

Creates an [audio_decorder](reference.md#audio_decoder-object) which can be used to convert opus audio to pcm.

    enc = app:audio_decoder()

app:vclock
-----

    vclock()

Creates a [vclock](reference.md#audio_decoder-object) object which is a vector clock.
A vector clock can be used to determine if any messages were sent concurrently. 

    v = app:vclock()

app:place
-----

    place(widget, row:int, column:int) : nil

Places a widget on the canvas. The canvas is an invisible grid which you can place your widgets on.
A widget is not displayed until it is placed on the canvas.

    a = app:button("one")
    b = app:button("two")

    app:place(a, 0, 0)
    app:place(b, 1, 0)

app:place_across
-----

    place_across(widget, row:int, column:int, rows:int, columns:int) : nil

Places a widget on the canvas that can span across rows and columns. 
The canvas is an invisible grid which you can place your widgets on.
A widget is not displayed until it is placed on the canvas.

    a = app:button("one")
    b = app:button("two")
    c = app:button("three")

    app:place_across(a, 0, 0, 1, 2)
    app:place(b, 1, 0)
    app:place(c, 1, 1)

app:height
-----

    height(pixels:int) : nil

Changes the height of the app in pixels. The app cannot get smaller than the height specified.
Use this if you want more horizontal room.

app:total_contacts
-----

    total_contacts() : int

Returns the count of contacts in the conversation.

app:contact
-----

    contact(index:int) : contact

Returns the [contact](reference.md#contact-object) at the index specified in the conversation.

app:last_contact
-----

    last_contact() : int

Returns the index of the last contact in the conversation.

app:who_started
-----

    who_started() : contact

Returns the [contact](reference.md#contact-object) that added the App in the conversation.

app:self
-----

    self() : contact

Returns the [contact](reference.md#contact-object) information of user using the app.

app:total_apps
-----

    total_apps() : int

Returns the count of all the apps int the conversation.

app:app
-----

    app(index:int) : app

Returns the [app](reference.md#app-object) at the index specified in the conversation.

app:message
-----

    message() : message

Creates a [message](reference.md#message-object) which can be sent via the [send](reference.md#appsend) function.

    m = app:message()
    m:set_type('a')
    app:send(m)

app:when_message_received
-----

    when_message_received(callback:string) : nil

Sets the callback called when a message is received. The callback must be of the following form.

    function callback(message)
    end

The callback receives a [message](reference.md#message-object) which was sent from another
instance of the same App from a different contact

    app:when_message_received("foo")

    function foo(m)
    end

app:when_local_message_received
-----

    when_local_message_received(callback:string) : nil

Sets the callback called when a local message is received. 
A local message is one that is sent by another app from within the conversation from the
same instance. 

The callback must be of the following form.

    function callback(message)
    end

The callback receives a [message](reference.md#message-object) which was sent from another
instance of the same App from a different contact

    app:when_local_message_received("foo")

    function foo(m)
    end

app:when_message
-----

    when_message(type:string, callback:string) : nil

Sets the callback called when a message is received of a specific type. 
Messages can have a **type** which can be any string. You can capture messages of a specific
type with this callback 

The callback must be of the following form.

    function callback(message)
    end

The callback receives a [message](reference.md#message-object) which was sent from another
instance of the same App from a different contact

    app:when_message("foo", "bar")

    function bar(m)
    end

app:when_local_message
-----

    when_local_message(type:string, callback:string) : nil

Sets the callback called when a message is received of a specific type. 
Messages can have a **type** which can be any string. You can capture messages of a specific
type with this callback 

A local message is one that is sent by another app from within the conversation from the
same instance. 

The callback must be of the following form.

    function callback(message)
    end

The callback receives a [message](reference.md#message-object) which was sent from another
instance of the same App from a different contact

    app:when_local_message("foo", "bar")

    function bar(m)
    end

app:send
-----

    send(msg:message) : nil

Sends a message to the same app to all contacts in the conversation. Once the message 
arrives to the other contacts, their respective message callbacks are called.

    m = app:message()
    m:set_type("foo")

    app:send(m)

app:send_to
-----

    send_to(to:contact, msg:message) : nil

Sends a message to the same app to a specific contact in the conversation. Once the message 
arrives to the contact, their respective message callback is called.

    m = app:message()
    m:set_type("foo")

    c = app:contact(0)

    app:send_to(c, m)

app:send_local
-----

    send_local(msg:message) : nil

Sends a message to the all apps in the conversation locally. The message does not go over
the network. Once the message arrives to the other apps, their respective local message 
callbacks are called.

This is useful for having different apps communicate with each other.

    m = app:message()
    m:set_type("foo")

    app:send(m)

app:save_file
-----

    save_file(name:string, data:string) : nil

This opens up the file save dialog to allow a user to save some data to a file.
You cannot specify the file location for safety reasons. This always brings up the file
save dialog to allow the user to select the location of the file saved. 

The name you give to this function is a suggested name and the user is free to change it.

    app:save("hello.txt", "Hello World")

app:save_bin_file
-----

    save_bin_file(name:string, data:bin_data) : nil

This opens up the file save dialog to allow a user to save some binary data to a file.
You cannot specify the file location for safety reasons. This always brings up the file
save dialog to allow the user to select the location of the file saved. 

The name you give to this function is a suggested name and the user is free to change it.

    app:save_bin_file("hello.bin", bin_dat)

app:open_file
-----

    open_file() : file_data

This opens up the file open dialog to allow a user to load some data from a file.
You cannot specify the file location for safety reasons. This always brings up the file
open dialog to allow the user to select the file they want to open. 

The function returns a [file_data](reference.md#file_data-object) object which contains
the contents of the file the user selected.

The name you give to this function is a suggested name and the user is free to change it.

    data = app:open_file()

app:open_bin_file
-----

    open_bin_file() : bin_file_data

This opens up the file open dialog to allow a user to load some data from a file.
You cannot specify the file location for safety reasons. This always brings up the file
open dialog to allow the user to select the file they want to open. 

The function returns a [bin_file_data](reference.md#bin_file_data-object) object which contains
the contents of the binary file the user selected.

The name you give to this function is a suggested name and the user is free to change it.

    data = app:open_bin_file()

app:i_started
-----

    i_started() : bool

Returns true if the user of the instance started the app in the conversation. If the app
started from a remoted user, then this will return false.

button object
=====

A button can be clicked. When it is clicked things can happen. Use [app:button](reference.md#appbutton) 
to create a button.

button:text
-----

    text() : string

Returns the text that the button is displaying.

    t = my_button:text()

button:set_text
-----

    set_text(text:string) : nil

Sets the text the button will display.

    my_button:set_text("Press me now")

button:set_image
-----

    set_image(img:image) : nil

Sets an image instead of text to the button. You have to use an [image](reference.md#image-object)

    my_button:set_image(my_image)

button:callback
-----

    callback() : string

Returns the code that will run when the button is clicked. Use [when_clicked](reference.md#buttonwhen_clicked) 
to set the code that will be executed.

    code = my_button:callback()

button:when_clicked
-----

    when_clicked(code:string) : nil

Sets the code that will execute when the button is clicked.

    my_button:when_clicked("foo()")

    function foo()

    end

button:set_name
-----

    set_name(name:string) : nil

When you give a name to a widget, it has a life that is shared across all instances.
All events in one instance are propagated automatically to the others.

For the case of a button, if the button is clicked on one instance, it will be clicked
on all instances.

    my_button:set_name("some unique name")
    my_button:when_clicked("foo()")

    function foo()

    end

So if one user clicked on the button, then the "foo" function will be called for each user
in a conversation.

button:enabled
-----

    enabled() : bool

Returns true if the button is enabled

    en = my_button:enabled()

button:enable
-----

    enable() : nil

Enables the button.

    my_button:enable()

button:disable
-----

    disable : nil

Disables the button.

    my_button:disable()

label object
=====

A label can show text. Use labels to label things. Create a label the [app:label](reference.md#applabel).
function.

label:text
-----

    text() : string

Returns the text that the label is displaying.

    t = my_label:text()

label:set_text
-----

    set_text(text:string) : nil

Sets the text the label will display.

    my_label:set_text("Showing stuff, things")

label:enabled
-----

    enabled() : bool

Returns true if the label is enabled

    en = my_label:enabled()

label:enable
-----

    enable() : nil

Enables the label.

    my_label:enable()

label:disable
-----

    disable() : nil

Disables the label.

    my_label:disable()

edit object
=====

An edit object provides a single line for a user to type text in. Create one using the
[app:edit](reference.md#appedit) function.

edit:text
-----

    text() : string

Returns the text that the edit box is displaying.

    t = my_edit:text()

edit:set_text
-----

    set_text(text:string) : nil

Sets the text the edit box will display.

    my_edit:set_text("Press me now")

edit:edited_callback
-----

    edited_callback() : string

Returns the function that will run when the edit box text changes. Use [when_edited](reference.md#editwhen_edited) 
to set the function that will be executed.

    code = my_edit:edited_callback()

edit:when_edited
-----

    when_edited(func:string) : nil

Sets the function that will execute when the edit box text changes.
The callback must be of the form

    callback(text:string)

Where the text is the new text of the edit box

    my_edit:when_edited("foo")

    function foo(text)

    end

edit:finished_callback
-----

    finished_callback() : string

Returns the function that will run when the user leaves or presses enter in the edit box. Use [when_finished](reference.md#editwhen_finished) 
to set the function that will be executed.

    code = my_edit:finished_callback()

edit:when_finished
-----

    when_finished(func:string) : nil

Sets the function that will execute when the edit box text changes.
The callback must be of the form

    callback(text:string)

Where the text is the new text of the edit box.

    my_edit:when_finished("foo")

    function foo(text)

    end

edit:set_name
-----

    set_name(name:string) : nil

When you give a name to a widget, it has a life that is shared across all instances.
All events in one instance are propagated automatically to the others.

For the case of an edit box, when the edit box text changes on one instance, it will change
on all instances.

    my_edit:set_name("some unique name")
    my_edit:when_edited("foo")

    function foo(text)

    end

So if one user chnages the text int the edit box, then the "foo" function will be called for each user
in a conversation.

edit:enabled
-----

    enabled() : bool

Returns true if the edit is enabled

    en = my_edit:enabled()

edit:enable
-----

    enable() : nil

Enables the edit.

    my_edit:enable()

edit:disable
-----

    disable() : nil

Disables the edit.

    my_edit:disable()


text_edit object
=====

A text_edit object provides a multi line text editor for a user to type text in.
Use [app:text_edit](reference.md#apptext_edit) to create a text_edit object.

text_edit:text
-----

    text() : string

Returns the text that the text_edit box is displaying.

    t = my_text_edit:text()

text_edit:set_text
-----

    set_text(text:string) : nil

Sets the text the text_edit box will display.

    my_text_edit:set_text("Press me now")

text_edit:edited_callback
-----

    edited_callback() : string

Returns the function that will run when the text_edit box text changes. Use [when_edited](reference.md#text_editedwhen_edited) 
to set the function that will be executed.

    code = my_text_edit:edited_callback()

text_edit:when_edited
-----

    when_edited(func:string) : nil

Sets the function that will execute when the text_edit box text changes.
The callback must be of the form

    callback(text:string)

Where the text is the new text of the text_edit box

    my_text_edit:when_edited("foo")

    function foo(text)

    end

text_edit:set_name
-----

    set_name(name:string) : nil

When you give a name to a widget, it has a life that is shared across all instances.
All events in one instance are propagated automatically to the others.

For the case of an text_edit box, when the text_edit box text changes on one instance, it will change
on all instances.

    my_text_edit:set_name("some unique name")
    my_text_edit:when_text_edited("foo")

    function foo(text)

    end

So if one user chnages the text int the text_edit box, then the "foo" function will be called for each user
in a conversation.

text_edit:enabled
-----

    enabled() : bool

Returns true if the text_edit is enabled

    en = my_text_edit:enabled()

text_edit:enable
-----

    enable() : nil

Enables the text_edit.

    my_text_edit:enable()

text_edit:disable
-----

    disable() : nil

Disables the text_edit.

    my_text_edit:disable()


list object
=====

You can put widgets in a list which are displayed vertically. 
Use [app:list](reference.md#applist) to create a list.

list:add
-----

    add(object:widget) : nil

Adds a widget to the list.

    b = app:button("me me me")
    my_list:add(b)

list:remove
-----

    remove(object:widget) : nil

Removes a widget from the list.

    b = app:button("me me me")
    my_list:add(b)
    my_list:remove(b)

list:size
-----

    size() : int

Returns count of all widgets in the list.

    s = my_list:size()

list:clear
-----

    clear() : nil

Removes all widgets from the list.

    my_list:clear()


list:enabled
-----

    enabled() : bool

Returns true if the list is enabled

    en = my_list:enabled()

list:enable
-----

    enable() : nil

Enables the list.

    my_list:enable()

list:disable
-----

    disable() : nil

Disables the list.

    my_list:disable()

grid object
=====

A grid object provides a way to layout widgets in a flexible way. You can nest grids within
grids if you like. Use [app:grid](reference.md#appgrid) to create a grid.

grid:place
-----

    place(object:widget, row:int, column:int) : nil

Places a widget on the grid at the row and column specified. You can also nest grids as
a grid is also a widget.

    b = app:button("moo cow")
    my_grid:place(b, 0, 0)

grid:place_across
-----

    place(object:widget, row:int, column:int, row_span:int, column_span:int) : nil

Places a widget on the grid at the row and column specified. You also must specify how
many rows and columns the widget can span. This provides finer control over size and
placements of widgets.  You can also nest grids as a grid is also a widget.

    a = app:button("moo cow")
    b = app:button("chicken")
    c = app:button("cat")

    my_grid:place_across(a, 0, 0, 2, 2)
    my_grid:place_across(b, 1, 0, 1, 1)
    my_grid:place_across(b, 1, 1, 1, 1)

grid:enabled
-----

    enabled() : bool

Returns true if the grid is enabled

    en = my_grid:enabled()

grid:enable
-----

    enable() : nil

Enables the grid.

    my_grid:enable()

grid:disable
-----

    disable() : nil

Disables the grid.

    my_grid:disable()

draw object
=====

You can use a draw object to display graphics such as lines, circles, and images.
Use [app:draw](reference.md#appdraw) to create one.

draw:mouse_moved_callback
-----

    mouse_moved_callback() : string

Returns the function called when the mouse is moved on the draw surface. 
Use [when_mouse_moved](reference.md#drawwhen_mouse_moved) to set the callback.

    cb = my_draw:mouse_moved_callback()

draw:mouse_pressed_callback
-----

    mouse_pressed_callback() : string

Returns the function called when a mouse button is pressed on the draw surface. 
Use [when_mouse_pressed](reference.md#drawwhen_mouse_pressed) to set the callback.

    cb = my_draw:mouse_pressed_callback()

draw:mouse_released_callback
-----

    mouse_released_callback() : string

Returns the function called when the mouse button is released on the draw surface. 
Use [when_mouse_released](reference.md#drawwhen_mouse_released) to set the callback.

    cb = my_draw:mouse_released_callback()

draw:mouse_dragged_callback
-----

    mouse_dragged_callback() : string

Returns the function called when the mouse is dragged on the draw surface. 
Dragged means a mouse button was down when the mouse was moving.
Use [when_mouse_dragged](reference.md#drawwhen_mouse_dragged) to set the callback.

    cb = my_draw:mouse_dragged_callback()

draw:when_mouse_mmoved
-----

    when_mouse_mmoved(func:string) : nil

Sets the function to be called when the mouse is moved on the draw surface. 

The callback must be of the form:

    callback(x: int, y: int)

Where **x** and **y** are the pixel coordinates of the mouse.

    my_draw:when_mouse_mmoved("foo")

    function foo(x, y)
    end

draw:when_mouse_pressed
-----

    when_mouse_pressed(func:string) : nil

Sets the function to be called when a mouse button is pressed on the draw surface. 

The callback must be of the form:

    callback(button: int, x: int, y: int)

Where **button** is the mouse button number and **x** and **y** are the pixel coordinates of the mouse.

    my_draw:when_mouse_pressed("foo")

    function foo(b, x, y)
    end

draw:when_mouse_released
-----

    when_mouse_released(func:string) : nil

Sets the function to be called when a mouse button is released on the draw surface. 

The callback must be of the form:

    callback(button: int, x: int, y: int)

Where **button** is the mouse button number and **x** and **y** are the pixel coordinates of the mouse.

    my_draw:when_mouse_released("foo")

    function foo(b, x, y)
    end

draw:when_mouse_dragged
-----

    when_mouse_dragged(func:string) : nil

Sets the function to be called when a mouse is dragged on the draw surface. 
Dragged means a mouse button was down when the mouse was moving.

The callback must be of the form:

    callback(button: int, x: int, y: int)

Where **button** is the mouse button number and **x** and **y** are the pixel coordinates of the mouse.

    my_draw:when_mouse_dragged("foo")

    function foo(b, x, y)
    end

draw:set_name
-----

    set_name(name:string) : nil

When you give a name to a widget, it has a life that is shared across all instances.
All events in one instance are propagated automatically to the others.

For the case of a draw surface, if the draw is clicked or the mouse is moved, it will be clicked
 or moved on all instances.

    my_draw:set_name("some unique name")
    my_draw:when_mouse_moved("foo")

    function foo(x, y)

    end

So if one user moves the mouse, then the "foo" function will be called for each user
in a conversation with the **x** and **y** coordinates.

draw:clear
-----

    clear() : nil

Clears all graphics from the draw surface.

    my_draw:clear()

draw:line
-----

    line(x1:int, y1:int, x2:int, y2:int) : nil

Draws a line from **x1**, **y1** to **x2**, **y2**

    my_draw:line(0, 0, 500, 500)

draw:circle
-----

    circle(x:int, y:int, radius:int) : nil

Draws a circle at **x**, **y** with the **radius** specified.

    my_draw:circle(50, 50, 20)

draw:image
-----

    image(img:image, x:int, y:int, width:int, height:int) : nil

Draws an image at **x**, **y** with the **width** and **height** specified.

    my_draw:image(my_img, 50, 60, 120, 120)

draw:pen
-----

    pen(object:pen) : nil

Sets the pen used by the draw functions.

    my_draw:pen(my_pen)

draw:get_pen
-----

    get_pen() : pen

Returns the pen used by the draw functions.

    p = my_draw:get_pen()


draw:enabled
-----

    enabled() : bool

Returns true if the draw is enabled

    en = my_draw:enabled()

draw:enable
-----

    enable() : nil

Enables the draw.

    my_draw:enable()

draw:disable
-----

    disable() : nil

Disables the draw.

    my_draw:disable()

image object
=====

An image object can display a raster image of various formats. Use [app:image](reference.md#appimage)
to create one.

image:width
-----

    width() : int

Returns the width of the image in pixels.

    w = my_image:width()

image:height
-----

    height() : int

Returns the height of the image in pixels.

    h = my_image:height()

image:good
-----

    good() : bool

Returns true if the data used to create the was actual image data. Only good images are 
displayed.

    g = my_image:good()

image:enabled
-----

    enabled() : bool

Returns true if the image is enabled

    en = my_image:enabled()

image:enable
-----

    enable() : nil

Enables the image.

    my_image:enable()

image:disable
-----

    disable() : nil

Disables the image.

    my_image:disable()

bin_data object
=====

A bin_data object stores binary data. It is created from various functions such as
[app:open_bin_file](reference.md#appopen_bin_file).

bin_data:size
-----

    size() : int

Returns the size of the data in bytes.

    s = my_data:size()

bin_data:get
-----

    get(index:int) : char

Returns the byte at the specified index.

    s = my_data:get(5)

bin_data:set
-----

    set(index:int, byte:char) : nil

Sets the byte at the specified index.

    my_data:set(5, 44)

bin_data:sub
-----

    sub(index:int, size:int) : bin_data

Returns a subsection of data at the specified index with the specified size.

    s = my_data:sub(10, 25)

bin_data:append
-----

    append(data:bin_data) : nil

Appends data to the end.

    my_data:append(more_ata)

bin_data:str
-----

    str() : string

Converts the data to a string.

    app:print(my_data:str())

mic object
=====

A mic object provides access to the microphone available to the computer.
Create one using [app:mic](reference.md#appmic) to create a mic.

mic:when_sound
-----

    when_sound(func:string) : nil

Sets the callback to be used when sound data is ready from the mic. 

The callback must be of the following form:

    callback(sound:bin_data)

The callback will be called repeated when more sound data is available.

    my_mic:when_sound("foo")

    spkr = app:speaker("opus")

    function foo(sound)
        spkr:play(sound)
    end

mic:start
-----

    start() : nil

Start recording from the microphone.

    my_mic:start()

mic:stop
-----

    stop() : nil

Stop recording from the microphone. This will cause the sound callback to stop being called.

    my_mic:stop()

speaker object
=====

A speaker object provides access to the speaker available to the computer. You can
use this to play sounds. Use [app:speaker](reference.md#appspeaker) to create a speaker.

speaker:play
-----

    play(sound:bin_data) : nil

Plays the sound given to it. The sound data must match the codec that the speaker was created with.

    my_spkr:play(fun_sound)

speaker:mute
-----

    mute() : nil

Mutes the speaker. 

    my_spkr:mute()

audio_encoder object
=====

An audio_encoder is used to encode **pcm** audio data to **opus**. 
Use [app:audio_encoder](reference.md#appaudio_encoder) to create an audio_encoder.

audio_encoder:encode
-----

    encode(pcm:bin_data) : bin_data

Takes **pcm** audio data and encodes it to **opus**.

    some_opus = my_encoder:encode(some_pcm)

audio_decoder object
=====

An audio_decoder is used to decode **opus** audio data to **pcm**. 
Use [app:audio_decoder](reference.md#appaudio_decoder) to create an audio_decorder.

audio_decoder:decode
-----

    decode(opus:bin_data) : bin_data

Takes **opus** audio data and decodes it to **pcm**.

    some_pcm = my_decoder:decode(some_opus)

contact object
=====

A contact object provides information about a contact. Use [app:contact](reference.md#appcontact)
to access which contacts are connected to the conversation.

contact:id
-----

    id() : string

Returns the **id** of the contact. Each contact has a unique **id**.

    id = my_friend:id()

contact:name
-----

    name() : string

Returns the **name** of the contact. The **name** is not unique.

    name = my_friend:name()

contact:online
-----

    online() : bool

Returns the true if the contact is still online. If a contact will not be in the 
conversation anymore if the are offline.

    on = my_friend:online()

app object
=====

An app object provides information about an app in the conversation. 
Use [app:app](reference.md#appapp) to access which apps are used in the 
current conversation.

app:id
-----

    id() : string

Returns the **id** of the app. Each app has a unique **id**.

    id = some_app:id()

app:send
-----

    send(msg:message) : nil

Sends a local message to the app.

    some_app:send(cool_msg)

message object
=====

A message object is used to store data for a message. Create one using the 
[app:message](reference.md#appmessage) function. Send a message using the
[app:send](reference.md#appsend) function.

message:from
-----

    from() : contact

Returns the contact that the message came from.

    c = cool_msg:from()

message:not_robust
-----

    not_robust() : nil

If a message is robust, then it will be resent automatically if it did not make it for
a fixed amount of times. You can turn this off by calling this method.

    cool_msg:not_robust()

message:get_bin
-----

    get_bin(key:string) : bin_data

Returns [bin_data](reference.md#bin_data-object) with the key specified.

    dat = cool_msg:get_bin("sound")

message:set_bin
-----

    set_bin(key:string, val:bin_data) : nil

Sets [bin_data](reference.md#bin_data-object) with to key specified.

    cool_msg:set_bin("sound", dat)

message:get_vclock
-----

    get_vclock(key:string) : bin_data

Returns a [vclock](reference.md#vlock-object) with the key specified.

    clock = cool_msg:get_vclock("clock")

message:set_vclock
-----

    set_vclock(key:string, val:bin_data) : nil

Sets [vclock](reference.md#vlock-object) with to key specified.

    cool_msg:set_vclock("clock", clk)

message:get
-----

    get(key:string) : lua object

Returns a lua object with the key specified.

    stuf = cool_msg:get("stuff")

message:set
-----

    set(key:string, val:lua table) : nil

Sets a lua object with to key specified. This can be any lua type including tables.
You can use this to send lua data structures. And use [get](reference.md#messageget) to
retrieve the data.

    cool_msg:set("stuff", {name="hey", stuff="hi"})

message:is_local
-----

    is_local() : bool

Returns a true if the message is local.

    loc = cool_msg:is_local()

message:set_type
-----

    set_type(type:string) : nil

Sets the **type** of the message. The type can be used to call different functions
to respond to a message. You can use [when_message](reference.md#appwhen_message) method 
in app to call a specific function based on the type.

    cool_msg:set_type("pic")

message:type
-----

    type() : string

Returns the **type** of the message.The type can be used to call different functions
to respond to a message. You can use [when_message](reference.md#appwhen_message) method 
in app to call a specific function based on the type.

    type = cool_msg:type()

message:app
-----

    app() : app

Returns the [app](reference.md#app-object) that sent the message if it was sent locally.
You can determine if a message is local by using [is_local](reference.md#messageis_local).

    app = cool_msg:app()

file_data object
=====

A file data object stores the file data returned from [app:open_file](reference.md#appopen_file).

file_data:good
-----

    good() : bool

Returns a true the file_data object is storing any file data.

    good = my_file:good()

file_data:name
-----

    name() : string

Name of the file the user choose when using [open_file](reference.md#appopen_file).

    name = my_file:name()

file_data:size
-----

    size() : int

Returns the size of the file picked in bytes.

    sz = my_file:size()

file_data:data
-----

    data() : string

Returns the file data as a string.

    dat = my_file:data()


bin_file_data object
=====

A bin_file data object stores the binary file data returned from [app:open_bin_file](reference.md#appopen_bin_file).

bin_file_data:good
-----

    good() : bool

Returns a true the bin_file_data object is storing any file data.

    good = my_file:good()

bin_file_data:name
-----

    name() : string

Name of the file the user choose when using [open_bin_file](reference.md#appopen_bin_file).

    name = my_file:name()

bin_file_data:size
-----

    size() : int

Returns the size of the file picked in bytes.

    sz = my_file:size()

bin_file_data:data
-----

    data() : bin_data

Returns the binary file data as a [bin_file](reference.md#bin_file-object).

    dat = my_file:data()

timer object
=====

A timer object can be used to execute call at a regular interval. 
Use [app:timer](reference.md#apptimer) to a timer.

timer:running
-----

    running() : bool

Returns true if the timer is running.

    is_running = my_timer:running()

timer:start
-----

    start() : nil

Starts the timer. Whatever function is set using [when_triggered](reference.md#timerwhen_triggered) 
will be called.

    my_timer:start()

timer:stop
-----

    stop() : nil

Stops the timer. 

    my_timer:stop()

timer:interval
-----

    interval(msec:int) : nil

Sets the timer interval in milliseconds.

    my_timer:interval(1500) --second and a half

timer:when_triggered
-----

    when_triggered(code:string) : nil

Sets the code called when the timer is triggered.

    my_timer:when_triggered("foo()")
    
    function foo()
    end

pen object
=====

A pen is used by the [draw](reference.md#draw-object) object to determine color and width
of the lines and circles that are drawn. Use [app:pen](reference.md#apppen) to create a pen.

pen:set_width
-----

    set_width(pixels:int) : nil

Sets the width of the pen.

    my_pen:set_width(4)

vclock object
=====

A vclock object is a vector clock. A vector clock can be used to determine if messages
are sent concurrently. You can send clocks with messages and use them to partially order
messages and determine when concurrent messages are sent.

Vector clocks are useful if you want need to have some understanding of order of events.

vclock:good
-----

    good() : bool

Returns true if it is a valid vector clock.

    yay = my_clock:good()

vclock:inc
-----

    inc() : bool

Increments the element for the current user by one in the vector clock.

    my_clock:inc()

vclock:merge
-----

    merge(other:vclock) : nil

Merges two vector clocks together. The clock is modified in place.

    my_clock:merge(other_clock)

vclock:conflict
-----

    conflict(other:vclock) : bool

Returns true if the two vector clocks are in conflict. This means the clocks have concurrent
changes and are not equal.

    c = my_clock:conflict(other_clock)

vclock:concurrent
-----

    concurrent(other:vclock) : bool

Returns true if the two vector clocks are concurrent. This means that neither clock is newer.
The clocks may be identical.

    c = my_clock:concurrent(other_clock)

vclock:comp
-----

    comp(other:vclock) : int

Compares two vector clocks. 

  * Returns -1 if the other clock is greater.
  * Returns 1 if the other clock is less.
  * Returns 0 if the clocks are not less or greater. 

This can be used to create a partial ordering of events.

    c = my_clock:comp(other_clock)

vclock:equals
-----

    equals(other:vclock) : bool

Returns true if the two vector clocks are identical. 

    identical = my_clock:equals(other_clock)


store object
=====

A store object is a global object that can be used to store permanent data. This data
will be available to all apps and survives restarts. There is only one **store** object.

store:get_bin
-----

    get_bin(key:string) : bin_data

Returns [bin_data](reference.md#bin_data-object) with the key specified.

    dat = store:get_bin("sound")

store:set_bin
-----

    set_bin(key:string, val:bin_data) : nil

Sets [bin_data](reference.md#bin_data-object) with to key specified.

    store:set_bin("sound", dat)

store:get_vclock
-----

    get_vclock(key:string) : bin_data

Returns a [vclock](reference.md#vlock-object) with the key specified.

    clock = store:get_vclock("clock")

store:set_vclock
-----

    set_vclock(key:string, val:bin_data) : nil

Sets [vclock](reference.md#vlock-object) with to key specified.

    store:set_vclock("clock", clk)

store:get
-----

    get(key:string) : lua object

Returns a lua object with the key specified.

    stuf = store:get("stuff")

store:set
-----

    set(key:string, val:lua table) : nil

Sets a lua object with to key specified. This can be any lua type including tables.
You can use this to save lua data structures. And use [get](reference.md#storeget) to
retrieve the data.

    store:set("stuff", {name="hey", stuff="hi"})

store:has
-----

    has(key:string) : bool

Returns true if the key exists.

    has_stuff = store:has("stuff")

store:remove
-----

    remove(key:string) : bool

Removes the value with the key from the store. Returns true if the value was removed.

    removed = store:remove("stuff")

data object
=====

A data object is a global object that can be used to store app data permanently. This data
is stored with the app and is sent to all contacts along with the app. Use this to store
any data the app needs like graphics and sounds. Do not store a lot of data because it
is sent to all contacts in a conversation each time the app starts.

You can also populate the data using the **data** tab in the App Editor.

data:get_bin
-----

    get_bin(key:string) : bin_data

Returns [bin_data](reference.md#bin_data-object) with the key specified.

    dat = data:get_bin("sound")

data:set_bin
-----

    set_bin(key:string, val:bin_data) : nil

Sets [bin_data](reference.md#bin_data-object) with to key specified.

    data:set_bin("sound", dat)

data:get_vclock
-----

    get_vclock(key:string) : bin_data

Returns a [vclock](reference.md#vlock-object) with the key specified.

    clock = data:get_vclock("clock")

data:set_vclock
-----

    set_vclock(key:string, val:bin_data) : nil

Sets [vclock](reference.md#vlock-object) with to key specified.

    data:set_vclock("clock", clk)

data:get
-----

    get(key:string) : lua object

Returns a lua object with the key specified.

    stuf = data:get("stuff")

data:set
-----

    set(key:string, val:lua table) : nil

Sets a lua object with to key specified. This can be any lua type including tables.
You can use this to save lua data structures. And use [get](reference.md#dataget) to
retrieve the data.

    data:set("stuff", {name="hey", stuff="hi"})

data:has
-----

    has(key:string) : bool

Returns true if the key exists.

    has_stuff = data:has("stuff")

data:remove
-----

    remove(key:string) : bool

Removes the value with the key from the data. Returns true if the value was removed.

    removed = data:remove("stuff")


