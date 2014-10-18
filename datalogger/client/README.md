datalog client
==============


Features
--------
- TCP connection to datalogd server.
- Store new Record, query or observe for matching Records.
- Stress test mode.


Howto
-----
client.py -s SERVER
Select datalogd server IP address. Default is localhost.

client.py -p PORT
Select TCP port to send to. Default is 12345.

client.py -s SERIAL,DEVTYPE,DATA
Form a Record to store from user input. 
SERIAL is mandatory, DEVTYPE is optional. DATA must be hex string in ascii format.

client.py -q SERIAL,DEVTYPE,AGE
Query server for all the Records matching SERIAL and DEVTYPE, that are newer than AGE seconds.
SERIAL and DEVTYPE may be '*' for wildcard.
AGE may be 0 for 'since Big Bang'

client.py -O SERIAL,DEVTYPE
Receive new Records from the server matching SERIAL and DEVTYPE.
SERIAL and DEVTYPE may be '*' for wildcard.

client.py -T
Stress test mode. Flood the server with new Records.
