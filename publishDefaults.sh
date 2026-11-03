mosquitto_pub -h 192.168.0.40 -p 1883 -t esp32_clock/offset -m 0
mosquitto_pub -h 192.168.0.40 -p 1883 -t esp32_clock/brightness -m '30'
mosquitto_pub -h 192.168.0.40 -p 1883 -t esp32_clock/handleHourColor -m 16711680
mosquitto_pub -h 192.168.0.40 -p 1883 -t esp32_clock/handleMinuteColor -m 65280
mosquitto_pub -h 192.168.0.40 -p 1883 -t esp32_clock/handleSecondColor -m 16776960

mosquitto_pub -h 192.168.0.40 -p 1883 -t esp32_clock/handleTickColor -m 8355711

mosquitto_pub -h 192.168.0.40 -p 1883 -t esp32_clock/dimOn -m 1
mosquitto_pub -h 192.168.0.40 -p 1883 -t esp32_clock/valueDim -m 20
mosquitto_pub -h 192.168.0.40 -p 1883 -t esp32_clock/hourStartDim -m 20
mosquitto_pub -h 192.168.0.40 -p 1883 -t esp32_clock/hourStopDim -m 6

mosquitto_pub -h 192.168.0.40 -p 1883 -t esp32_clock/dumpValues -m 1

mosquitto_pub -h 192.168.0.40 -p 1883 -t esp32_clock/save_Pref -m 1

exit

{
  "offset": 0,
  "brightness": 20,
  "handleHourColor": 16711680,
  "handleMinuteColor": 65280,
  "handleSecondColor": 16776960,
  "tickColor": 3289650,
  "dimOn": true,
  "valueDim": 10,
  "hourStartDim": 20,
  "hourStopDim": 6
}
