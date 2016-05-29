
# pulse oxymeter OEM module YS2000 by www.berry-med.com
# http://www.aliexpress.com/item/YS2000-Digital-LED-Screen-Spo2-Module/996279525.html
#

import os
import sys
import serial


PORT = "/dev/ttyACM0"
READ_TIMEOUT = 5

s = serial.Serial("/dev/ttyACM1", baudrate=4800)

current_byte = -1
print_len_last = 0
frame = ""

while True:
    byte = ord(s.read())
    syncbit = (byte&1<<7)>>7
    # print syncbit
    if syncbit == 1:
        os.system('clear')
        print frame
        frame = ""
        current_byte = 0
    frame += "{:08b}".format(byte) + '\n'

    if current_byte == 0:
        # signal strength
        signalStrength = byte&7
        frame += "signal strength: %.0f\n" % signalStrength
        # signal
        signalOK = (byte&1<<4)>>4
        if signalOK == 0:
            frame += "signal OK\n"
        else:
            frame += "signal lost\n"
        # probe
        probeUnplugged = (byte&1<<5)>>5
        if probeUnplugged == 0:
            frame += "probe connected\n"
        else:
            frame += "probe unplugged\n"
        # pulse beep
        pulsebeep = (byte&1<<6)>>6
        if pulsebeep == 0:
            frame += "^\n"
        else:
            frame += "-\n"
        frame += '-'* 80 + '\n'
        current_byte += 1
    elif current_byte == 1:
        pleth = (byte&127) # first 7 bits
        frame += "pleth: %.2f\n" % pleth
        frame += '-'* 80 + '\n'
        current_byte += 1
    elif current_byte == 2:
        # bargraph
        bargraph = byte&7
        frame += "bargraph: %.0f\n" % bargraph
        # sensor
        sensorOff = (byte&1<<4)>>4
        if sensorOff == 0:
            frame += "sensor OK\n"
        else:
            frame += "sensor off\n"
        # puls research
        pulseResearch = (byte&1<<5)>>5
        if pulseResearch == 0:
            frame += "puls OK\n"
        else:
            frame += "puls research\n"
        # heart rate bit 7
        heart_rate_bit7 = (byte&1<<6)>>6
        frame += '-'* 80 + '\n'
        current_byte += 1
    elif current_byte == 3:
        # heart rate
        heart_rate = (byte&127) # first 7 bits
        if heart_rate == 127:
            frame += "rate: ---.--\n"
        else:
            frame += "rate: %.2f\n" % heart_rate
        frame += '-'* 80 + '\n'
        current_byte += 1
    elif current_byte == 4:
        # oxygen saturation
        SpO2_pct = (byte&127) # first 7 bits
        if SpO2_pct == 127:
            frame += "spO2: --.--\n"
        else:
            frame += "spO2: %.2f\n" % SpO2_pct
        frame += '-'* 80 + '\n'
        current_byte +=1
    elif current_byte == 5:
        # not used
        frame += '-'* 80 + '\n'
        current_byte +=1
    elif current_byte == 6:
        # not used
        frame += '-'* 80 + '\n'
        current_byte = -1
    else:
        frame += "out of sync\n"
