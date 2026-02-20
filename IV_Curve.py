# -*- coding: utf-8 -*-
"""
Created on Thu Jul 18 16:10:02 2024

@author: amiles92
"""

import sys
import os
import platform
import argparse
import pyvisa as visa
import time
import numpy as np
import matplotlib.pyplot as plt

from enum import Enum
from functools import total_ordering

import traceback

plt.ion()

################################################################################
#####
#####       Log and error utilities
#####
################################################################################

@total_ordering
class logLevels(Enum):
    DEBUG = 0
    INFO = 1
    WARNING = 2
    ERROR = 3
    # FATAL = 4 # Can't be disabled
    def __lt__(self, other):
        if self.__class__ is other.__class__:
            return self.value < other.value
        return NotImplemented

g_log = logLevels.INFO

g_hardVLimit = 88

np.seterr(divide='ignore')

def debug(*args):
    if g_log <= logLevels.DEBUG:
        print("DEBUG   -", *args)

def info(*args):
    if g_log <= logLevels.INFO:
        print("INFO    -", *args)

def warning(*args):
    if g_log <= logLevels.WARNING:
        print("WARNING -", *args)

def error(*args):
    if g_log <= logLevels.ERROR:
        print("ERROR   -", *args)

def fatal(*args):
    print("\nFATAL   -", *args, "\nExiting immediately\n")
    errorAndExit()

def errorAndExit():
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
    raise RuntimeError

def safeExit(vs, *args):
    error(*args,"\nAttempting to ramp down safely without jumps")
    # vs.instrument.write("IFC")
    try:
        rampVoltage(vs, 0, 0)
        vs.instrument.write("*RST") 
    except:
        fatal("Error occurred while attempting safe exit")
    info("Safe exit successful, exiting now")
    raise RuntimeError

################################################################################
#####
#####       Actual voltage control
#####
################################################################################

class voltageSettings:
    voltageRange = 0
    currentVoltage = None
    def __init__(self, resetFlag, threshVoltage, normIncrement, 
                 threshIncrement, numMeasurements=0, trigDelay=0):
        if numMeasurements == 0:
            self.ivCheck = False
            self.numMeasurements = 0
            self.trigDelay = 0
        else:
            self.ivCheck = True
            self.numMeasurements = numMeasurements
            self.trigDelay = trigDelay

        self.instrument = initialisePowerSupply(resetFlag, numMeasurements, trigDelay)
        self.threshVoltage = threshVoltage
        self.normIncrement = normIncrement
        self.threshIncrement = threshIncrement

def grabArgs():

    usage = 'python3 RS232_IV_Curve.py [-h] <voltage-level> <voltage-increment> ' + \
        '<threshold-voltage> <threshold-increment> [-i <file-basename> ' + \
        '<num-measurements> <trigger-delay>] [-j <jump-voltage>] [--no-reset]'

    parser = argparse.ArgumentParser(prog="VoltageControl_RS232",
                                        description='Monash slowcontrol system for Keithley 6487 power supply',
                                        usage=usage,
                                        formatter_class=argparse.RawTextHelpFormatter)

    parser.add_argument('targetVoltage', type=float, metavar='voltage-level',
                    help='Voltage Level Required (volts .1f)')

    parser.add_argument('normIncrement', type=float, metavar='voltage-increment',
                        help='Voltage Increment Before Threshold Voltage (volts .1f)')

    parser.add_argument('threshVoltage', type=float, metavar='threshold-voltage',
                        help='Threshold Voltage (volts .1f)')

    parser.add_argument('threshIncrement', type=float, metavar='threshold-increment',
                        help='Voltage Increment After Threshold Voltage (volts .1f)')

    parser.add_argument('--no-reset', dest='reset', action='store_false',
                            help='Stop reset flag being sent to the hardware. Mostly to be used\n'
                                '  when the connection was previously severed unexpectedly')

    parser.add_argument('-i', dest='ivSettings', action="extend", nargs=3, type=str, 
                        metavar=('file-basename', 'num-measurements', 'trigger-delay'),
                        help='For taking the IV Curve, three arguments are required\n'
                            '  Base file name for saving data to\n'
                            '  Number of current measurements at each voltage (int)\n'
                            '  Trigger delay between measurements (seconds .1f)')

    parser.add_argument('-j', dest='jumpTarget', type=float, metavar='jump-voltage',
                        help='Set a SAFE voltage to jump to/from (volts .1f)')


    if len(sys.argv)==1:
        parser.print_help()
        parser.exit()

    args = vars(parser.parse_args())

    if (args['ivSettings'] != None):
        try:
            args['ivSettings'] = [args['ivSettings'][0], int(args['ivSettings'][1]), float(args['ivSettings'][2])]
        except:
            parser.exit(message="Invalid value for IV Curve settings (str, int, float): %s\n" % ", ".join(args['ivSettings']))
    
    return args

def initialisePowerSupply(resetFlag, numMeasurements, trigDelay):
    ############################################Initialisation Commands##############################################
    rm = visa.ResourceManager()
    listResources = rm.list_resources()
    debug(listResources)
    #Create a connection with the RS232 port listed when printing list_resources
    time.sleep(0.5)

    systemName = platform.system()
    if systemName == "Windows":
        if len(listResources) == 1:
            instrument=rm.open_resource(listResources[0])
        else:
            instrument=rm.open_resource(u'ASRL3::INSTR')
    elif systemName == "Linux":
        instrument=rm.open_resource(u'ASRL/dev/ttyUSB0::INSTR')

    instrument.baud_rate=9600
    instrument.write_termination = '\n'
    instrument.read_termination = '\n'
    if numMeasurements > 0:
        instrument.timeout=np.max([1000*(numMeasurements+trigDelay),25000]) # timeout is in ms

    #Setup command character terminations - basically the settings so that the PC and instrument
    #recognise the end of a command - this is instrument dependent unfortunately and not always
    #clear as to what it should be set to.

    #Send a query command for instrument identification to test both read and write commands are
    #functioning at both ends

    #Display settings
    info("%%%%%%%%%RS232-USB Configuration Settings%%%%%%%%%%%")
    debug("IP Address: 192.168.10.200")
    debug("Connection Port: 1234")
    info("Write Termination Character: \\n")
    info("Read Termination Character: \\n")
    info("Instrument Address: 22")

    if(resetFlag==1):
        info("RESET:", instrument.write("*RST"))

    instrument.write("TRAC:CLE") # clear buffer just in case
    instrument.write("*CLS")

    instrument.write("*IDN?")
    info(instrument.read())

    return instrument

def checkFinished(vs):
    try:
        vs.instrument.write("SYST:ERR?")
        errCheck = str(vs.instrument.read())
    except:
        warning("SYST:ERR? command failed")
        errCheck = ""
    try:
        vs.instrument.write("*OPC?")
        opcCheck = str(vs.instrument.read())
    except:
        warning("*OPC? command failed")
        opcCheck = ""

    debug("errCheck result:", errCheck)
    debug("opcCheck result:", opcCheck)

    vs.instrument.write("*CLS")

    if errCheck != "0,\"No error\"" or opcCheck != "1":
        if errCheck != "0,\"No error\"": error("SYST:ERR? returned", errCheck)
        if opcCheck != "1": warning("OPC returned incomplete. Result:", opcCheck, "\n\n")
        traceback.print_stack()
        print()
        return (errCheck, opcCheck)
    return None

def setVoltageRange(vs, targetVoltage):
    targetVoltageMag = abs(targetVoltage)
    if(targetVoltageMag<10.0):
        vs.instrument.write("SOUR:VOLT:RANG 10") #Set voltage range to 10 V range
        info("Voltage Range: 10 V")
        vs.voltageRange = 10
    elif(targetVoltageMag<50.0):
        vs.instrument.write("SOUR:VOLT:RANG 50") #Set voltage range to 50 V range
        info("Voltage Range: 50 V")
        vs.voltageRange = 50
    elif(targetVoltageMag<500.0):
        vs.instrument.write("SOUR:VOLT:RANG 500") #Set voltage range to 50 V range
        info("Voltage Range: 500 V")
        vs.voltageRange = 500
    else:
        safeExit(vs, "setVoltageRange received invalid target voltage:", str(targetVoltage))
        return 0
    return 1

def setVoltage(vs, newVoltage):
    if abs(newVoltage) > g_hardVLimit:
        fatal("setVoltage received voltage above hard limit of", g_hardVLimit, "V:", newVoltage, "\nAborting to prevent damage to electronics")
    command = "SOUR:VOLT " + str(round(newVoltage,2))
    info("Voltage: %.2f V " % newVoltage)
    vs.instrument.write(command)
    if checkFinished(vs) is not None:
        warning("setVoltage unsuccessful command:", command)
    return

def getVoltage(vs):
    debug("beginning getVoltage")
    # vs.instrument.write("INIT") # this does nothing
    vs.instrument.write("FORM:ELEM READ,TIME,VSO")
    vs.instrument.write("TRAC:CLE")
    vs.instrument.write("SYST:AZER OFF") # disable autozero to speed up measurements
    vs.instrument.write("READ?")

    rawResult = str(vs.instrument.read())
    result = float(rawResult.replace('A','').split(',')[2])
    vs.instrument.write("SYST:AZER ON")

    vs.instrument.write("TRAC:CLE") # Clear buffer


    debug("getVoltage raw res:", rawResult, "processed res:", result)

    if result < -600:
        error("getVoltage result =", result)
        try:
            setVoltage(vs, vs.currentVoltage)
        except:
            fatal("setVoltage failed in error handling")

        vs.instrument.write("READ?")
        rawResult = str(vs.instrument.read())
        result = float(rawResult.replace('A','').split(',')[2])
        if result < -600:
            fatal("Unable to return to last voltage step")

        safeExit(vs, "getVoltage returned to last voltage step but will exit for safety")
    
    if checkFinished(vs) is not None: warning("getVoltage unsuccessful")
    debug("finished getVoltage")

    vs.currentVoltage = result # update current voltage to be used in error exiting

    return result

def takeMeasurement(vs):
    start = time.time()
    if vs.trigDelay > 0:
        time.sleep(vs.trigDelay)
    debug("takeMeasurement: delay done")
    t0 = time.time()
    # vs.instrument.write("TRAC:POIN " + str(vs.numMeasurements)) #buffer expects 'n=measPoints' measurements

    vs.instrument.write("SYST:AZER OFF") # disable autozero to speed up measurements
    vs.instrument.write("READ?")

    rawResult = str(vs.instrument.read())
    vs.instrument.write("SYST:AZER ON")

    tDone = time.time()
    results = (rawResult.replace("A","")).split(',') #Read and format results
    RArray = (np.array(results)).astype(np.float64) #Convert results to float array

    debug("takeMeasurement raw res:", rawResult)
    debug("takeMeasurement took {:.2f} seconds".format(tDone - t0))

    vs.instrument.write("TRAC:CLE") # Clear buffer
    checkFinished(vs)

    diff = time.time() - start
    if diff < 2: # Makes sure we take at least two seconds between V jumps, to save MPPCs
        time.sleep(2 - diff)

    CArray = RArray[0::3]#Extract current measurements
    TArray = RArray[1::3]#Extract buffer timestamps
    VArray = RArray[2::3]#Read voltage source data (not needed so might take this out to improve read speeds)

    cMean = np.mean(CArray) ; cStd = np.std(CArray)
    tMean = np.mean(TArray) ; tStd = np.std(TArray)
    vMean = np.mean(VArray) ; vStd = np.std(VArray)
    rMean = vMean / cMean   ; rStd = rMean * np.sqrt((vStd/vMean)**2 + (cStd/cMean)**2)

    info("Mean Voltage: {:.2e} +- {:.2e}".format(vMean, vStd))
    info("Mean Current: {:.2e} +- {:.2e}".format(cMean, cStd))
    info("Mean Resistance: {:.2e} +- {:.2e}\n".format(rMean, rStd))

    # plt.figure() ; plt.scatter(TArray, CArray) ; plt.xlabel("Time (s)") ; plt.ylabel("Current (A)") ; plt.title("Current vs Time") ; plt.show()

    vs.currentVoltage = VArray[-1]
    
    return CArray, TArray, VArray

def runSetup(vs, targetVoltage, currentLimit, currentRange="2e-2"):
    vs.instrument.write("*CLS")

    if vs.ivCheck:
        runZeroCheck(vs)

    vs.instrument.write("RANG " + currentRange)
    vs.instrument.write("FORM:ELEM READ,TIME,VSO")#Read current, timestamp, voltage
    vs.instrument.write("TRIG:DEL 0.0") #Trigger delay of 0;;; IT'S NOT WORKING!!
    # vs.instrument.write("TRIG:COUN " + str(vs.numMeasurements)) #Trigger count of 25 # REDUNDANT
    vs.instrument.write("NPLC .1") # Set integration time to 0.01 power line cycles (PLC) for faster measurements (default is 1 PLC, which is 0.02 seconds at 50 Hz)
    # vs.instrument.write("AVER:COUN 10") #100 point averaged filter
    vs.instrument.write("AVER:TCON REP") #Repeating filter
    # vs.instrument.write("AVER ON") #Turn filter on
    

    vs.instrument.write("STAT:MEAS:ENAB 512")
    vs.instrument.write("*SRE 1")
    checkFinished(vs)

    
    # --------------------------------------------------------------------------
    setVoltageRange(vs, targetVoltage)

    vs.instrument.write("SOUR:VOLT:ILIM " + currentLimit)
    vs.instrument.write("SOUR:VOLT:STAT ON") #Turn voltage source on
    return
    # --------------------------------------------------------------------------

def rampDown(vs, targetVoltage, measure):
    if vs.currentVoltage is not None:
        debug("rampDown: using existing voltage")
        voltageRead = vs.currentVoltage  
    else:
        debug("rampDown: getting new voltage")
        voltageRead = getVoltage(vs)

    data = [[], [], []]

    if measure:
        measurement = takeMeasurement(vs)
        for i in range(len(measurement)):
            data[i].append(measurement[i])

    while (voltageRead > targetVoltage):
        newVoltage = voltageRead
        if (voltageRead > vs.threshVoltage):
            newVoltage = voltageRead - vs.threshIncrement
        else:
            newVoltage = voltageRead - vs.normIncrement
        
        if newVoltage < targetVoltage:
            newVoltage = targetVoltage

        setVoltage(vs, newVoltage)

        if measure:
            if g_log == logLevels.INFO:
                print("\n")
            measurement = takeMeasurement(vs)
            for i in range(len(measurement)):
                data[i].append(measurement[i])
            voltageRead = vs.currentVoltage
        else:
            time.sleep(2)
            voltageRead = getVoltage(vs)

    if measure:
        return data

def rampUp(vs, targetVoltage, measure):
    if vs.currentVoltage is not None:
        debug("rampUp: using existing voltage")
        voltageRead = vs.currentVoltage  
    else:
        debug("rampUp: getting new voltage")
        voltageRead = getVoltage(vs)

    data = [[], [], []]

    if measure:
        measurement = takeMeasurement(vs)
        for i in range(len(measurement)):
            data[i].append(measurement[i])

    while (voltageRead < targetVoltage):
        newVoltage = voltageRead
        if (voltageRead < vs.threshVoltage):
            newVoltage = voltageRead + vs.normIncrement 
        else:
            newVoltage = voltageRead + vs.threshIncrement
        
        if newVoltage > targetVoltage:
            newVoltage = targetVoltage

        setVoltage(vs, newVoltage)

        if measure:
            if g_log == logLevels.INFO:
                print("\n")
            measurement = takeMeasurement(vs)
            for i in range(len(measurement)):
                data[i].append(measurement[i])
                voltageRead = vs.currentVoltage
        else:
            time.sleep(2)
            voltageRead = getVoltage(vs)

    if measure:
        return data

def rampVoltage(vs, targetVoltage, measure=0):
    info("Ramping to {:.2f} V".format(targetVoltage))

    if measure == 1:
        vs.instrument.write("TRIG:COUN " + str(vs.numMeasurements))
        vs.instrument.write("TRAC:POIN " + str(vs.numMeasurements))
        vs.instrument.write("TRAC:CLE")
        vs.instrument.write("AVER:COUN 10")
        vs.instrument.write("AVER ON")
        checkFinished(vs)
        voltageRead = getVoltage(vs)

    else:
        vs.instrument.write("TRIG:COUN 1")
        vs.instrument.write("TRAC:POIN 1")
        vs.instrument.write("TRAC:CLE")
        vs.instrument.write("AVER:COUN 10")
        vs.instrument.write("AVER OFF")
        checkFinished(vs)
        voltageRead = getVoltage(vs)

    res = None

    if (voltageRead < targetVoltage):
        res = rampUp(vs, targetVoltage, measure)

    if (voltageRead > targetVoltage):
        res = rampDown(vs, targetVoltage, measure)
    
    if g_log == logLevels.INFO and measure == 0:
        print("\n")

    return res

def jumpVoltage(vs, targetVoltage):
    ############################################################################
    ##
    ##  THIS FUNCTION IS DANGEROUS AND SAFETY OF ELECTRONICS SHOULD BE CONFIRMED
    ##  BEFORE USING WITH SENSITIVE ELECTRONICS
    ##
    ############################################################################

    if targetVoltage > vs.voltageRange:
        setVoltageRange(vs, targetVoltage)

    setVoltage(vs, targetVoltage)

    time.sleep(5)
    return

def plotData(data):
    plt.figure()

    c, t, v = data

    debug("c, t, v shapes:", np.shape(c), np.shape(t), np.shape(v))

    plt.errorbar(np.mean(v,axis=1), np.mean(c,axis=1), yerr = np.std(c,axis=1), fmt='o')
    # plt.errorbar(np.mean(v,axis=2), np.mean(c,axis=2), yerr = np.std(c,axis=2))

    # plt.show(block=False)
    plt.pause(1.0)

    return

def saveToFile(data, fileName, directory):

    datanp = np.array(data,dtype=np.float64)
    np.save(directory + fileName + ".npy", datanp)

    shapeStr = ",".join(map(str, datanp.shape))
    dataForCsv = np.reshape(datanp, (3, -1)).T
    np.savetxt("test.csv", dataForCsv, delimiter=",", comments="",
        header="%s\nCurrent (A), Time (s), Voltage (V)" % shapeStr)
    return

def main(args, currentLimitStr, currentRangeStr):

    resetFlag = args['reset']

    targetVoltage   = args['targetVoltage'] # How high voltage is to be set (V)
    normIncrement   = args['normIncrement'] # Voltage increment below the threshold
    threshVoltage   = args['threshVoltage'] # Voltage level after which it will increment in ThreshIncrement volts
    threshIncrement = args['threshIncrement'] # Voltage increment above the threshold

    jump = False
    if args['jumpTarget'] != None:
        jump = True
        jumpTarget = args['jumpTarget']

    if args['ivSettings'] != None:

        directory =  "./data/IV_Curves/"
        os.makedirs(directory, exist_ok=True)
        fileName, numMeasurements, trigDelay = args['ivSettings']

        vs = voltageSettings(resetFlag, threshVoltage, normIncrement, 
                            threshIncrement, numMeasurements, trigDelay)
    
    else:
        vs = voltageSettings(resetFlag, threshVoltage, normIncrement, 
                            threshIncrement)

    if not resetFlag: # This is going to be pointless if reset is run
        rampVoltage(vs, 0, 0)
        vs.instrument.write("*RST") # reset now that Voltage is definitely 0

    # if vs.ivCheck:
    #     runSetup(vs, targetVoltage, currentLimitStr)
    # else:   
    #     runSetup(vs, targetVoltage, currentLimitStr, currentRangeStr)
    runSetup(vs, targetVoltage, currentLimitStr, currentRangeStr)

    if jump:
        jumpVoltage(vs, jumpTarget)

    if vs.ivCheck:
        data = rampVoltage(vs, targetVoltage, 1)
        saveToFile(data, fileName, directory)
        try:
            plotData(data)
        except:
            warning("Plotting failed, continuing anyways")
        inputVoltage = 0
    else:
        rampVoltage(vs, targetVoltage)
        inputVoltage = float(input("Enter next voltage:\n"))

    while inputVoltage != 0:
        try:
            if abs(inputVoltage) > 500:
                warning("Voltage entered out of range, must be between -500 and 500 V")
            else:
                targetVoltage = inputVoltage
                if abs(targetVoltage) > vs.voltageRange:
                    setVoltageRange(vs, targetVoltage)
                rampVoltage(vs, inputVoltage, 0)

            inputVoltage = float(input("Enter next voltage:\n"))
            
        except:
            safeExit(vs, "Something happened in inputVoltage loop")
    try:
        if jump:
            rampVoltage(vs, jumpTarget, 0)
            jumpVoltage(vs, 0)
        else:
            rampVoltage(vs, 0, 0)

        vs.instrument.write("*RST") # Reset all parameters now that safely ramped down
    except:
        fatal("Unexpected error occurred while ramping to 0!!")
    return

def runZeroCheck(vs):

    debug("Running zero check")
    vs.instrument.write("FUNC 'CURR'") #Set measurement function to current
    vs.instrument.write("CURR:RANG 2e-9")
    vs.instrument.write("SYST:ZCH ON")
    vs.instrument.write("INIT")
    vs.instrument.write("SYST:ZCOR:STAT OFF")
    vs.instrument.write("SYST:ZCOR:ACQ")
    vs.instrument.write("SYST:ZCOR ON")
    vs.instrument.write("SYST:ZCH OFF")



    """ - Manual -
    vs.instrument.write("SYST:ZCH ON")
    vs.instrument.write("INIT")
    vs.instrument.write("SYST:ZCOR:STAT OFF")
    vs.instrument.write("SYST:ZCOR:ACQ")
    vs.instrument.write("SYST:ZCOR ON")
    vs.instrument.write("CURR:RANG:AUTO ON")
    vs.instrument.write("SYST:ZCH OFF")
    vs.instrument.write("READ?")
    """

    """ - Take measurement -
    vs.instrument.write("TRIG:COUN " + str(vs.numMeasurements))
    vs.instrument.write("TRAC:POIN " + str(vs.numMeasurements))
    vs.instrument.write("TRAC:CLE")
    vs.instrument.write("AVER:COUN 10")
    vs.instrument.write("AVER ON")
    """

    return

    
if __name__ == "__main__":

    g_log = logLevels.INFO

    ############################################################################
    # Fixed Parameters

    # currentRangeStr = "2e-3" # 2 mA # Applicable to combined MPPCs
    # currentLimitStr = "2.5e-4"

    currentRangeStr = "2e-6" # 2 mA # Applicable to 'single' MPPC channel (4/16 mixed for COMET)
    currentLimitStr = "2.5e-6"
    
    # currentRangeStr = "2e-2" 
    # currentLimitStr = "2e-2" #testing with 50Ohm terminator for 5V
    ############################################################################

    args = grabArgs()

    t0 = time.time()

    main(args, currentLimitStr, currentRangeStr)

    info("Script finished in {:.2f} seconds".format(time.time() - t0))
