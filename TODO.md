Hannes Steps 2020-10-03 -> inplemented in 3.0.4
-----------------------
- baro first, then modulate by spo2
- alt simulation mode by holding user1_btn at startup
  - paperclip between pin 1 and 4 of DB9 connector

Waveglide 3X firmware
---------------------
- check protocol
- tune breathFAST, breathLOW
- status led
  - breathing amp -> lumination
  - spo2 mode -> blue on inhale
  - altimeter fallback -> green on inhale
  - low spo2 -> pink on exhale
  - low bat -> red on exhale
- status buzzer

Waveglide 3X v3.1 Issues
------------------------
- feather footprint usb on wrong side
- update netlist
- case
  - buzzer funnel
  - valve screw holes
  - valve cover in different color?


Beta-01 Issues
--------------
- backfeeding of power through sense lines (LB, BAT, USB)
  - connect BAT by 1.33K resistor
- some traces interfere with ChargePower500 mount holes
  (were changed without notice by PCB fabricator)
- some mount holes blocked
  - move 12V booster a bit
- move buzzer inward a bit
- breath snsor difficult to mount
  - use pads and mount rotated
- sdcard unaccessible (maybe ok, since not used)



Alpha-01 Issues
---------------
- e-valve too close to sensor
- display header flipped
- barometric module wrong pins
- buzzer space to crowded
- power lines missing
- gnd lines missing
- 12V booster header flipped (may be ok)
- 12V booster does not power when testing via USB
- not charging when board off
- button pin not allowing interrupt (move to 2 or 3)
- sense charging via USB pins
- sense low-bat via LB pin
- sense bat-V via analog in

- expose serial pins (for external pulsoxy)
- coupling for constant flow option (external)
