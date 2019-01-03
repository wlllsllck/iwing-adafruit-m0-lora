import sys
import serial
import time
import os

import queue
import threading
import logging
import multiprocessing
import base64

#for binary manipulation
import struct 

from datetime import datetime


logging.basicConfig(level=logging.DEBUG,
                    format='[%(levelname)s] (%(threadName)-10s) %(message)s',
                    )

# threading concept of python -- https://pymotw.com/2/threading/
# pack/unpack http://tuttlem.github.io/2016/04/06/packing-data-with-python.html

def SerialRead(ser, i, channel, channel_message, count):
    print("Serial Read")
    SFD = b'\x7e'  #0x7e
    while (True): 
        #Read until found SFD
        while (True):
            if (ser.inWaiting()>0): #if incoming bytes are waiting to be read from the serial input buffer
                sfd = ser.read(size=1)
                if sfd == SFD:
                    # print(sfd)
                    break
        # tmp = ser.read(size=1)       #D for checking in main-rx.cpp
        msgType = ser.read(size=1)
        len = ser.read(size=1)
        len = int.from_bytes(len, byteorder='big', signed=False)
#        l2 = ser.read(size=1)
#        while (ser.inWaiting() < len):
#            pass
        data = ser.read(size=len)
        block_id = struct.unpack("H", data[0:2])[0]
        rec_message = ""
        i = 2
        while i < len:
            rec_message = rec_message + chr(int(data[i]))
            i = i + 1

        print(f"Receive {msgType} {len} {block_id} {rec_message}")

        # saving result file process
        if(rec_message == "start"):     # data = start --> count["start"]++
            if(count["start"]<channel):
                count["start"] = count["start"] + 1    
            global rec_data
            rec_data = b""
            
        elif(rec_message == "end"):     # data = end --> count["end"]++
            print(count)
            channel_message = rec_data
            # print(channel_message)    
            if(count["end"]<channel):
                count["end"] = count["end"] + 1   

        else:    # data != start/end --> write data to rec_data
            # print("tmp1")
            # print(rec_message)
            # print("tmp2")
            # print(base64.b64decode(rec_message))
            rec_data = rec_data + base64.b64decode(rec_message)
            # print("tmp3")
            # print(rec_data)

    return

def FileAssemble(count, channel, data_channel_message):
    while (True):
        if(count["start"] == channel and count["end"] == channel):
            # f = open("result/result3.jpg","wb")
            for i in range(channel):
                print(data_channel_message[i])
                # f.write(data_channel_message[i])
            # f.close
            # count["start"] = 0
            # count["end"] = 0
            # for i in range(channel):
            #     data_channel_message[i] = b""

    return

def Main():
    ser = None
    # port = sys.argv[1]
    channel = int(sys.argv[1])   # number of channel
    #begin connect to serial port
    #log(out,"Connecting to {}".format(port))

    ser={}
    for i in range(channel):
        ser[i] = serial.Serial()
        ser[i].baudrate = 115200
        ser[i].port = input("Enter port name: ")
        ser[i].open()
        while (ser[i].is_open!= True):
            continue

    print(ser)

    count_dic = {}
    count_dic["start"] = 0
    count_dic["end"] = 0

    # q = multiprocessing.Queue()

    data_channel_message = {}
    for i in range(channel):
        data_channel_message[i] = b""

    SerialRx = {}
    for i in range(channel):
        SerialRx[i] = threading.Thread(name="SerialRead", target=SerialRead, args=[ser[i], i, channel, data_channel_message[i], count_dic])

    for i in range(channel):
        SerialRx[i].start()

    # SaveFile = threading.Thread(name="FileAssemble", target=FileAssemble, args=[count_dic, channel, data_channel_message])
    # SaveFile.start()

if __name__ == '__main__':
    Main()