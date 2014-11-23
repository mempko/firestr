API Reference
=====

Fire★ Apps are built using the [Lua](http://lua.org) programming language. Fire★ provides a
custom object oriented API for building Apps. The main object you interact with is the [app](reference.md#app) object

app object
=====

app:print
-----

    print(message:string) : nil

Prints a string to the app editor log and Fire★ log. Newline is added automatically.

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

text_edit
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

label:text
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

So if one user clicked on the button, then the "foo" function will be called on each user
in a conversation.

button:enabled
-----

    enabled() : bool

Returns true if the button is enabled

    en = my_button:enabled()

button:enable()
-----

    enable() : nil

Enables the button.

    my_button:enable()

button:disable()
-----

    disable() : nil

Disables the button.

    my_button:disable()

label object
=====

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

label:enable()
-----

    enable() : nil

Enables the label.

    my_label:enable()

label:disable()
-----

    disable() : nil

Disables the label.

    my_label:disable()

edit object
=====

text_edit object
=====

list object
=====

grid object
=====

draw object
=====

timer object
=====

image object
=====

bin_data object
=====

mic object
=====

speaker object
=====

audio_encoder object
=====

audio_decoder object
=====

contact object
=====

app object
=====

message object
=====

file_data object
=====

bin_file_data object
=====
