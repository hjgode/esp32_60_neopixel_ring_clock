# esp32_60_neopixel_ring_clock

based on https://github.com/AndroidCrypto/ESP32_60_LED-Pixel_Ring_Clock

Arduino IDE 2.x

esp32 dev board

Added customization options by using variables for colors etc.

Settings can be changed using mqtt (/esp32_clock/...), yes it starts with a slash (due to pubsubclient :-( :

  possible mqtt values

  brightness  0-255
  offset      0-59

  handleMinuteColor as 32Bit Hex number, ie 001f3a40
  handleHourColor
  handleSecondColor
  tickColor

  dimOn
  valueDim
  hourStartDim
  hourStopDim
  dumpValues
  save_Pref
  load_Pref
  getAutoDimmed

psDashboard.py is a Python3 tkinter example showing how to control the settings remotely.

Settings are saved and reloaded between reboots.

The hour moves smoothly between the full hours, easier to read when you see it is after or before 11:00 AM.

