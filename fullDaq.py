import sys
from datetime import datetime
import IV_Curve as vc
import daq6000 as gen
import daq6000a as daq

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

def daqPerBias(bias, mppcList, mv50List, mv200List, date, pmt='1.2', extra=''):

    s = r'%s'

    if extra != '':
        extra = '_' + extra
    
    mppcStr = "-".join(mppcList)

    outFilePattern = r"./data/%s_%sV_%s_%skV_%s%s" % \
                        (date, str(bias), s, pmt, mppcStr, extra)
    
    runDark(outFilePattern)

    runMvList(2, outFilePattern, mv50List)

    runMvList(4, outFilePattern, mv200List)

    return

def runDark(oFilePattern):
    daq.multiSeriesSetDaqSettings(
                    0, 1, 1000,
                    0, 1, 1000,
                    0, 1, 1000,
                    0, 1, 1000,
                    100, 2, 20000, 0)
    

    gen.runFunctionGenerator(0,38)
    out = oFilePattern % "Dark"
    print("\n\n\nNext DAQ: %s" % out)
    daq.multiSeriesCollectData(out)
    return

def runMvList(vRange, oFilePatternRaw, mvList):

    daq.multiSeriesSetDaqSettings(
                    0, vRange, 400,
                    0, vRange, 400,
                    0, vRange, 400,
                    0, vRange, 400,
                    100, 2, 20000, 0)

    oFilePattern = oFilePatternRaw % ("%imV")
    for mv in mvList:
        out = oFilePattern % mv
        gen.runFunctionGenerator(mv,38)
        print("\n\n\nNext DAQ: %s" % out)
        daq.multiSeriesCollectData(out)

    return

def main(mppcList, pmt):

    date = datetime.today().strftime('%Y-%m-%d')

    picoscopes = ['IW098/0028','IW114/0004']
    fnGen = "GO024/040"

    resetFlag = True

    targetVoltage   = 83
    normIncrement   = 2
    threshVoltage   = 76
    threshIncrement = 0.5

    jumpTarget = 70

    biasVoltageList = [82.5, 82, 81.5, 81, 80.5, 80, 79.5, 79, 78.5, 78]

    # bias voltage is key, first list if 50mV range, second list is 200mV
    ledVoltageMap = {83  : [[805, 810, 820],[840,870,890]],
                     82.5: [[805, 810, 820],[840,870,890]],
                     82  : [[805, 810, 820],[840,870,890]],
                     81.5: [[805, 810, 820],[840,870,890]],
                     81  : [[805, 810, 820],[840,870,890]],
                     80.5: [[805, 810, 820],[840,870,890]],
                     80  : [[805, 810, 820],[840,870,890]],
                     79.5: [[805, 810, 820],[840,870,890]],
                     79  : [[805, 810, 820],[840,870,890]],
                     78.5: [[805, 810, 820],[840,870,890]],
                     78  : [[805, 810, 820],[840,870,890]],
    }

    vs = vc.voltageSettings(resetFlag, threshVoltage, normIncrement, 
                            threshIncrement)

    vs.instrument.write("*RST") # reset now that Voltage is definitely 0
 
    vc.runSetup(vs, targetVoltage, "2.5e-4")

    vc.jumpVoltage(vs, jumpTarget)

    vc.rampVoltage(vs, targetVoltage)

    initPicoScopes(picoscopes, fnGen)

    input("Ramp PMT then press enter to begin...")

    try:
        for bias in biasVoltageList:
            vc.rampVoltage(vs, bias)
            mvLists = ledVoltageMap[bias]
            daqPerBias(bias, mppcList, mvLists[0], mvLists[1], date, pmt=pmt)
    except:
        vc.rampVoltage(vs, 0)

    try:
        vc.rampVoltage(vs, jumpTarget, 0)
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

if __name__ == '__main__':
    args = sys.argv
    if len(args) != 5:
        print("python3 fullDaq.py n1 n2 n3 n4")
        print("Add each mppc number in separate argument!")
    main(args[1:4], args[4])