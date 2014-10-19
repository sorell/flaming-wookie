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
	recStr = s.split(',')
	if len(recStr) != 3:
		print 'STORE must be of format SERIAL,DEVTYPE,DATA'
		exit(1)
	for x in recStr[:2]:
		if x == '*':
			print 'SERIAL and DEVTYPE can\'t be wildcards'
			exit(1)
	return recStr


def parseQueryStr(s):
	recStr = s.split(',')
	if len(recStr) != 3:
		print 'QUERY must be of format SERIAL,DEVTYPE,AGE'
		print 'SERIAL and DEVTYPE may be \'*\' for wildcard'
		print 'AGE may be 0 to denote \'since the dawn of time\'. Otherwise considered as record age in seconds'
		exit(1)
	try:
		recStr[2] = int(recStr[2])
		if (recStr[2] != 0):
			recStr[2] = time.time() - recStr[2]
	except:
		print 'AGE must be an integer'
		exit(1)
	return recStr


def parseObserveStr(s):
	recStr = s.split(',')
	if len(recStr) != 2:
		print 'OBSERVE must be of format SERIAL,DEVTYPE'
		print 'SERIAL and DEVTYPE may be \'*\' for wildcard'
		exit(1)
	return recStr


def parseStressStr(s):
	recStr = s.split(',')
	if len(recStr) != 2:
		print 'STRESS must be of format SERIAL,DEVTYPE'
		exit(1)
	for x in recStr:
		if x == '*':
			print 'SERIAL and DEVTYPE can\'t be wildcards'
			exit(1)
	return recStr


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
	finished = False

	while (not finished):
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
			
			packet = {}
			tmp = s[2:]

			try:
				tmp = parseTlvHeader(tmp, packet)
				tmp = parseTlvHeader(tmp, packet)
				tmp = parseTlvHeader(tmp, packet)
				tmp = parseTlvHeader(tmp, packet)
				tmp = parseTlvHeader(tmp, packet)
				s = tmp
			except:
				# Not enough data: Wait for more
				break

			if len(packet['serial']) == 0  and  packet['time'] == 0.0:
				# Empty reply: last packet
				print 'Last record'
				finished = True
				break;

			print 'Query result:', packet


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


def stressTest(sock, serial, devType):
	s = struct.Struct('! H HHH HH%is HH%is HHI' % (len(serial), len(devType)))
	nextReport = time.time() + 1
	num = 0
	prevNum = 0
	avg = 0

	while True:
		values = (0x5A5A, PRM_ACTION, 2, REC_ACT_STORE, PRM_SERNUM, len(serial), serial, PRM_DEVTYPE, len(devType), devType, PRM_DATA, 4, num)
		data = s.pack(*values)
		sock.send(data)
		num = num + 1
		
		if 0 == num % 1000:
			now = time.time()
			if now > nextReport:
				if 0 == avg:
					avg = num - prevNum 
				else: 
					avg = avg * 5 / 6 + (num - prevNum) / 6

				print num - prevNum, 'packets / s, moving avgerage', avg
				prevNum = num
				nextReport = now + 1


if __name__ == "__main__":
	argParser = argparse.ArgumentParser()
	argParser.add_argument('-S', '--server', type=str, help='Data logger IP address (default localhost)')
	argParser.add_argument('-p', '--port', type=int, help='TCP port of the data logger server (default 12345)')
	argParser.add_argument('-s', '--store', type=str, help='Store data line')
	argParser.add_argument('-q', '--query', type=str, help='Query data line')
	argParser.add_argument('-O', '--observe', type=str, help='Observe for matching records')
	argParser.add_argument('-T', '--stress', type=str, help='Stress test mode')
	args = argParser.parse_args()

	if int(bool(args.store)) + int(bool(args.query)) + int(bool(args.observe)) + int(bool(args.stress)) != 1:
		argParser.print_help()
		print 'You must define one and only one action (-s, -q, -O or -T)'
		exit(1)


	server = args.server if args.server != None else DEFAULT_SERVER
	port = args.port if args.port != None else DEFAULT_PORT

	if args.store:
		recStr = parseStoreStr(args.store)

	if args.query:
		recStr = parseQueryStr(args.query)

	if args.observe:
		recStr = parseObserveStr(args.observe)

	if args.stress:
		recStr = parseStressStr(args.stress)


	sock = openConnection(server, port)

	if args.stress:
		stressTest(sock, recStr[0], recStr[1])
		# This never returns
	
	elif args.store:
		packet = makeStorePacket(recStr[0], recStr[1], recStr[2])
		sock.send(packet)
		
	elif args.query:
		packet = makeQueryPacket(recStr[0], recStr[1], recStr[2])
		sock.send(packet)
		parseQueryPackets(sock)

	elif args.observe:
		packet = makeObservePacket(recStr[0], recStr[1])
		sock.send(packet)
		parseQueryPackets(sock)

	sock.close()
	exit(0)
