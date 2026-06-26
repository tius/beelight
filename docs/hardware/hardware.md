# hardware

final product hardware overview. board-level and subsystem details live in
dedicated documents.

## electrical setup

### Rysta board
- existing board, product back
- MCU ESP8266EX, rear RGB LED, wake logic, and LDO
- on-board Rysta button exists but is not used in the product
- see [rysta.md](rysta.md)

### piggyback board
- custom daughter PCB, product front
- charging logic, IR RX + TX, WS2812, and light sensor
- connector for the rear product button, diode-isolated to WAKE and MP2667 INT
- [piggyback.md](piggyback.md)

### battery
- external single-cell li-ion battery

## mechanical setup

- two front-facing concentric hexagons
- outer edge ~30 mm, inner edge ~15 mm
- thickness ~25 mm, battery on product back
- front diffuser with outer and inner regions

## see also

- [power.md](power.md): rails, charging, shipping mode, wake paths
- [gpio-mapping.md](gpio-mapping.md): product GPIO signals and config macros
- [i2c-addresses.md](i2c-addresses.md): product I2C addresses and config macros
