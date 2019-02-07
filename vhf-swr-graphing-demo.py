#!/usr/bin/env python

import serial
import time

# ser = serial.Serial('/dev/ttyACM0')
ser = serial.Serial('/dev/ttyUSB0', baudrate=9600)
# ser.flushInput()

time.sleep(3)

# Fixed for a VHF antenna for now
ser.write(b"f135000000\n")
ser.write(b"t150000000\n")

ser.write(b"g\n")

ser.flushInput()

values = []

frequencies = []
swrs = []

while True:
    line = ser.readline().strip()

    if "end" in line:
        break

    try:
        marker, frequency, something, swr = line.split(":")
    except:
        pass
        continue

    swr = int(swr) / 10.0
    frequency = int(frequency) / 1000000.0

    print(frequency, swr)
    values.append((frequency, swr))
    frequencies.append(frequency)
    swrs.append(swr)


print(values)
print(frequencies)
print(swrs)

import matplotlib.pyplot as plt
import numpy as np
import matplotlib.ticker as ticker

fig, ax = plt.subplots()
ax.plot(frequencies, swrs)

start, end = ax.get_xlim()
starty, endy = ax.get_ylim()
ax.xaxis.set_ticks(np.arange(start, end, 0.5))
ax.xaxis.set_major_formatter(ticker.FormatStrFormatter('%0.1f'))
ax.yaxis.set_ticks(np.arange(starty, endy, 0.1))
ax.yaxis.set_major_formatter(ticker.FormatStrFormatter('%0.1f'))

plt.show()
