# Antuino

Arduino+Si5351 = Antenna/Network Analyzer

## Firmware Upgrade Process

1. On Windows, install the device driver for the CH340 / CH341 USB controller
   present on `antuino` using the following link.

   http://www.wch.cn/download/CH341SER_EXE.html

   Mirror URL: https://github.com/himalayanelixir/Arduino_USB_Drivers/tree/master/Windows

   On Linux, the CH340 / CH341 USB controller works automatically.

2. Connect `antuino` to a PC using a USB `A Male to Mini B` cable. External DC
   power is not required for programming the AA.

3. Install `Arduino IDE` and open the `antuino` sketch
   (`antuino_analyzer_27mhz_v2.ino`) in [Arduino IDE](https://www.arduino.cc/en/main/software).

4. In the Arduino IDE, select the following options.

   - Menu > Tools > Board = "Arduino Nano"

   - Menu > Tools > Processor > "ATmega328P (Old Bootloader)"

   - Menu > Tools > Programmers > "AVR ISP"

   This `Processor` setting is valid for the `antuino` version that was
   distributed during LARC 2019 event.

   https://www.arduino.cc/en/Guide/ArduinoNano has some additional details on
   this `Processor` setting.

5. Upload the sketch to `antuino` using the `Menu -> Sketch -> Upload` option
   in the Arduino IDE.

6. After upgrading the firmware, run the `Calibrate SWR` menu option on
   `antuino`. Note: Keep the antenna disconnected during this process.

7. Reboot the antuino device. After the reboot, antuino is ready to be used.


## Notes on powering antunio

The following power sources are stable:

* USB 2.0 and USB 3.0 ports on Desktop computers

* Chinese 6x18650 USB / 9v DC power banks

The following power sources are unstable:

* Small 9v battery


### Notes

1. Anil Sharma (VU2TFG) reports that `antunio` SWR values are pretty close to
   the SWR values shown by `GEK - 60MHz Antenna Analyzer by VU3GEK`.

   http://homebrewcorner.blogspot.com/p/page-update-642015-need-of-useful-tool.html

   The GEK AA feels pretty solid and has a dual-digit SWR display (e.g. 1.82).

   In comparison, `antunio` has a single digit SWR display (e.g. 1.8).

   The GEK AA is a fixed-function instrument and can't be programmed. In
   comparison, the possibilities are endless with `antunio`.
