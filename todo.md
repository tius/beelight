# to do - personal notes

## low level
- status led: helligkeit regeln
- status led: fehler, charger, battery low ...
- ota
- httpd?
- telnetd?
- station / wps
- esp-now, cli, radiowake

## high level
- detect dancing
- ir interaction
- esp-now control
- bump to pair
- tv-b-gone

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
- gebrauchsmuster anmelden

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
- WithXyz statt XyzCrtp
- namen
- clean-up codepath
- kommentare
