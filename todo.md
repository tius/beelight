# to do - personal notes

## initial flash
- pogo pin adapter

## initial firmware
- detect missing piggyback board
- show status via led
- connect to predefined wifi network
- request ota





- check morse
- deployment / claiming
- effect engine
- ir interaction
- radio interaction
- g interaction
- deep sleep
- wake on button, radio, ir, g

## basics
- ota (signiert, ext. power / high battery)
- fs als singleton?
- selbsttest

## wifi client mode
- connect / disconnect / autoconnect?
- scan?


## sleep mode
- timer
- radiowake

## status led
- helligkeit regeln
- charger + battery

## config interface
- g-sensor?
- httpd?

## debug interface
- telnetd?
- ftpd?
- radiocmd shell

## api
- animation sandbox

## infrared
- ir interaction
- tv-b-gone

## esp now
- remote control
- remote cli (debugging)

## g-sensor
- orientation
- dance detection
- bump to pair

claim
- tap, double-tap
- schütteln
- flat / face-up / face-down

==========================================================
hardware setup
- sägen
- firmware
- qr-code?

claim
- feste ssid
- animation / status-led "claimable" 
- browser zeigt "tap n times"
- user tap
- browser zeigt gleiche individuelle "claimed" animation wie bee
- optional: lichtsensor mit farberkennung vor display
- optional: ir-remote




==========================================================

## low level
- station / wps
- esp-now, cli, radiowake

## toolkit
- clean-up logging
- check superloop load and latency
- [ temporal dithering ]
- esp-now
- modem sleep / light sleep?
- persist
- ota

## readme
- api for apps
- architektur, super-loop, heap-frei, event-bus ...
- cooperative scheduling

## docs
- gesamtstatus
- ux dokument aufteilen

## build
- tests build
- release version
- lto

==============================

## yagni
- parameter charger und gauge optimieren
- innenwiderstand akku testen
- kalibrierung led-verbrauch
- kalibrierung farbsensor
- flash mem für cli, logging

## stil
- weniger casts, insbesondere bei printf
- namen
- clean-up codepath
- kommentare
