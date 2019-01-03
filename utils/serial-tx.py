import sys
import serial
import time
from datetime import datetime

RECONNECT_TIME = 5

#############################
def log(out,line):
    print(datetime.now(),line,file=out)
    print(datetime.now(),line)
    out.flush()

port = sys.argv[1]


#begin connect to serial port
#log(out,"Connecting to {}".format(port))
ser = serial.Serial()
ser.baudrate = 115200
ser.port = port
ser.open()

c = 130
buffData1 = b"\x7e"+bytes([1, 130])+b"12"*60
#print(buffData)
count = 0

while (True):
    if (ser.inWaiting()>0): #if incoming bytes are waiting to be read from the serial input buffer
        data_str = ser.read(ser.inWaiting()).decode('ascii') #read the bytes and convert from binary array to ASCII
        print(data_str, end='') #pri 

    count_str = str(count)
    buffData = buffData1 + b"0"*(10-len(count_str)) + bytearray(count_str, 'utf8')
    if (ser.out_waiting == 0):
        y = ser.write(buffData)
    #    print(y)
    # if (ser.out_waiting() > 0):
    #     print()
    count=(count+1)%1024
ser.close()

