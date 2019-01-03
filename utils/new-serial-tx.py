import sys
import serial
import time
import os
import math

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

def SerialRead(index, ser):
    while (True):
        if (ser.inWaiting()>0): #if incoming bytes are waiting to be read from the serial input buffer
            # print('ser.inWaiting = ' + str(ser.inWaiting()));
            data_str = ser.read(ser.inWaiting()).decode('ascii') #read the bytes and convert from binary array to ASCII
            print(f"{index}: {data_str}", end='') #pri 
    return

def read_file(path, block_size, start_pointer, end_pointer): 
    with open(path, 'rb') as f: 
        f.seek(start_pointer * block_size)
        for i in range(end_pointer-start_pointer): 
            piece = base64.b64encode(f.read(block_size)) 
            if piece: 
                yield piece 
            else: 
                return

def SerialWriteFromFile(ser, inFile, ID, channel):
    b_size=59
    file_size = os.path.getsize(inFile) # get file size
    block_num = math.ceil(file_size/b_size) # get number of blocks
    block_per_channel = block_num//channel # get number of blocks per channel
    remainder = block_num % channel

    start_block_id = block_per_channel*ID + min(ID,remainder) # store the start block id
    print(start_block_id)
    end_block_id = block_per_channel*(ID+1) + min(ID+1,remainder) # store the end block id
    print(end_block_id)
    # logging.debug("Serial Write"+" "+inFile)
    if os.path.exists(inFile):
        #send starting data frame
        start_frame = FrameConstruction("start".encode(), start_block_id) # starting block ID = start_block_id
        ser.write(start_frame)    # write end_frame to serial
        # print("start") 
        print(f"{ID}: {start_block_id} - {start_frame}")

        # send all data frame
        block_id = start_block_id
        for p in read_file(inFile, b_size, start_block_id, end_block_id):
            frame = FrameConstruction(p, block_id)    # construction frame
            ser.write(frame)    # write frame to serial

            print(f"{ID}: {block_id} - {frame}")
            block_id = block_id + 1
        
        #send ending data frame
        end_frame = FrameConstruction("end".encode(), end_block_id) # ending block ID = end_block_id
        ser.write(end_frame)    # write end_frame to serial
        print(f"{ID}: {end_block_id} - {end_frame}")
        # print("end") 
        
    return

def FrameConstruction(message, block_id):
    frame_sfd = 0x7e
    frame_msgType = 0x01
    frame_block_id = struct.pack("H", block_id)

    frame_len = len(frame_block_id) + len(message)
    
    frame_hdr =  struct.pack("BBB", frame_sfd, frame_msgType, frame_len) #frame header
    frame = frame_hdr + frame_block_id + message
    
    return frame

def Main():
    ser = None
    # port = sys.argv[1]
    channel = int(sys.argv[1])   # number of channel
    inputFile = "/Users/pithayuth.c/Desktop/iwing-adafruit-m0-lora/PICT0001.JPG"
    # begin connect to serial port
    # log(out,"Connecting to {}".format(port))
    
    ser={}
    for i in range(channel):
        ser[i] = serial.Serial()
        ser[i].baudrate = 115200
        ser[i].port = input("Enter port name: ")
        ser[i].open()
        while (ser[i].is_open!= True):
            continue

    print(ser)

    q = multiprocessing.Queue()

    # SerialRx = threading.Thread(name="SerialRead", target=SerialRead, args=[ser])
    # SerialTx = threading.Thread(name="SerialWriteFromFile", target=SerialWriteFromFile, args=[ser, inputFile])

    SerialTx = {}
    SerialRx = {}
    for i in range(channel):
        SerialTx[i] = threading.Thread(name="SerialWriteFromFile", target=SerialWriteFromFile, args=[ser[i], inputFile, i, channel])
        SerialRx[i] = threading.Thread(name="SerialRead", target=SerialRead, args=[i, ser[i]])
    
    for i in range(channel):
        SerialTx[i].start()
        SerialRx[i].start()
    # SerialTx[0].start()
    # SerialRx[0].start()
    # SerialTx[1].start()
    # SerialTx[1].start()

if __name__ == '__main__':
    Main()