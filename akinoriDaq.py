import os
import sys
from datetime import datetime
import time
import IV_Curve as vc
import daq6000 as gen
import daq6000a as daq

def safeExit(vs, jumpTarget, msg): # meant for crash handling
    vc.rampVoltage(vs, jumpTarget)
    vc.jumpVoltage(vs, 0)
    vs.instrument.write("*RST")
    raise RuntimeError(msg)

def startFnGen(serial, voltage):
    status = gen.initFunctionGenerator(serial)
    if status == 0:
        print("Python - Device not opened")
        safeExit()
    gen.runFunctionGenerator(voltage,38)
    return

def closeFnGen():
    gen.clearFunctionGenerator()
    return

def main(label, reset, extra=''):

    ############################################################################
    ###
    ###        AKINORI TO SET
    ###
    ############################################################################

    directory = "./"
    picoscope = 'IW098/0028'#'IW114/0004' # Not sure which is being used
    biasVoltage = 81
    ledVoltage = 560 # might need to be higher
    mppcVRange = 5 # 500mV
    numSamples = 1000
    numWaveforms = 1000

    ############################################################################
    ###
    ###        END
    ###
    ############################################################################

    start = time.time()

    date = datetime.today().strftime('%Y-%m-%d')

    bias = str(biasVoltage) + "V"
    led = str(ledVoltage) + "mV"

    outFile = directory + "/" + ("%s_%s_%s_%s_%s" % (date, label, bias, led, picoscope))

    fnGen = "GO024/040"

    targetVoltage   = biasVoltage
    normIncrement   = 2
    threshVoltage   = 76
    threshIncrement = 0.5

    jumpTarget = 70

    vs = vc.voltageSettings(reset, threshVoltage, normIncrement, 
                            threshIncrement) # MUST BE /dev/ttyUSB0

    if not reset:
        vc.rampDown(vs, jumpTarget, 0)
        vc.jumpVoltage(vs, 0)
        vs.instrument.write("*RST")

    # initPicoScopes(picoscopes, fnGen)
 
    vc.runSetup(vs, targetVoltage, "2.5e-4")

    vc.jumpVoltage(vs, jumpTarget)

    vc.rampVoltage(vs, targetVoltage)

    try:
        vc.rampVoltage(vs, bias)
        startFnGen(fnGen, ledVoltage)
        daq.runFullDaq(outFile,
                       0, mppcVRange, numSamples, 
                       0, mppcVRange, numSamples, 
                       0, mppcVRange, numSamples, 
                       0, mppcVRange, numSamples, 
                       100, 2, numWaveforms, 0, picoscope)
            
    except KeyboardInterrupt:
        safeExit(vs, jumpTarget, "Data collection loop was interrupted by user")
    except Exception as e:
        safeExit(vs, 0, "Data collection failed due to exception:\n" + str(e))

    closeFnGen()
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
    
    print("Total elapsed time:", int(time.time() - start), "s")
    
if __name__ == '__main__':
    args = sys.argv

    if len(args) != 2:
        print("wrong number of arguments")
        exit()
    
    main(args[1], True, "")
    

