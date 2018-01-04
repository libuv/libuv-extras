# Eventpool
Basic C implementation of observer pattern. Custom functions can be attached to a certain predefined event. 
These events can subsequently be triggered anywhere in the program. The can greatly increase the modularity 
of you program, because module can now attach itself to a certain event, without having to hard-code 
interactions. Only as all listeners are called, the eventpool will trigger a garbage collection function that 
allows you to clean memory. Listeners therefor don't have to care about garbage collection and so can just 
consume information passed.

## API
Ideally, a custom numeric list of events needs to be predifened so it is easier to trigger a named event. The 
same named events can then be added to the `static struct reasons_t` inside the `eventpool.c` source file. 
The eventpool first needs to be initialized using the `eventpool_init(int mode)` function using either 
`EVENTPOOL_THREADED` of `EVENTPOOL_NO_THREAD`. The first will use the libuv threadpool to trigger the 
listener callbacks. The latter trigger run all listener callbacks from the same thread as the libuv loop is 
running. Before terminating the program, the `eventpool_gc(void)` needs to be called to free all allocated 
memory again. Listener register callbacks using the `eventpool_callback(int reason, void *(*)(int reason, 
void *data))` function to a certain event. The program can now trigger an event using the 
`eventpool_trigger(int reason, void *(*done)(void *), void *data)` function. The `done` function will be 
called as soon as all listener callback where called.

## Improvements
- Allow (named) events to be added dynamically.
- Using a priority list for event queue so listeners high priority events are triggered before priority events. But, this is not supported in libuv (yet).

## Building
Just run `Make` inside this folder and run the eventpool `program` afterwards. This will run an example program in with various event callbacks are triggered with various listeners.
Make sure you have the libuv shared library installed so it can be linked with `-luv`.
