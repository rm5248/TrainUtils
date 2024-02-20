#!/usr/bin/python3

# 	packet: 9F 23 1D .
import re
import sys

signal_regex = re.compile('\s+packet: ([0-9A-F][0-9A-F]) ([0-9A-F][0-9A-F]) ([0-9A-F][0-9A-F]).*')


for line in sys.stdin:
	match = signal_regex.match(line)
	if match:
		#print("yes match")
		#print(match.group(1))
		#print(match.group(2))
		#print(match.group(3))
		byte1 = int(match.group(1), 16)
		byte2 = int(match.group(2), 16)
		byte3 = int(match.group(3), 16)

		#address = (byte1 & 0x3f) << 5
		#address |= (byte2 & 0x7a) >> 2
		#address |= (byte2 & 0x6) >> 1

		# from nmradcc library
		BoardAddress = ( ( (~byte2) & 0b01110000) << 2) | (byte1 & 0b00111111) ;
		TurnoutPairIndex = (byte2 & 0b00000110) >> 1;

		OutputAddress = ( ( (BoardAddress - 1) << 2) | TurnoutPairIndex) + 1 ;

		address = OutputAddress
		aspect = byte3 & 0x1f

		signal = "unknown"

		OutputAddress += 4
		aspect_string = "UNKN"

		if OutputAddress == 1406:
			signal = "7E"
		elif OutputAddress == 1403:
			signal = "8W"
		elif OutputAddress == 1402:
			signal = "7W"
		elif OutputAddress == 1405:
			signal = "6E"

		if aspect == 0:
			aspect_string = "stop"
		elif aspect == 21:
			aspect_string = "Caution"
		elif aspect == 29:
			aspect_string = "Clear"

		print(f"{byte1:02x} {byte2:02x} {byte3:02x} address: {address} ({signal}) aspect: {aspect} {aspect_string}", flush=True)
