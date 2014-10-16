#!/usr/bin/env python

import socket
import struct
import binascii
import time
import datetime
import argparse
import sys


DEFAULT_SERVER = 'localhost'
DEFAULT_PORT = 12345

REC_ACT_STORE = 0x0001
REC_ACT_GET_AFTER = 0x0002
REC_ACT_OBSERVE = 0x0003

PRM_ACTION = 0x0001
PRM_SERNUM = 0x0002
PRM_DEVTYPE = 0x0003
PRM_DATA = 0x0004
PRM_TIME = 0x0005



def openConnection(server, port):
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	try:
		sock.connect((server, port))
		return sock
	except socket.error as msg:
		print 'Can\'t open connection to ' + server + ':' + str(port) + ': ' + str(msg)
		exit(1)


def parseStoreStr(s):
	storestr = s.split(',')
	if len(storestr) != 3:
		print 'STORE must be of format SERIAL,DEVTYPE,DATA'
		exit(1)
	return storestr


def parseQueryStr(s):
	querystr = s.split(',')
	if len(querystr) != 3:
		print 'QUERY must be of format SERIAL,DEVTYPE,AGE'
		print 'SERIAL and DEVTYPE may be \'*\' for wildcard'
		print 'AGE may be 0 to denote \'since the dawn of time\'. Otherwise considered as record age in seconds'
		exit(1)
	try:
		querystr[2] = int(querystr[2])
		if (querystr[2] != 0):
			querystr[2] = time.time() - querystr[2]
	except:
		print 'AGE must be an integer'
		exit(1)
	return querystr


def parseObserveStr(s):
	observeStr = s.split(',')
	if len(observeStr) != 2:
		print 'OBSERVE must be of format SERIAL,DEVTYPE'
		print 'SERIAL and DEVTYPE may be \'*\' for wildcard'
		exit(1)
	return observeStr


def parseTlvHeader(s, packet):
	if len(s) < 4:
		raise Exception()

	header = struct.unpack('! HH', s[:4])

	s = s[4:]
	if len(s) < header[1]:
		raise Exception()

	if header[0] == PRM_ACTION:
		if header[1] != 2:
			print 'header action len mismatch'; exit(1)
		packet['action'] = struct.unpack('! H', s[0:2])[0]
		return s[2:]

	if header[0] == PRM_SERNUM:
		strlen = int(header[1])
		packet['serial'] = s[0:strlen]
		return s[strlen:]

	if header[0] == PRM_DEVTYPE:
		strlen = int(header[1])
		packet['devtype'] = s[0:strlen]
		return s[strlen:]

	if header[0] == PRM_DATA:
		strlen = int(header[1])
		packet['data'] = binascii.b2a_hex(s[0:strlen])
		return s[strlen:]

	if header[0] == PRM_TIME:
		if header[1] != 8:
			print 'TLV time len mismatch'; exit(1)
		val = struct.unpack('! II', s[0:8])
		packet['time'] = float(val[0]) + float(val[1]) / 1000000
		return s[8:]

	print 'Unrecognized type', header[0]
	exit(1)


def parseQueryPackets(sock):
	s = ''
	while (True):
		try:
			s += sock.recv(1500)
		except socket.error as msg:
			print 'Received error:', str(msg)
			exit(1)

		if len(s) == 0:
			print 'Disconnected'
			exit(0)

		while len(s) > 10:
			if struct.unpack('! H', s[0:2])[0] != 0x5A5A:
				print 'Start byte mismatch'; exit(1)
			
			try:
				packet = {}
				tmp = s[2:]
				tmp = parseTlvHeader(tmp, packet)
				tmp = parseTlvHeader(tmp, packet)
				tmp = parseTlvHeader(tmp, packet)
				tmp = parseTlvHeader(tmp, packet)
				tmp = parseTlvHeader(tmp, packet)
				s = tmp
				print 'packet', packet
			except:
				break







def stressTest(sock):
	exit(0)



def makeStorePacket(serial, devType, logData):
	logData = binascii.a2b_hex(logData)
	values = (0x5A5A, PRM_ACTION, 2, REC_ACT_STORE, PRM_SERNUM, len(serial), serial, PRM_DEVTYPE, len(devType), devType, PRM_DATA, len(logData))
	s = struct.Struct('! H HHH HH%is HH%is HH' % (len(serial), len(devType)))
	data = s.pack(*values) + logData
	return data


def makeQueryPacket(serial, devType, since):
	values = (0x5A5A, PRM_ACTION, 2, REC_ACT_GET_AFTER, PRM_SERNUM, len(serial), serial, PRM_DEVTYPE, len(devType), devType, PRM_TIME, 8, int(since), int(since % 1 * 1000000))
	s = struct.Struct('! H HHH HH%is HH%is HHII' % (len(serial), len(devType)))
	data = s.pack(*values)
	return data


def makeObservePacket(serial, devType):
	values = (0x5A5A, PRM_ACTION, 2, REC_ACT_OBSERVE, PRM_SERNUM, len(serial), serial, PRM_DEVTYPE, len(devType), devType)
	s = struct.Struct('! H HHH HH%is HH%is' % (len(serial), len(devType)))
	data = s.pack(*values)
	return data




if __name__ == "__main__":
	argParser = argparse.ArgumentParser()
	argParser.add_argument('-S', '--server', type=str, help='Data logger IP address (default localhost)')
	argParser.add_argument('-p', '--port', type=int, help='TCP port of the data logger server (default 12345)')
	argParser.add_argument('-s', '--store', type=str, help='Store data line')
	argParser.add_argument('-q', '--query', type=str, help='Query data line')
	argParser.add_argument('-O', '--observe', type=str, help='Observe for matching records')
	argParser.add_argument('-T', '--stress', action="store_true", help='Stress test mode')
	args = argParser.parse_args()

	if int(bool(args.store)) + int(bool(args.query)) + int(bool(args.observe)) + int(bool(args.stress)) != 1:
		argParser.print_help()
		print 'You must define one and only one action (-s, -q, -O or -T)'
		exit(1)


	server = args.server if args.server != None else DEFAULT_SERVER
	port = args.port if args.port != None else DEFAULT_PORT

	if args.store:
		storeStr = parseStoreStr(args.store)

	if args.query:
		queryStr = parseQueryStr(args.query)

	if args.observe:
		observeStr = parseObserveStr(args.observe)


	sock = openConnection(server, port)

	if args.stress:
		stressTest(sock)
	
	elif args.store:
		packet = makeStorePacket(storeStr[0], storeStr[1], storeStr[2])
		sock.send(packet)
		
	elif args.query:
		packet = makeQueryPacket(queryStr[0], queryStr[1], queryStr[2])
		sock.send(packet)
		parseQueryPackets(sock)

	elif args.observe:
		packet = makeObservePacket(observeStr[0], observeStr[1])
		sock.send(packet)
		parseQueryPackets(sock)



	sock.close()

	exit(0)
