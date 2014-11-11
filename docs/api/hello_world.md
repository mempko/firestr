Hello World
================

This tutorial will show you how to program your first Fire★ App. We will create a button
which, when pressed changes it's text to "Hello World"

Starting the App Editor
-----------------------

Fire★ has a built in App Editor which allows you to write an App by yourself or together with 
any number of other people. 

The App Editor compiles the code as you write it and provides immediate feedback on errors.

If you are writing an App that sends messages, it is best to either program it with a friend, 
or run a test instance of Fire★. The App we are writing in this tutorial won't require 
another instance. How to run a test instance will be explained in another tutorial.

To start the App Editor:

  1. Create a new conversation via the **Conversation** menu.
  2. Select **App Editor** in the **App** menu.
  3. Select **"<new app\>"** in the App selection dialog and press **OK**.

How the App Editor Works
-----------------------

The App Editor is composed of two main parts. There is a **code** tab and a **data** tab.
The **code** tab is where you write you App code and the **data** tab is where you can import
files, images, or other data into your App. 

The **code** tab is composed of four parts

  1. A **canvas** where you can place widgets.
  2. A **print area** where messages and print statements are displayed.
  3. The **code editor** where you write your code.
  4. The **save** and **export** buttons.

Writing the Code
-------------------------

All Fire★ apps are written in [Lua](http://lua.org) and all API functions are under the **app** object.
To create a button you use the **"button"** function which accepts the text you want the button to display.

Type the following in the code editor.

    hello = app:button("Hello")

Apps are compiled and executed automatically when you stop typing. You won't see a button
yet because you have to place the button on the **canvas**.

    app:place(hello, 0, 0)

You can imagine the **canvas** as an invisible grid where you can place widgets. The first
parameter is the widget you want to place, the second is the row, and third is the column. The row
and column start from zero. Now you should see your hello button on the **canvas**.

If you click on it, nothing should happen. You have to register a function that will be
called when the button is clicked.

    hello:when_clicked("world()")

The **"when_clicked"** function will execute the code inside the quotes. In this case it will
call a function **"world"** with no parameters.

If you click the button now you should see an error in the **print area**.

    error: Lua Error: 
    -------------------------------------------------------
    [string "--app..."]:2: attempt to call global 'world' (a nil value)
    Traceback: [ 0 (C) ] @ world(global)
    [ 1 (main) ] [string "--app..."]:2

To fix this we have to create a function called **"world"**

    function world()

    end

If you click now, you should not get any error. Of course nothing will happen, so we have
to make this function do something. What we will do is change the text of our button to
say **"Hello World"**

    function world()
        hello:set_text("Hello World")
    end

Clicking on the button now should change the button text to **"Hello World"**. If you click it
a second time, nothing new should happen. Congratulations, you wrote your first Fire★ App!

To save the app to your collection press the **save** button at the bottom and give it a
name. You can now use this App in a conversation with others. Other people won't need to 
install this App because it gets deployed automatically when it is used.

The full App code is listed below.

    hello = app:button("Hello")
    app:place(hello, 0, 0)

    hello:when_clicked("world()")

    function world()
        hello:set_text("Hello World")
    end








