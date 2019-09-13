
Design around gemssensors.com's E3310-V-VO-BM-C201-OC. This requires about a 1/4 of the energy (0.65W) and can run off 5V. This allows for a smaller battery and remove the 12V stepup module.

- Subminiature Solenoid Gems Sensors: E3110-V-VO-BM-C200-OC
  - [order directly](https://ecatalog.gemssensors.com/ecatalog/configurators/EG/E3110)
  - min order 10, 630 USD
- SSR AQY212GH
- Diff Pressure Sensor: MPXV7002DP
- [Adafruit BMP280 Altimeter](https://www.adafruit.com/product/2651)


---------

Further we can drop the display module and replace it with

Two operation modes, adaptive, 100% switchable by breathing code.

Two start altitude modes switchable by applying underpressure when powering up.

Use an adafruit feather instead of trinket and power boost.

Components to drop:
- display
- push button
- 12V power module
- 2500 to 2000 mA battery
- Feather instead of trinket and PowerBoost 500
