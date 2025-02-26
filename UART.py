from serial.tools import list_ports, list_ports_common
from serial import Serial
import matplotlib.pyplot as plt
from struct import unpack


def serial_read():
    TARGET = list_ports_common
    for COM in list_ports.comports():
        if COM.serial_number != None:
            TARGET = COM
    with Serial(TARGET.name) as s:
        while s.is_open:
            line = s.readline()
            if line == b"MS5837_02BA\n":
                while s.is_open:
                    line = s.read(4)
                    data = unpack("f", line)[0]
                    if data >= -20 and data <= 85:
                        print(data)
                    elif data >= 300 and data <= 1200:
                        print(data)
                    else:
                        print("UNRECOGNIZED DATA")
            else:
                print(line)


serial_read()
