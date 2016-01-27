
Beta-02
-------


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
