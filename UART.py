from serial.tools import list_ports
from serial import Serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from struct import unpack
from collections import deque
from threading import Thread
from time import sleep
from datetime import datetime

TEMPERATURE = deque(maxlen=60)
PRESSURE = deque(maxlen=60)
TIME_TEMP = deque(maxlen=60)
TIME_PRES = deque(maxlen=60)
ALIVE = 1


def serial_read():
    START_TIME = datetime.now()
    TARGET = None
    for COM in list_ports.comports():
        if COM.serial_number != None:
            TARGET = COM
    if TARGET == None:
        print("device not found")
        return

    with Serial(TARGET.name) as s:
        global ALIVE
        while s.is_open:
            line = s.readline()
            if line == b"MS5837_02BA\n":
                while ALIVE:
                    line = s.read(4)
                    data = unpack("f", line)[0]

                    if data >= -50 and data <= 120:
                        TEMPERATURE.append(data)
                        TIME_TEMP.append((datetime.now() - START_TIME).total_seconds())
                    elif data >= 0 and data <= 2000:
                        PRESSURE.append(data)
                        TIME_PRES.append((datetime.now() - START_TIME).total_seconds())
                    else:
                        print("UNRECOGNIZED DATA")
                        print(data)

                return
            else:
                print(line)


if __name__ == "__main__":
    sr = Thread(target=serial_read, args=[])
    sr.start()

    sleep(5)

fig, ax = plt.subplots()
graph = ax.plot(
    TIME_PRES,
    PRESSURE,
    linestyle="-",
    color="r",
)[0]
plt.ylim(0, 10)


# updates the data and graph
def update(frame):
    global graph

    # creating a new graph or updating the graph
    graph.set_xdata(TIME_PRES)
    graph.set_ydata(PRESSURE)
    try:
        plt.xlim(TIME_PRES[0], TIME_PRES[-1])
        plt.ylim(min(PRESSURE), max(PRESSURE))
    except:
        pass

anim = FuncAnimation(fig, update, frames=None)
plt.show()
