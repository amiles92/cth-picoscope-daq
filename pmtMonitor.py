import os
import sys
from datetime import datetime
import time
import sanityCheck as sc
import daq6000 as gen
import daq6000a as daq
import isegNhqControl as nhq

### GLOBAL FLAGS ###
g_quickPlots = True
####################

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

def safeExit(isegSer, msg): # meant for crash handling
    nhq.isegSetVoltage(isegSer, 0)
    nhq.isegStartVoltageRamp(isegSer)
    raise RuntimeError(msg)

def main(iterations):

    start = time.time()

    date = datetime.today().strftime('%Y-%m-%d')

    path = r'/media/cdc/MPPC-QC/pmtMonitor/' + date + '/'
    os.makedirs(path, exist_ok=True)

    picoscopes = ['IW098/0028','IW114/0004']
    picohyphen = [ps.replace('/','-') for ps in picoscopes]
    fnGen = "GO024/040"

    sc.g_picoscopes = picohyphen # so that quickPlot can find files properly
    pmtVoltage = 1400
    pmtVoltageRampSpeed = 50 # V/s

    ledVoltage = 630
    ledStr = str(ledVoltage) + "mV"

    isegSer = nhq.initSerial(1) # MUST BE /dev/ttyUSB1
    if isegSer == None:
        print("Incorrect port configurations. Unplug and replug them in the correct order.")
        return
    
    nhq.isegSetVoltage(isegSer, pmtVoltage)
    nhq.isegSetVoltageRampSpeed(isegSer, pmtVoltageRampSpeed)
    status = nhq.isegStartVoltageRamp(isegSer)
    if status not in ["S1=L2H", "S1=H2L", "S1=ON"]:
        print("PMT HV Supply incorrectly configured. Response:", status)

    initPicoScopes(picoscopes, fnGen)

    daq.multiSeriesSetDaqSettings(
                    0, 99, 0,
                    0, 99, 0,
                    0, 99, 0,
                    0, 4, 400,
                    100, 2, 20000, 0)

    oFilePattern = path + "_".join([date, ledStr, "1.4kV", "%i"])

    # confirm pmt ramped properly
    status = nhq.isegGetStatus(isegSer)
    while status in ["S1=L2H", "S1=H2L"]:
        time.sleep(1)
        status = nhq.isegGetStatus(isegSer)
    
    if status != "S1=ON":
        print("Possible problem with PMT supply, please manually ramp it down")
        print("PMT Supply response:", status)
        safeExit(isegSer, "Error in PMT HV supply")

    try:
        for iter in range(int(iterations)):
            prev = time.time()
            daq.multiSeriesCollectData(oFilePattern % iter)
            sc.quickPlotPmt((oFilePattern % iter) + "_%s.dat")
            delay = 3600 + prev - time.time()
            time.sleep(delay)
    except KeyboardInterrupt:
        safeExit(isegSer, "Data collection loop was interrupted by user")
    except Exception as e:
        safeExit(isegSer, "Data collection failed due to exception:\n" + str(e))

    nhq.isegSetVoltage(isegSer, 0)
    nhq.isegStartVoltageRamp(isegSer)
    if status not in ["S1=L2H", "S1=H2L", "S1=ON"]:
        print("Possible problem with PMT supply, please manually ramp it down")
        print("PMT Supply response:", status)
    
    closePicoscopes()
    print("Total elapsed time:", int(time.time() - start), "s")
    print("Total sets of data taken:", iter)

if __name__ == '__main__':
    args = sys.argv

    if len(args) != 2:
        print("python3 pmtMonitor.py H")
        print("  Records PMT response once an hour for H hours")
        exit()
    main(args[1])
