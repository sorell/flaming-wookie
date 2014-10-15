#!/usr/bin/env python

import time
import socket
import argparse
import struct
import binascii


REC_ACT_STORE = 0x0001
REC_ACT_GET_AFTER = 0x0002

PRM_ACTION = 0x0001
PRM_SERNUM = 0x0002
PRM_DEVTYPE = 0x0003
PRM_DATA = 0x0004
PRM_TIME = 0x0005


def makeStorePacket(serial, devType, logData):
	values = (0x5A5A, PRM_ACTION, 2, REC_ACT_STORE, PRM_SERNUM, len(serial), serial, PRM_DEVTYPE, len(devType), devType, PRM_DATA, len(logData), logData)
	s = struct.Struct('! H H H H H H ' + str(len(serial)) + 's H H ' + str(len(devType)) + 's H H ' + str(len(logData)) + 's')
	data = s.pack(*values)
	return data


def makeQueryPacket(serial, devType, age):
	if age != 0:
		print "eioo"

	values = (0x5A5A, PRM_ACTION, 2, REC_ACT_GET_AFTER, PRM_SERNUM, len(serial), serial, PRM_DEVTYPE, len(devType), devType, PRM_TIME, 8, 1, 2)
	s = struct.Struct('! H H H H H H ' + str(len(serial)) + 's H H ' + str(len(devType)) + 's H H I I')
	data = s.pack(*values)
	return data




IPADDR = '127.0.0.1'
PORT = 12345
BUFSIZE = 1500


sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((IPADDR, PORT))

# i = 100
# while i < 110:
# 	packet = makeStorePacket('EL001122', 'AC1000', str(i))
# 	sock.send(packet)
# 	i = i + 1

packet = makeQueryPacket('*', '*', 0)
sock.send(packet)
data = sock.recv(1000)

print data


