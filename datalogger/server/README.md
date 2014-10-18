datalogd
========


Features
--------
- Data logging daemon.
- TCP server for clients.
- Database type selection on daemon start.
- Possibility for a client to receive new Records matching query parameters.


Howto
-----
devlogd -o SINK  
Select Sink to use for database. -h option shows compiled Sinks.

devlogd -p PORT  
Select TCP port to listen to.


Code related highlights
-----------------------
- There is no memory deallocation involved, if you want to forget that stl classes heavily use dynamic memory (de)allocation.

- Compile time code generation based on templates (protocol.hpp).

- Use of new c++0x stuff: Lambda functions (main.hpp) and move constructor (tcpSource.hpp).

- Self-initializing singleton objects (bintxt.cpp).


Future considerations
---------------------
- Multithreading support for scalability. Clients could be served in their own thread. Locking should be done in Sink's end: Database write should be executed in protected code section. (std::mutex)

- Support for Sources other than TCP. Implementing several Sources in addition to TcpSource would be a trivial task, much like how Sink selection is implemented. For example, ZeroMQ would be very efficient alternative for TCP/IP localhost connection.

- Bintxt database integrity testing and time or size based file rotation. 

- Rigorous testing. Server must be tested againtst invalid messages, file corruption, socket tearing, powerouts and sudden aborts.

