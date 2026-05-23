# piggyback board

custom daughter PCB. soldered onto the Rysta GPIO pads. becomes the
visible product front.

## role in product

- product front
- carries front LEDs, IR TX/RX, motion and ambient sensors
- carries charger, fuel gauge, USB-C input, battery connector
- provides switched WS2812 rail and IR TX driver

## components

| part         | function                              |
| ------------ | ------------------------------------- |
| Rysta board  | see [rysta.md](rysta.md)              |
| MP2667       | I2C charger with power-path           |
| BQ27421-G1   | I2C fuel gauge                        |
| TPS22917LDBV | load switch for WS2812 rail (VLED)    |
| AO3400A      | low-side driver for IR TX LED         |
| polyfuse     | 1500 mA, in series with VLED          |
| BMA253       | accelerometer, I2C                    |
| VEML3328     | ambient / color sensor, I2C           |
| WS2812 x18   | front expression                      |
| USB-C        | charge input, power only              |

## led layout

- 18 × WS2812 chain on the ws2812 data signal
- supplied via VLED (switched VOUT_MP2667, see below)
- arranged behind hex diffuser
- diffuser: two concentric hexagons, outer edge ~30 mm, inner edge ~15 mm
- IR TX/RX and VEML3328 optics are behind the diffuser

## ws2812 power switch

- load switch TPS22917 between VOUT_MP2667 (Rysta +5V rail) and VLED
- polyfuse 1500 mA in series with VLED
- enable controlled by the ws2812 power signal via diode + RC low-pass
- low-pass prevents glitches during boot and uart noise from toggling the rail
- WS2812 nominal 5V; runs from VBAT (~3.0-4.2V) here, accepted brightness drop

## ir transmitter

- IR TX LED driven by low-side MOSFET Q1 (AO3400A)
- gate driven by `IR_TX_GPIO`
- supply rail: VLED (switched)

## ir receiver

- IR RX module data on `IR_RX_GPIO`
- supply rail: VLED (switched)

## sensors

- BMA253 accelerometer, I2C, motion and orientation
- VEML3328 ambient / color sensor, I2C, behind front diffuser
- both share the I2C bus with charger and gauge

## power hardware

- single-cell li-ion via J2
- USB-C J3 for charge input, power only (no data, no PD)
- CC1/CC2 pulled to GND via 5k1 (sink advertising default current)
- MP2667 charger and power-path IC
- BQ27421 fuel gauge
- VOUT_MP2667 is the system 5V rail on Rysta
- see [power.md](power.md) for charging, shutdown, wake, and status behavior

## off-mode wake wiring

MP2667 and BQ27421 each have a dedicated control pin used as **input** on
this board (not as output). Both pins are tied to the Rysta wake lines
through diodes, so the existing rear button can wake them from off mode.

- MP2667 `INT` (input) -- diode -- Rysta `GPIO 16` / WAKE line
- BQ27421 `GPOUT` (input) -- diode -- Rysta `RST` line
- diode parts: BAT54W for INT and GPOUT paths
- see [power.md](power.md) for shipping and shutdown behavior

## mechanical

- octagonal/hex outline, matches diffuser
- thickness contribution dominated by battery on the back side
- total product thickness ~25 mm
