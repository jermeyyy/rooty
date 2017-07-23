#!/usr/bin/env python

import socket
import sys
import os
from struct import *
import time

AUTH = 0xDEC0ADDE

#src_ip = sys.argv[2]
dst_ip = sys.argv[1]

def checksum(str):

        csum = 0
        countTo = (len(str) / 2) * 2
 
        count = 0
        while count < countTo:
                thisVal = ord(str[count+1]) * 256 + ord(str[count])
                csum = csum + thisVal
                csum = csum & 0xffffffffL #
                count = count + 2
 
        if countTo < len(str):
                csum = csum + ord(str[len(str) - 1])
                csum = csum & 0xffffffffL #
 
        csum = (csum >> 16) + (csum & 0xffff)
        csum = csum + (csum >> 16)
        answer = ~csum
        answer = answer & 0xffff
 
        answer = answer >> 8 | (answer << 8 & 0xff00)
 
        return answer


try:
    sd = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.getprotobyname("icmp"))
except socket.error, msg:
    print 'Socket could not be created. Err code: ' + str(msg[0]) + ' Message ' + msg[1]
    exit()

packet = '';

dst_ip  =  socket.gethostbyname(dst_ip)
ID = os.getpid() & 0xFFFF
my_checksum = 0

header = pack("bbHHh", 8, 0, my_checksum, ID, 1)
data = pack("!I", AUTH);
my_checksum = checksum(header + data)
header = pack("bbHHh", 8, 0, socket.htons(my_checksum), ID, 1)
packet = header + data

sd.sendto(packet, (dst_ip, 0))