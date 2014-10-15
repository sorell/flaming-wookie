#!/usr/bin/env python

import time
import socket
import argparse
import struct
import binascii


REC_ACT_STORE = 0x0001
REC_ACT_GET_AFTER = 0x0002


def makePacket(action, serial, devType, logData):
	PRM_ACTION = 0x0001
	PRM_SERNUM = 0x0002
	PRM_DEVTYPE = 0x0003
	PRM_DATA = 0x0004

	# binstr = struct.pack(logData)
	values = (0x5A5A, PRM_ACTION, 2, action, PRM_SERNUM, len(serial), serial, PRM_DEVTYPE, len(devType), devType, PRM_DATA, len(logData), logData)
	#print 'len', len(binstr), 'val', binstr
	s = struct.Struct('! H H H H H H ' + str(len(serial)) + 's H H ' + str(len(devType)) + 's H H ' + str(len(logData)) + 's')
	data = s.pack(*values)
	return data




exit 


IPADDR = '127.0.0.1'
PORT = 12345
BUFSIZE = 1500


sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((IPADDR, PORT))

i = 100
while i < 110:
	packet = makePacket(REC_ACT_STORE, 'EL001122', 'AC1000', str(i))
	sock.send(packet)
	i = i + 1
	print packet

time.sleep(2)
#data = sock.recv(BUFSIZE)


