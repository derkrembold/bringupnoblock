import serial
import os
import time
import sys
import RPi.GPIO as GPIO
from linaddresses import constants
import numpy as np
import pygame
import subprocess

MAXSET = 160
MINSET = -160

class Lin:

    def __init__(self):
        SLP_PIN = 23  # GPIO16
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(SLP_PIN, GPIO.OUT)
        GPIO.output(SLP_PIN, GPIO.HIGH)  # LIN-Transceiver aktivieren
        
        self.ser = serial.Serial(
            port='/dev/ttyS0',  # Standard UART des Raspberry Pi
            baudrate=19200,       # LIN-Bus Baudrate
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=2 # Timeout für Empfang erhoeht
        )
        
        
    def checksum(self, data):
        check = 0
        for abyte in data:
            check += abyte
            if check > 0xFF:
                check -=  0xFF
        check = (~check) & 0xFF
        return check
    


    def addparity(self, pid):
        
        temp = pid & 0x3F;
        p0 = ((pid & 0x01) + ((pid >> 1) & 0x01) + ((pid >> 2) & 0x01) + ((pid >> 4) & 0x01)) & 0x01;
        p1 = (~(((pid>>1) & 0x01) + ((pid >> 3) & 0x01) + ((pid >> 4) & 0x01) + ((pid >> 5) & 0x01))) & 0x01;
        
        temp = (p0 << 6) | temp;
        temp = (p1 << 7) | temp;
        return temp;


    def write(self, address, data):

        if address not in constants.pids:
            print("pid not known")
            return -1
        index = constants.pids.index(address)
        mbytes = constants.messagebytes[index]
        if len(data) != mbytes:
            print("number of bytes wrong")
            return -2
        
        if constants.sources[index] != constants.master:
            print("you must be master to write")
            return -3
        
        self.ser.write(bytes([constants.sync]))
        response = self.ser.read(1)
        #print(f"Raw: {response.hex()}")
        self.ser.write(bytes([self.addparity(address)]))
        response = self.ser.read(1)
        #print(f"Raw: {response.hex()}")
        
        for b in data:
            self.ser.write(bytes([b]))
            response = self.ser.read(1)
            #print(f"Raw: {response.hex()}")

        
        self.ser.write(bytes([self.checksum(data)]))
        response = self.ser.read(1)
        #print(f"Raw: {response.hex()}")

        return 0


    def read(self, address):

        #print("address: ", address)
        #print("constants: ", constants.pids)
        
        if address not in constants.pids:
            print("pid not known")
            return -1, []

        index = constants.pids.index(address)
        mbytes = constants.messagebytes[index]

        if constants.sources[index] == constants.master:
            print("you must be client to write")
            return -2, []
        
        self.ser.write(bytes([constants.sync]))
        response = self.ser.read(1)
        #print(f"Raw: {response.hex()}")
        self.ser.write(bytes([self.addparity(address)]))
        response = self.ser.read(1)
        #print(f"Raw: {response.hex()}")

        data = []
        for b in range(0,mbytes):
            response = self.ser.read(1)
            #print(type(response[0]))
            data.append(response[0])
            #print(f"Raw: {response.hex()}")

        response = self.ser.read(1)
        #print(f"Raw: {response.hex()}")
        if response[0] != self.checksum(data):
            print("checksum not right")
            return -3, []
            
        return 0, data
    
    def close(self):
        self.ser.close()
        GPIO.cleanup()


def addparity(pid):
    
    temp = pid & 0x3F;
    p0 = ((pid & 0x01) + ((pid >> 1) & 0x01) + ((pid >> 2) & 0x01) + ((pid >> 4) & 0x01)) & 0x01;
    p1 = (~(((pid>>1) & 0x01) + ((pid >> 3) & 0x01) + ((pid >> 4) & 0x01) + ((pid >> 5) & 0x01))) & 0x01;
    
    temp = (p0 << 6) | temp;
    temp = (p1 << 7) | temp;
    return temp;


def checksum(data):
    check = 0
    for abyte in data:
        check += abyte
        if check > 0xFF:
            check -=  0xFF
    check = (~check) & 0xFF
    return check



"""
def main():

    dw = {"SLV0": constants.cntlslv0, "SLV1": constants.cntlslv1, "SLV2": constants.cntlslv2 ,"SLV3": constants.cntlslv3}
    data = [0xaa, 0xcd]

    lin = Lin()
    ret = 0

    try:
        while True:
            #print("1")
            ret = lin.write(dw["SLV1"], data)
            if ret < 0:
                print(f"something wrong with run: {ret}")
                break
            time.sleep(0.1)
    except KeyboardInterrupt:
        lin.close()
        print("done")

        
    return
"""

def getindex(pid):
    for i, apid in enumerate(constants.pids):
        if apid == pid:
            return i
    return -1

def main():


    SLP_PIN = 23  # GPIO16
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(SLP_PIN, GPIO.OUT)
    GPIO.output(SLP_PIN, GPIO.HIGH)  # LIN-Transceiver aktivieren
        
    ser = serial.Serial(
        port='/dev/ttyS0',  # Standard UART des Raspberry Pi
        baudrate=19200,       # LIN-Bus Baudrate
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=2 # Timeout für Empfang erhoeht
    )



    if sys.argv[1] == "on":
        ser.write(bytes([0x55, addparity(constants.cntlslv0)]))
        data = [0x01, 0xab]
        ser.write(bytes(data))
        ser.write(bytes([checksum(data)]))

        
    if sys.argv[1] == "off":
        ser.write(bytes([0x55, addparity(constants.cntlslv0)]))
        data = [0xcd, 0x0c]
        ser.write(bytes(data))
        ser.write(bytes([checksum(data)]))


    if sys.argv[1] == "hal":
        address = constants.stslv0
        ser.write(bytes([0x55]))
        response = ser.read(1)
        ser.write(bytes([addparity(address)]))
        response = ser.read(1)

        index = getindex(address)
        print("index: ", index)
        
        data = []
        for i in range(0,constants.messagebytes[index]):
            response = ser.read(1)
            print(hex(response[0]))
            #print(type(response[0]))
            data.append(response[0])
            #print(f"Raw: {response.hex()}")

        response = ser.read(1)
        print(f"Raw: {response.hex()}")
        if response[0] != checksum(data):
            print("checksum not right")
            


    if sys.argv[1] == "temp":
        address = constants.stslv1
        ser.write(bytes([0x55]))
        response = ser.read(1)
        ser.write(bytes([addparity(address)]))
        response = ser.read(1)
        
        index = getindex(address)
        #print("index: ", index)
        
        data = []
        for i in range(0,constants.messagebytes[index]):
            response = ser.read(1)
            #print("Raw: ", hex(response[0]))
            data.append(response[0])

        response = ser.read(1)
        #print(f"Raw: {response.hex()}")
        if response[0] != checksum(data):
            print("checksum not right")

        print(f"Value: {hex(data[0]+256*data[1])}")
        
        
    time.sleep(0.2)
            
 

    ser.close()
    GPIO.cleanup()
    return


if __name__ == "__main__":

    main()

    
