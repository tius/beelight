# power

power documentation for Rysta plus piggyback.

see also:
- [Rysta board](rysta.md)
- [piggyback board](piggyback.md)

## goals

- simultaneous operation and charging (power path)
- no dedicated power switch
- low current in standby mode
- minimal current in power off mode
- configurable charging parameters
- accurate charger state
- accurate battery state

## implementation components

### MP2667

- configurable charger with power-path
- provides power off behavior via shipping mode
- MCU enters shipping mode via I2C
- requirement: MP2667 can leave shipping mode only via INT

### BQ27421

- provides battery state telemetry
- MCU enters hibernate mode via I2C for off mode
- shutdown mode is intentionally not used because it can discard learned battery
	data and can make wake behavior unreliable

### WS2812 high-side switch

- isolates the led rail from the system rail when switched off

### Rysta wake logic

- provides button-driven reset pulse generation on RST
- wake signal is routed via diode to MP2667 INT
- RST signal is routed via diode to BQ27421 GPOUT
- BQ27421 GPOUT wake path remains wired but is not used in normal firmware

## power flow

```text
USB-C -> MP2667 -> VOUT_MP2667 -> Rysta +5V rail -> LDO -> 3V3

Li-ion battery (external) <-> MP2667 BAT
Li-ion battery (external) <-> BQ27421 (on piggyback)

Rysta +5V rail -> high-side switch TPS22917L -> polyfuse 1.5 A -> VLED (WS2812)
```

### system rail

- USB-C power enters at MP2667
- MP2667 VOUT feeds the Rysta +5V rail
- Rysta +5V is a legacy rail name, real level tracks battery and is
	typically about 3.0 to 4.2 V
- on Rysta, the LDO converts this rail to 3V3 for MCU and low-voltage logic

### battery and gauge

- Li-ion battery is external
- BQ27421 is on the piggyback and observes the external battery path

### led rail

- WS2812 power is switched, then fused
- high-side switch first, polyfuse next

### notes

- no boost converter is present in this design
- Rysta micro-USB for +5V input is not used in product

## signal flow

- MP2667 and BQ27421 are controlled by the MCU via I2C
- button triggers an RST pulse via wake logic on Rysta
- wake and RST diode routing is defined in "Rysta wake logic" above

## charging mode

- charging uses USB-C input and MP2667
- MCU can configure charging behavior on MP2667 via I2C
- charging and system operation can run at the same time via MP2667 power-path
- MCU can read charger state from MP2667 via I2C
- MCU can read battery state from BQ27421 via I2C

## active operation mode

- operation can run from battery or USB-C input
- MCU can read active power source from MP2667 via I2C
- MCU can read battery discharge state from BQ27421 via I2C

## standby mode

- MCU enters deep sleep
- MCU switches off the led rail before deep sleep
- wake is possible via ESP deep sleep timer or button
- current below 100 uA is targeted

## power off

- MCU enters BQ27421 hibernate mode via I2C
- MCU enters MP2667 shipping mode via I2C
- BQ27421 shutdown mode is not used, to preserve learned battery data and
	avoid wake problems
- exit from shipping mode requires MP2667 INT
- current well below 10 uA is targeted
