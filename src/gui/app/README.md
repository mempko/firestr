gui/app library
===================================================================

This library implements the app model and service. Including several
internal applications and the all important script_app application.

file summary
===================================================================

app            
-------------------------------------------------------------------

Model for what an application is.

app_editor     
-------------------------------------------------------------------

gui for creating an application and adding it to your
collection.
             
app_service    
-------------------------------------------------------------------

Service which deals with managing your application library
and load/saving/cloning applications.

chat_sample    
-------------------------------------------------------------------

Implements a chat program in C++. This is one of the 
internal applications.

lua_script_api 
-------------------------------------------------------------------

Implements the API for creating applications in Lua.
Has the LUA bindings to interact with the rest of
firestr
              
script_app     
-------------------------------------------------------------------

Application which uses lua scripts (in the future different languages).
