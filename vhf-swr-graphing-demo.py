#!/usr/bin/env python

import sys
import time
import serial
import optparse
import serial.tools.list_ports

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

template = """frequencies = %s

sws = %s
"""


def main(startf="135000000", endf="t150000000", step_size="500000"):
    # ser = serial.Serial('/dev/ttyACM0')

    # auto-detect antuino port
    """
    $ lsusb
    ...
    $ Bus 001 Device 022: ID 1a86:7523 QinHeng Electronics HL-340 USB-Serial adapter
    ...
    """

    ports = list(serial.tools.list_ports.comports())
    found = None
    for p in ports:
        if p.vid == 0x1a86:
            found = p
            device = p.device
            break

    if not found:
        print("I couldn't find a connected antuino. Exiting!\n")
        sys.exit(0)

    ser = serial.Serial(device, baudrate=9600)  # Arduino Nano v2 clone on Linux
    # ser.flushInput()

    # wait for the connection to settle down
    time.sleep(3)

    ser.write(b"s%s\n" % step_size)
    ser.write(b"f%s\n" % startf)
    ser.write(b"t%s\n" % endf)

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

    timestr = time.strftime("%Y%m%d-%H%M%S")
    with open("logbook-%s.py" % timestr, "w") as f:
        f.write(template % (frequencies, swrs))

    fig, ax = plt.subplots()
    ax.plot(frequencies, swrs)
    plt.xlabel("Frequency (MHz)")
    plt.ylabel("SWR")

    if True:
        start, end = ax.get_xlim()
        starty, endy = ax.get_ylim()
        ax.xaxis.set_ticks(np.arange(start, end, 0.5))
        ax.xaxis.set_major_formatter(ticker.FormatStrFormatter('%0.1f'))
        ax.yaxis.set_ticks(np.arange(starty, endy, 0.1))
        ax.yaxis.set_major_formatter(ticker.FormatStrFormatter('%0.1f'))

    plt.show()


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: %s -f <start frequency (hertz)> -t <stop frequency> [-s <step size>]" % sys.argv[0])
        print("\nExample: python %s -f 140000000 -t 150000000" % sys.argv[0])
        sys.exit(0)

    parser = optparse.OptionParser()
    parser.add_option('-f', action="store", dest="startf", help="start frequency")
    parser.add_option('-t', action="store", dest="endf", help="stop frequency")
    options, remainder = parser.parse_args()

    main(options.startf, options.endf)
