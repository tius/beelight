# gpio mapping

product-level gpio mapping across Rysta and piggyback.

## mapping

| signal       | esp gpio  | config macro   | dir | remarks                                  |
| ------------ | --------- | -------------- | --- | ---------------------------------------- |
| I2C SDA      | `gpio 0`  | `I2C_SDA_GPIO` | i/o | boot strap, pull-up 10k                  |
| I2C SCL      | `gpio 13` | `I2C_SCL_GPIO` | out | pull-up 10k                              |
| ws2812 data  | `gpio 2`  | `NEO_GPIO`     | out | boot strap, pull-up                      |
| ws2812 power | `gpio 3`  | `n/a`          | out | uart0 rxd; via diode + RC to TPS22917    |
| ir rx        | `gpio 4`  | `IR_RX_GPIO`   | in  |                                          |
| ir tx        | `gpio 5`  | `IR_TX_GPIO`   | out | drives low-side MOSFET (AO3400A)         |
| rear led r   | `gpio 15` | `n/a`          | out | active high, must be low on boot         |
| rear led g   | `gpio 12` | `n/a`          | out | active high                              |
| rear led b   | `gpio 14` | `n/a`          | out | active high                              |
| button       | `gpio 16` | `n/a`          | in  | rear button -> RST pulse (Rysta), also drives MP2667 INT |
