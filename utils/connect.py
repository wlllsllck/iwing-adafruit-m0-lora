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

#############################
if len(sys.argv) != 3:
    print("Usage: {} <device> <log>".format(sys.argv[0]))
    sys.exit(1)

port = sys.argv[1]
gateway = None

with open(sys.argv[2],"a") as out:
    while True:
        try:
            if gateway is None:
                log(out,"Connecting to {}".format(port))
                gateway = serial.Serial(port) 

            # Serial.readline() returns a byte array, so use decode() to
            # convert to string
            line = gateway.readline().decode().strip()
            log(out,line)

        except serial.SerialException:
            log(out, 
                "Cannot read from {}; "
                "try to reconnect in {} seconds".format(port,RECONNECT_TIME))
            gateway = None
            time.sleep(5)

        except Exception as e:
            # Unknown exception
            print(str(e))
            print(e.__class__.__name__)
            break

