import os
import sys
from datetime import datetime
import time
import IV_Curve as vc
import sanityCheck as sc
import daq6000 as gen
import daq6000a as daq
import isegNhqControl as nhq

### GLOBAL FLAGS ###
g_quickPlots = True
####################

def endNotification():
    if not music: return
    try:
        from pygame import mixer
        import glob
        import random

        options = glob.glob("./msc/*.mp3")
        choice = random.choice(options)

        mixer.init()
        mixer.music.load(choice)
        mixer.music.play()
        input("Press ENTER to stop the music")
        mixer.music.stop()
    except:
        return
    return

def initPicoScopes(picoList, fnGen):
    for ps in picoList:
        status = daq.multiSeriesInitDaq(ps)
        if status == 0:
            print("Python - Device not opened")
            exit()

    status = gen.initFunctionGenerator(fnGen)
    if status == 0:
        print("Python - Device not opened")
        exit()

def closePicoscopes():
    daq.multiSeriesCloseDaq()
    gen.clearFunctionGenerator()
    return

def daqPerBias(bias, mppcStr, mvLists, date, pmt, d, extra):
    """Runs DAQ for a range of LED Voltages for a given bias voltage"""
    s = r'%s'
    outFilePattern = d + r"%s_%sV_%s_%skV_%s%s" % \
                        (date, str(bias), s, pmt, mppcStr, extra)
    
    runDark(outFilePattern)

    if len(mvLists) > 0:
        runMvList(2, outFilePattern, mvLists[0], 2)

    if len(mvLists) > 1:
        runMvList(4, outFilePattern, mvLists[1], 4)
    
    if len(mvLists) > 2:
        runMvList(6, outFilePattern, mvLists[2], 6)

    return

def runDark(oFilePattern):
    """Runs DAQ for dark photos: no LED, very low voltage range"""
    daq.multiSeriesSetDaqSettings(
                    0, 1, 2000,
                    0, 1, 2000,
                    0, 1, 2000,
                    0, 1, 2000,
                    100, 2, 5000, 0)
    

    gen.runFunctionGenerator(0,38)
    out = oFilePattern % "Dark"
    print("\n\n\nNext DAQ: %s" % out)
    daq.multiSeriesCollectData(out)
    if g_quickPlots: sc.quickPlot(out + "_%s.dat")
    return

def runMvList(vRange, oFilePatternRaw, mvList, pmtVRange=2):
    """Runs DAQ for range of LED voltages for a given bias and voltage range"""
    daq.multiSeriesSetDaqSettings(
                    0, vRange, 400,
                    0, vRange, 400,
                    0, vRange, 400,
                    0, pmtVRange, 400, # 50mV is good for all, may want to reduce it for mv50List
                    100, 2, 20000, 0)

    oFilePattern = oFilePatternRaw % ("%imV")
    for mv in mvList:
        out = oFilePattern % mv
        gen.runFunctionGenerator(mv,38)
        print("\n\n\nNext DAQ: %s" % out)
        daq.multiSeriesCollectData(out)
        if g_quickPlots: sc.quickPlot(out + "_%s.dat")

    return

def quickCheck(bias, mppcStr, ledV, date, pmt, d, extra, picoscopes):
    """
    Runs short DAQ with high LED and bias to check if the signals seem reasonable.

    Specific values are hard coded in sanityCheck.py. It will halt if the value
    is low, but can be continued or killed if it fails.
    """
    daq.multiSeriesSetDaqSettings(
                    0, 4, 400,
                    0, 4, 400,
                    0, 4, 400,
                    0, 2, 400,
                    100, 2, 1000, 0)

    out = d + r"Check_%s_%sV_%s_%skV_%s%s" % \
                (date, str(bias), ledV, pmt, mppcStr, extra)
    
    gen.runFunctionGenerator(ledV,38)
    print("\n\n\nNext DAQ: %s" % out)
    daq.multiSeriesCollectData(out)

    ex = False
    for ps in picoscopes:
        res = sc.sanityBool(out + "_%s.dat" % ps.replace("/","-"), output=True)
        if not res:
            print("\nERROR: Possible issues present in Picoscope %s" % ps)
            sc.quickPlot(out + "_%s.dat")
            ex = True
    
    if ex:
        cont = input("Continue anyways? [y]/n")
        if cont != '':
            if cont[0].lower() == 'n':
                return False

    return True

def safeExit(vs, jumpTarget, msg): # meant for crash handling
    vc.rampVoltage(vs, jumpTarget)
    vc.jumpVoltage(vs, 0)
    vs.instrument.write("*RST")
    raise RuntimeError(msg)

def main(mppcList, reset, extra=''):

    start = time.time()

    date = datetime.today().strftime('%Y-%m-%d')

    directory = r'/media/cdc/MPPC-QC/QC-data/test_2025Mar'
    mppcStr = "-".join(mppcList)
    if extra != '':
        extra = '_' + extra
    if mppcStr == "999-999-999": # passing 999 999 999 will safely ramp down and exit the daq
        reset = False
    else:
        path = r"%s/%s%s/" % (directory, mppcStr, extra)
        os.makedirs(path, exist_ok=True)

    picoscopes = ['IW098/0028','IW114/0004']
    picohyphen = [ps.replace('/','-') for ps in picoscopes]
    fnGen = "GO024/040"

    sc.g_picoscopes = picohyphen # so that quickPlot can find files properly
    pmt = "1.4"
    pmtVoltage = 1400
    pmtVoltageRampSpeed = 50 # V/s

    biasVoltageList = [83, 82.5, 82, 81.5, 81, 80.5, 80, 79.5, 79, 78.5, 78]

    mv50LongList = [540, 545]
    mv200LongList = [565, 585]
    mv1kLongList = [600, 610, 630]
    ledLongSweep = [mv50LongList, mv200LongList, mv1kLongList]

    mv50ShortList = [540]
    mv200ShortList = [585]
    mv1kShortList = [610]
    ledShortSweep = [mv50ShortList, mv200ShortList, mv1kShortList]

    quickCheckLedV = 630

    # bias voltage is key, first list if 50mV range, second list is 200mV
    ledVoltageMap = {83  : ledShortSweep, # [[840],[880, 900]],
                     82.5: ledShortSweep, # [[840],[880, 900]],
                     82  : ledShortSweep, # [[840],[880, 900]],
                     81.5: ledLongSweep, # [[820, 840, 850],[880, 890, 900]],
                     81  : ledLongSweep, # [[820, 840, 850],[880, 890, 900]],
                     80.5: ledLongSweep, # [[820, 840, 850],[880, 890, 900]],
                     80  : ledShortSweep, # [[840],[880, 900]],
                     79.5: ledShortSweep, # [[840],[880, 900]],
                     79  : ledShortSweep, # [[840],[880, 900]],
                     78.5: ledShortSweep, # [[840],[880, 900]],
                     78  : ledShortSweep, # [[840],[880, 900]],
    }

    targetVoltage   = biasVoltageList[0]
    normIncrement   = 2
    threshVoltage   = 76
    threshIncrement = 0.5

    jumpTarget = 70

    isegSer = nhq.initSerial(1) # MUST BE /dev/ttyUSB1
    if isegSer == None:
        print("Incorrect port configurations. Unplug and replug them in the correct order.")
        return
    
    nhq.isegSetVoltage(isegSer, pmtVoltage)
    nhq.isegSetVoltageRampSpeed(isegSer, pmtVoltageRampSpeed)
    status = nhq.isegStartVoltageRamp(isegSer)
    if status not in ["S1=L2H", "S1=H2L", "S1=ON"]:
        print("PMT HV Supply incorrectly configured. Response:", status)

    vs = vc.voltageSettings(reset, threshVoltage, normIncrement, 
                            threshIncrement) # MUST BE /dev/ttyUSB0

    if not reset:
        vc.rampDown(vs, jumpTarget, 0)
        vc.jumpVoltage(vs, 0)
        vs.instrument.write("*RST")

    if mppcStr == "999-999-999":
        return

    initPicoScopes(picoscopes, fnGen)
 
    vc.runSetup(vs, targetVoltage, "2.5e-4")

    vc.jumpVoltage(vs, jumpTarget)

    vc.rampVoltage(vs, targetVoltage)

    # confirm pmt ramped properly
    status = nhq.isegGetStatus(isegSer)
    while status in ["S1=L2H", "S1=H2L"]:
        time.sleep(1)
        status = nhq.isegGetStatus(isegSer)
    
    if status != "S1=ON":
        print("Possible problem with PMT supply, please manually ramp it down")
        print("PMT Supply response:", status)
        safeExit(vs, jumpTarget, "Error in PMT HV supply")

    res = quickCheck(targetVoltage, mppcStr, quickCheckLedV, date, pmt, path, extra, picoscopes)
    if not res:
        safeExit(vs, jumpTarget, "Quick check failed, early ramp down initiated")

    try:
        for bias in biasVoltageList:
            vc.rampVoltage(vs, bias)
            mvLists = ledVoltageMap[bias]
            daqPerBias(bias, mppcStr, mvLists, date, pmt, path, extra)
    except KeyboardInterrupt:
        safeExit(vs, jumpTarget, "Data collection loop was interrupted by user")
    except Exception as e:
        safeExit(vs, 0, "Data collection failed due to exception:\n" + str(e))

    nhq.isegSetVoltage(isegSer, 0)
    nhq.isegStartVoltageRamp(isegSer)
    if status not in ["S1=L2H", "S1=H2L", "S1=ON"]:
        print("Possible problem with PMT supply, please manually ramp it down")
        print("PMT Supply response:", status)

    try:
        vc.rampVoltage(vs, jumpTarget)
        vc.jumpVoltage(vs, 0)

        vs.instrument.write("*RST") # Reset all parameters now that safely ramped down
    except:
        print("\nUnexpected error occurred while ramping to 0!!")
        print("Please ramp down the device manually")
        print("    To do so, press the \"Config/Local\" button and then use the")
        print("    up and down arrow buttons in the \"V-SOURCE\" box, with gray")
        print("    body colour and a white triangular arrow.")
        print("")
        print("    Change which digit to increment with the left and right arrow")
        print("    buttons directly below them, with white body colour and gray")
        print("    arrows.")
        print("")
        print("    Once safely ramped down, press the \"OPER\" button to switch")
        print("    the voltage off.")
        print("")
        print("    DO NOT USE THE PURE WHITE \"RANGE\" BUTTONS!!!")
        exit()
    
    closePicoscopes()
    print("Total elapsed time:", int(time.time() - start), "s")
    endNotification()

if __name__ == '__main__':
    args = sys.argv

    reset = True
    if "--crashed" in args:
        reset = False
        args.remove("--crashed")
    
    music = True
    if "--silent" in args:
        music = False
        args.remove("--silent")

    if len(args) < 4 | len(args) > 5:
        print("python3 fullDaq.py n1 n2 n3 [label] [--crashed]")
        print("Add each mppc number in separate argument!")
        print("  n1 = Channel A and is furthest in enclosure")
        print("  n2 = Channel B and is middle in enclosure")
        print("  n3 = Channel C and is closest in enclosure")
        print("Additional label string is optional\n")
    if len(args) == 4:
        main(args[1:4], reset)
    else:
        main(args[1:4], reset, args[4])
