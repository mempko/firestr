gui qtw
===================================================================

QWidget frontend implementation for apps.
The backend calls the frontend via the frontend interface and the
frontend calls the backend via the backend interface.

file summary
===================================================================

frontend  
-------------------------------------------------------------------
QWidget implementation of the frontend widgets. Implements the frontend 
interface.

frontend_client
-------------------------------------------------------------------
Frontend implementation which uses messages to call the actual frontend.
The backend will typically call into this which provides non blocking
manipulation of the frontend.

audio  
-------------------------------------------------------------------
Code to get audio from mic and play audio through speakers using Qt
