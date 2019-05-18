#!/usr/bin/python
from __future__ import print_function
import serial, struct, itertools

def makechecksum(data, offset, length):
	crc = 0x7F
	for b in itertools.islice(data, offset, offset+length):
		crc = (crc - b) & 0x7F
	return crc

def vbusdecode(data):
#
# byte     function
#
# 0        sync byte 0xAA
# 1,2      destination address
# 3,4      source address
# 5        protocol version
# 6,7      command
# 8        frame count
# 9        checksum

	# TODO check length of buffer
	(destination, source, pversion, command, framecount, checksum) = struct.unpack_from("<HHBHBB", data, offset=1)

	print("----")
	print("dest:    0x%04x" % destination)
	print("source:  0x%04x" % source)
	print("version: 0x%02x" % pversion)
	print("command: 0x%02x" % command)
	print("frames:  %i" % framecount)
	print("checksum: 0x%02x" % checksum)
	print("          0x%02x" % makechecksum(data, 1, 8))
	print("packet bytes:   %i" % len(data))

	if command != 0x100:
		#print(["%02x" % i for i in data])
		print("wrong command 0x%04x" % command)
		return

	payloadoffset = 10
	framesize = 6
	bytesinframe = 4
	septetindex = 4
	checksumindex = 5

	offset = payloadoffset
	for f in range(0, framecount):
		cs = data[offset + checksumindex]
		ccs = makechecksum(data, offset, bytesinframe+1)
		if cs != ccs:
			print("wrong checksum in frame %i:" % f)
			print("cs: 0x%02x computed cs: 0x%02x" % (cs, ccs))

		septet = data[offset + septetindex]
		for b in range(0, bytesinframe):
			if septet & (1 << b) != 0:
				data[offset + b] |= 0x80
		offset += framesize

	unpacked = struct.unpack_from("<hhxxhhxxBBBBxxHHxxHHxxHH", data, payloadoffset)

	names = ("tempS1", "tempS2", "tempS3", "tempS4", "pumpR1", "pumpR2", "errMask", "variant", "opHoursR1", "opHoursR2", "heat1", "heat1k", "heat1M", "sysTime")
	values = {n:v for (n, v) in zip(names, unpacked)} 

	values["tempS1"] *= 0.1
	values["tempS2"] *= 0.1
	values["tempS3"] *= 0.1
	values["tempS4"] *= 0.1

	values["heat"] = values['heat1'] + values['heat1k'] * 1000 + values['heat1M'] * 1000000

	print(values)

# DeltaSol C 0x4212

#  Offset
#  |  Size
#  |  |  Mask
#  |  |  |  Name
#  |  |  |  |                  Factor Unit  unpack code
#  0  2     Temperature S1     0.1    C     h
#  2  2     Temperature S2     0.1    C     h
#  4  2     Temperature S3     0.1    C     h
#  6  2     Temperature S4     0.1    C     h
#  8  1     Pump speed R1        1    %     B
#  9  1     Pump speed R2        1    %     B
# 10  1     Error mask           1          B
# 11  1     Variant              1          B
# 12  2     Operating hours R1   1    h     H
# 14  2     Operating hours R2   1    h     H
# 16  2     Heat quantity        1    Wh    H
# 18  2     Heat quantity     1000    Wh    H
# 20  2     Heat quantity  1000000    Wh    H
# 22  2     System time          1          H

# if ((Command==0x0100) and (Bufferlength==10+Framecnt*6)) {


with serial.Serial('/dev/ttyUSB0', 9600, timeout=1) as ser:
	data = None
	while True:
		x = ser.read()          # read one byte

		if len(x) <= 0:
			continue

		x = int(x.encode('hex'), 16)

		if x == 0xAA:
			# ignore the first incomplete frame
			if data is not None:
				vbusdecode(data)
			data = bytearray()

		if data is not None:
			data.append(x)

