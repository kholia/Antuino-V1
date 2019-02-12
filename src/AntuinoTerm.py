"""

# https://matplotlib.org/gallery/user_interfaces/embedding_in_wx5_sgskip.html

================
Embedding In Wx5
================

"""

import sys
import time
import serial
import optparse
import serial.tools.list_ports

import numpy as np

import wx
import wx.lib.agw.aui as aui
import wx.lib.mixins.inspection as wit

import matplotlib as mpl
import matplotlib.ticker as ticker
from matplotlib.backends.backend_wxagg import FigureCanvasWxAgg as FigureCanvas
from matplotlib.backends.backend_wxagg import NavigationToolbar2WxAgg as NavigationToolbar


class Plot(wx.Panel):
    def __init__(self, parent, id=-1, dpi=None, **kwargs):
        wx.Panel.__init__(self, parent, id=id, **kwargs)
        self.figure = mpl.figure.Figure(dpi=dpi, figsize=(32, 32))
        self.canvas = FigureCanvas(self, -1, self.figure)
        self.toolbar = NavigationToolbar(self.canvas)
        self.toolbar.Realize()

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.canvas, 1, wx.EXPAND)
        sizer.Add(self.toolbar, 0, wx.LEFT | wx.EXPAND)
        self.SetSizer(sizer)

        self.theplot = self.figure.add_subplot(111)
        self.theplot.set_title("")
        self.theplot.set_xlabel("Frequency (MHz)")
        self.theplot.set_ylabel("SWR")


class PlotNotebook(wx.Panel):
    def __init__(self, parent, id=-1):
        wx.Panel.__init__(self, parent, id=id)
        self.nb = aui.AuiNotebook(self)
        sizer = wx.BoxSizer()
        sizer.Add(self.nb, 1, wx.EXPAND)
        self.SetSizer(sizer)

    def add(self, name="plot"):
        page = Plot(self.nb)
        self.nb.AddPage(page, name)
        return page.figure


template = """frequencies = %s

sws = %s
"""


def main(startf="135000000", endf="t150000000", step_size="500000", xtick=0.5, ytick=0.1):
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
        print("I couldn't find a connected antuino. Exiting!")
        time.sleep(4)
        sys.exit(0)

    ser = serial.Serial(device, baudrate=9600)  # Arduino Nano v2 clone on Linux
    # ser.flushInput()

    # wait for the connection to settle down
    time.sleep(3)

    ser.write(b"s%s\n" % step_size.encode("ascii"))
    ser.write(b"f%s\n" % startf.encode("ascii"))
    ser.write(b"t%s\n" % endf.encode("ascii"))

    ser.write(b"g\n")

    ser.flushInput()

    values = []

    frequencies = []
    swrs = []

    while True:
        line = ser.readline().strip()
        line = line.decode("ascii")
        print(line)

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

    # draw graph
    app = wit.InspectableApp()
    frame = wx.Frame(None, -1, 'AntuinoTerm', size=(1024,768))
    plotter = PlotNotebook(frame)
    ax = plotter.add('SWR sweep').gca()
    ax.plot(frequencies, swrs)
    start, end = ax.get_xlim()
    starty, endy = ax.get_ylim()
    ax.xaxis.set_ticks(np.arange(start, end, xtick))
    ax.xaxis.set_major_formatter(ticker.FormatStrFormatter('%0.1f'))
    ax.yaxis.set_ticks(np.arange(starty, endy, ytick))
    ax.yaxis.set_major_formatter(ticker.FormatStrFormatter('%0.1f'))

    frame.Show()
    app.MainLoop()


if __name__ == "__main__":
    parser = optparse.OptionParser()
    parser.add_option('-f', action="store", dest="startf", help="start frequency", default="140000000")
    parser.add_option('-t', action="store", dest="endf", help="stop frequency", default="150000000")
    parser.add_option('-s', action="store", dest="step_size", help="sweep step size", default="500000")
    parser.add_option('-x', action="store", dest="xtick", help="x-axis tick value", default=0.7, type="float")
    parser.add_option('-y', action="store", dest="ytick", help="y-axis tick value", default=0.1, type="float")
    parser.add_option('-d', action="store_false", dest="demo", help="demo mode (2m)", default=False)

    options, remainder = parser.parse_args()

    if len(sys.argv) < 2 and not options.demo:
        print("Usage: %s -f <start frequency (hertz)> -t <stop frequency> [-s <step size>]" % sys.argv[0])
        print("\nExample: python %s -f 140000000 -t 150000000\n" % sys.argv[0])
        for option in parser.option_list:
            if option.default != ("NO", "DEFAULT"):
                option.help += (" " if option.help else "") + "[default: %default]"
        parser.print_help(sys.stderr)
        sys.exit(0)


    main(options.startf, options.endf, options.step_size, options.xtick, options.ytick)
