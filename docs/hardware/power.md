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
- MCU enters shipping mode via I2C (`FET_DIS`)
- MP2667 can also cut battery discharge path when `INT` is held low for >8 s
- MP2667 can exit shipping mode by pulling `INT` low for >500 ms

### BQ27421

- provides battery state telemetry
- firmware restores battery-model and hibernate data-memory settings during
	startup
- firmware clears the hibernate request during startup while configuration is
	restored
- firmware does not modify Fast-Hibernate data-memory settings
- periodic gauge updates read one cached value, then arm hibernate mode as the
	last BQ27421 I2C access
- actual hibernate entry happens when the gauge sees a relaxed cell and
	current below `Hibernate I`
- shutdown mode is intentionally not used because it can discard learned battery
	data and can make wake behavior unreliable

### WS2812 high-side switch

- isolates the led rail from the system rail when switched off

### Wake and reset wiring

- Rysta wake circuit generates a short RST pulse when WAKE goes low
- the on-board Rysta button is not used in this product
- a separate rear button on the piggyback drives WAKE via a diode-isolated path
- the same rear button drives MP2667 INT via a second diode-isolated path
- consequence: a long rear-button hold can trigger MP2667 INT timing behavior
	independent of firmware
- BQ27421 GPOUT is not connected to the wake/reset path, only pull-up is
	present

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
- firmware updates the gauge once per second
- each update reads one cached value from the BQ27421, then sends
	`SET_HIBERNATE` as the last BQ27421 command
- status uses cached `SOC` and `AverageCurrent`
- detail logging uses cached `Voltage`, `RemainingCapacity`, and
	`FullChargeCapacity`
- status and detail reads do not access the BQ27421 directly

### led rail

- WS2812 power is switched, then fused
- high-side switch first, polyfuse next

### notes

- no boost converter is present in this design
- Rysta micro-USB for +5V input is not used in product

## signal flow

- MP2667 and BQ27421 are controlled by the MCU via I2C
- piggyback rear button pulls WAKE low and triggers an RST pulse via Rysta
	wake logic
- piggyback rear button also feeds MP2667 INT via a diode-isolated path
- wake wiring is defined in "Wake and reset wiring" above

## charging mode

- charging uses USB-C input and MP2667
- MCU can configure charging behavior on MP2667 via I2C
- charging and system operation can run at the same time via MP2667 power-path
- MCU can read charger state from MP2667 via I2C
- firmware publishes cached battery state from the periodic BQ27421 update

## active operation mode

- operation can run from battery or USB-C input
- MCU can read active power source from MP2667 via I2C
- firmware reads battery discharge current through the periodic BQ27421 update
- with external USB-C power present, a long rear-button hold (>8 s on INT)
	can disconnect the battery while the system keeps running from input power

### corner case: expected int-triggered battery disconnect

- with external USB-C power present, INT low for >8 s disconnects the battery
	path while the system can continue running from input power
- without external input, the same transition removes system supply and the
	device powers off
- this transition is driven by MP2667 INT timing and is independent of
	firmware control

### rare undefined behavior after int-triggered disconnect

- in rare cases, the sequence above can lead to an undefined operating state
- observed symptoms can include incomplete boot, unstable system voltage, and
	unavailable I2C devices
- the behavior is intermittent and currently not reliably reproducible
- working hypothesis (not confirmed):
	- MP2667 may enter a temporary undefined internal state after INT-triggered
		battery disconnect while external power remains present
	- the unstable state may only become visible on the next reset
	- one observed symptom pattern is boot-loop-like startup with only early boot
		messages and about 2V on the system rail
	- after a shipping-mode off transition, the LDO path may recover while I2C
		can remain blocked until full power removal
	- a plausible cause is startup reconfiguration while MP2667 is in a corner
		state, this remains speculation
- recovery sequence:
	- disconnect external power
	- toggle shipping mode off/on via rear-button sequence

### options to infer shipping mode in firmware

- MP2667 does not provide a dedicated persistent shipping-state flag
- a practical heuristic is to detect a stable REG07 transition from charging or
	charge-done to not-charging while power-good remains high
- this heuristic could be combined with timing context from the rear-button
	hold window around the expected INT threshold
- REG06 `FET_DIS` is not a reliable detector because the bit can auto-clear
	after battery FET off
- REG08 fault bits can provide hints but are not sufficient as a primary
	shipping detector
- classify the result as shipping-likely, not shipping-confirmed

## standby mode

- MCU enters deep sleep
- MCU switches off the led rail before deep sleep
- wake is possible via ESP deep sleep timer or piggyback rear button
- current below 100 uA is targeted

## power off

- firmware switches off the led rail and flushes logs, then avoids BQ27421
	access before shipping mode
- BQ27421 hibernate mode is kept armed by periodic gauge updates
- MCU enters MP2667 shipping mode via I2C
- after MP2667 shipping removes the system load, BQ27421 can enter hibernate
	automatically when its own conditions are met
- BQ27421 shutdown mode is not used, to preserve learned battery data and
	avoid wake problems
- exit from shipping mode requires MP2667 INT low pulse (>500 ms)
- current well below 10 uA is targeted
