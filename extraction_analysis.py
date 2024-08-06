import math
from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.pyplot as plt
import numpy as np
import os
import ROOT
from scipy import stats, optimize
from scipy.stats import norm
from scipy.signal import find_peaks
import sys

###############################################################################
###                               Parameters                                ###
###############################################################################

### Data extraction
ps6000VRanges = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 
                 10000, 20000, 50000]
                 
### pre-analysis  
windowlow, windowhigh = 10, 50        
nplots = 100
timeBase = 0.8
numSigmaBaseline = -10
inputDirectory = "../DAQ_C/data/"

### analysis
nbins = 200
p0_chargeDist = [5, 0, 0.1, -2.7, 0.1] ## likely fit parameters
p0bounds_chargeDist =([0, -4, 0, -10, 0], [15, 0.5, 2, 0, 2]) ## fit parameters bounds
p0_chargeInte = [4, 20, 20, -80, 5] ## likely fit parameters
p0bounds_chargeInte =([0, -50, 0.01, -100, 0.001], [30, 50, 50, -50, 5]) ## fit parameters bounds
p0_chargeInteHighPE = [-1000, 100] ## likely fit parameters
p0bounds_chargeInteHighPE =([-10000, 0.01], [0, 5000]) ## fit parameters bounds
i1, i2 = int(nbins/2), nbins-1-int(nbins/8) ## fitting range, eg: int(nbins/3), nbins-1
kmax = 10 ## max of poisson sum

### global
voltagesMPPC = [80, 80.5, 81, 81.5, 82]
voltagesLED = [1.33, 1.34, 1.35, 1.40, 1.41, 1.42, 1.50]
beginRange = 0
endRange = 0  
pdfDirectory = "plots/"
npyDirectory = "npys/"
withMa = 1
showing = 1
saving = 0

### files
## Monash setup / alternate LED on
#inputFileList = ["BothLED_81V_3.25V", "RightLED_81V_3.25V", "LeftLED_81V_3.25V"]

## new setup / no LED on
#inputFileList = ["19Jun24_80V_Dark_TapedBox", "19Jun24_80.5V_Dark_TapedBox", 
#                 "19Jun24_81V_Dark_TapedBox", "19Jun24_81.5V_Dark_TapedBox", 
#                 "19Jun24_82V_Dark_TapedBox"]
## new setup / LED at 1.41V
#inputFileList = ["19Jun24_80V_1.41V_TapedBox", "19Jun24_81V_1.41V_TapedBox", 
#                 "19Jun24_81.5V_1.41V_TapedBox", "19Jun24_82V_1.41V_TapedBox"]
## new setup / no LED on / run 2
#inputFileList = ["19Jun24_81V_Dark_TapedBox_Round2"]
## new setup / no LED on / rotated MPPC
#inputFileList = ["19Jun24_81V_Dark_TapedBox_RotatedMPPC"]

## new setup / no LED on / other MPPC
#inputFileList = ["20Jun24_80V_Dark_TapedBox_MPPC3", "20Jun24_80.5V_Dark_TapedBox_MPPC3", 
#                  "20Jun24_81V_Dark_TapedBox_MPPC3"]
## new setup / no LED on / other MPPC / black sheet / with light
#inputFileList = ["20Jun24_80V_Dark_Sheet_MPPC3", "20Jun24_81V_Dark_Sheet_MPPC3"]
## new setup / no LED on / other MPPC / black sheet / without light
#inputFileList = ["20Jun24_80V_Dark_Sheet_MPPC3_LightsOff", "20Jun24_81V_Dark_Sheet_MPPC3_LightsOff"]
## new setup / LED at 1.41V / other MPPC / black sheet
#inputFileList = ["20Jun24_81V_1.41V_Sheet_MPPC3_LightsOn"]
## new setup / LED at 1.41V / other MPPC / black sheet / diffuser added
#inputFileList = ["20Jun24_81V_1.41V_Diffuser_Sheet_MPPC3_LightsOn"]
## new setup / LED at 1.44V / other MPPC / black sheet / diffuser added
#inputFileList = ["20Jun24_81V_1.44V_Diffuser_Sheet_MPPC3_LightsOn"]
## new setup / LED at 1.41V / other MPPC / black sheet / reverse MPPC
#inputFileList = ["20Jun24_80V_1.41V_Diffuser_Sheet_MPPC3_LightsOn_Rotated", 
#                 "20Jun24_80.5V_1.41V_Diffuser_Sheet_MPPC3_LightsOn_Rotated", 
#                 "20Jun24_81V_1.41V_Diffuser_Sheet_MPPC3_LightsOn_Rotated", 
#                 "20Jun24_80V_1.42V_Diffuser_Sheet_MPPC3_LightsOn_Rotated",                   
#                 "20Jun24_80.5V_1.42V_Diffuser_Sheet_MPPC3_LightsOn_Rotated", 
#                 "20Jun24_81V_1.42V_Diffuser_Sheet_MPPC3_LightsOn_Rotated", 
#                 "20Jun24_80V_1.44V_Diffuser_Sheet_MPPC3_LightsOn_Rotated", 
#                 "20Jun24_80.5V_1.44V_Diffuser_Sheet_MPPC3_LightsOn_Rotated",                   
#                 "20Jun24_81V_1.44V_Diffuser_Sheet_MPPC3_LightsOn_Rotated"]

## new MPPCs
#inputFileList = ["24Jun24_80V_Dark_MPPC10", "24Jun24_80V_1.35V_MPPC10", 
#                 "24Jun24_80V_1.36V_MPPC10"]
#inputFileList = ["24Jun24_80V_Dark_MPPC1", "24Jun24_80V_1.35V_MPPC1", 
#                 "24Jun24_80V_1.36V_MPPC1"]
#inputFileList = ["25Jun24_80V_Dark_MPPC5", "25Jun24_80V_1.35V_MPPC5", 
#                 "25Jun24_80V_1.36V_MPPC5", "25Jun24_80V_1.37V_MPPC5", 
#                 "25Jun24_80V_1.38V_MPPC5", "25Jun24_80V_1.39V_MPPC5", 
#                 "25Jun24_80V_1.40V_MPPC5"]
#inputFileList = ["27Jun24_80V_1.2kV_1.38V_3-6-8_IW098_0028_BetterVRanges", "27Jun24_80V_1.2kV_1.38V_3-6-8_IW098_0028"]
#inputFileList = ["28Jun24_80V_1.35V_1.2kV_3-6-8_IW098-0028_Noisy", "28Jun24_80V_1.35V_1.2kV_3-6-8_IW114-0004_Noisy"]
#inputFileList = ["28Jun24_80V_1.35V_1.2kV_3-6-8_IW098-0028", "28Jun24_80V_1.35V_1.2kV_3-6-8_IW114-0004"]
#inputFileList = ["01Jul24_80V_Dark_1.2kV_3-6-8_IW098-0028", "01Jul24_80V_Dark_1.2kV_3-6-8_IW114-0004", 
#                 "01Jul24_80V_1.33V_1.2kV_3-6-8_IW098-0028", "01Jul24_80V_1.33V_1.2kV_3-6-8_IW114-0004", 
#                 "01Jul24_80V_1.34V_1.2kV_3-6-8_IW098-0028", "01Jul24_80V_1.34V_1.2kV_3-6-8_IW114-0004", 
#                 "01Jul24_80V_1.35V_1.2kV_3-6-8_IW098-0028", "01Jul24_80V_1.35V_1.2kV_3-6-8_IW114-0004", 
#                 "01Jul24_80V_Dark_1.2kV_3-6-8_SmallVRange_IW098-0028", "01Jul24_80V_Dark_1.2kV_3-6-8_SmallVRange_IW114-0004", 
#                 "01Jul24_80V_1.33V_1.2kV_3-6-8_SmallVRange_IW098-0028", "01Jul24_80V_1.33V_1.2kV_3-6-8_SmallVRange_IW114-0004", 
#                 "01Jul24_80V_1.34V_1.2kV_3-6-8_SmallVRange_IW098-0028", "01Jul24_80V_1.34V_1.2kV_3-6-8_SmallVRange_IW114-0004", 
#                 "01Jul24_80V_1.35V_1.2kV_3-6-8_SmallVRange_IW098-0028", "01Jul24_80V_1.35V_1.2kV_3-6-8_SmallVRange_IW114-0004", 
#                 "01Jul24_81V_Dark_1.2kV_3-6-8_SmallVRange_IW098-0028", "01Jul24_81V_Dark_1.2kV_3-6-8_SmallVRange_IW114-0004", 
#                 "01Jul24_81V_1.33V_1.2kV_3-6-8_SmallVRange_IW098-0028", "01Jul24_81V_1.33V_1.2kV_3-6-8_SmallVRange_IW114-0004", 
#                 "01Jul24_81V_1.34V_1.2kV_3-6-8_SmallVRange_IW098-0028", "01Jul24_81V_1.34V_1.2kV_3-6-8_SmallVRange_IW114-0004", 
#                 "01Jul24_81V_1.35V_1.2kV_3-6-8_SmallVRange_IW098-0028", "01Jul24_81V_1.35V_1.2kV_3-6-8_SmallVRange_IW114-0004"]
#inputFileList = ["01Jul24_81V_1.35V_1.2kV_3-6-8_SmallVRange_IW098-0028", "01Jul24_81V_1.35V_1.2kV_3-6-8_SmallVRange_IW114-0004"]
#inputFileList = ["01Jul24_80V_1.33V_1.2kV_3-6-8_SwappedConnections_IW098-0028", "01Jul24_80V_1.33V_1.2kV_3-6-8_SwappedConnections_IW114-0004", 
#                 "01Jul24_80V_1.33V_1.2kV_3-6-8_SwappedConnectionsAndOrder_IW098-0028", "01Jul24_80V_1.33V_1.2kV_3-6-8_SwappedConnectionsAndOrder_IW114-0004"]
#inputFileList = ["02Jul24_81V_1.33V_0kV_3-6-8_IW098-0028", "02Jul24_81V_1.33V_0kV_3-6-8_IW114-0004", 
#                 "02Jul24_81V_1.34V_0kV_3-6-8_IW098-0028", "02Jul24_81V_1.34V_0kV_3-6-8_IW114-0004",
#                 "02Jul24_81V_1.35V_0kV_3-6-8_IW098-0028", "02Jul24_81V_1.35V_0kV_3-6-8_IW114-0004"]
#inputFileList = ["03Jul24_81V_1.33V_0kV_3-6-8_IW098-0028", "03Jul24_81V_1.33V_0kV_3-6-8_IW114-0004", 
#                 "03Jul24_81V_1.34V_0kV_3-6-8_IW098-0028", "03Jul24_81V_1.34V_0kV_3-6-8_IW114-0004",
#                 "03Jul24_81V_1.35V_0kV_3-6-8_IW098-0028", "03Jul24_81V_1.35V_0kV_3-6-8_IW114-0004"]
#inputFileList = ["03Jul24_81V_1.33V_0kV_3-6-8_Delay_IW098-0028", "03Jul24_81V_1.33V_0kV_3-6-8_Delay_IW114-0004", 
#                 "03Jul24_81V_1.34V_0kV_3-6-8_Delay_IW098-0028", "03Jul24_81V_1.34V_0kV_3-6-8_Delay_IW114-0004",
#                 "03Jul24_81V_1.35V_0kV_3-6-8_Delay_IW098-0028", "03Jul24_81V_1.35V_0kV_3-6-8_Delay_IW114-0004"]
#inputFileList = ["08Jul24_81V_700mV_0kV_3-6-8_Delay_IW098-0028", "08Jul24_81V_700mV_0kV_3-6-8_Delay_IW114-0004", 
#                 "08Jul24_81V_800mV_0kV_3-6-8_Delay_IW098-0028", "08Jul24_81V_800mV_0kV_3-6-8_Delay_IW114-0004",
#                 "08Jul24_81V_900mV_0kV_3-6-8_Delay_IW098-0028", "08Jul24_81V_900mV_0kV_3-6-8_Delay_IW114-0004"]
#inputFileList = ["08Jul24_81V_900mV_0kV_3-6-8_IW098-0028", "08Jul24_81V_900mV_0kV_3-6-8_IW114-0004"]
#inputFileList = ["08Jul24_81V_Dark_0kV_3-6-8_IW098-0028", "08Jul24_81V_Dark_0kV_3-6-8_IW114-0004"]
#inputFileList = ["09Jul24_81V_Dark_0kV_3-6-8_IW098-0028", "09Jul24_81V_Dark_0kV_3-6-8_IW114-0004",
#                 "09Jul24_81V_805mV_0kV_3-6-8_IW098-0028", "09Jul24_81V_805mV_0kV_3-6-8_IW114-0004", 
#                 "09Jul24_81V_810mV_0kV_3-6-8_IW098-0028", "09Jul24_81V_810mV_0kV_3-6-8_IW114-0004",
#                 "09Jul24_81V_820mV_0kV_3-6-8_IW098-0028", "09Jul24_81V_820mV_0kV_3-6-8_IW114-0004",
#                 "09Jul24_81V_830mV_0kV_3-6-8_IW098-0028", "09Jul24_81V_830mV_0kV_3-6-8_IW114-0004", 
#                 "09Jul24_81V_840mV_0kV_3-6-8_IW098-0028", "09Jul24_81V_840mV_0kV_3-6-8_IW114-0004",
#                 "09Jul24_81V_850mV_0kV_3-6-8_IW098-0028", "09Jul24_81V_850mV_0kV_3-6-8_IW114-0004",
#                 "09Jul24_81V_860mV_0kV_3-6-8_IW098-0028", "09Jul24_81V_860mV_0kV_3-6-8_IW114-0004", 
#                 "09Jul24_81V_870mV_0kV_3-6-8_IW098-0028", "09Jul24_81V_870mV_0kV_3-6-8_IW114-0004",
#                 "09Jul24_81V_880mV_0kV_3-6-8_IW098-0028", "09Jul24_81V_880mV_0kV_3-6-8_IW114-0004",
#                 "09Jul24_81V_890mV_0kV_3-6-8_IW098-0028", "09Jul24_81V_890mV_0kV_3-6-8_IW114-0004", 
#                 "09Jul24_81V_900mV_0kV_3-6-8_IW098-0028", "09Jul24_81V_900mV_0kV_3-6-8_IW114-0004",
#                 "09Jul24_81V_905mV_0kV_3-6-8_IW098-0028", "09Jul24_81V_905mV_0kV_3-6-8_IW114-0004",
#                 "09Jul24_81V_920mV_0kV_3-6-8_IW098-0028", "09Jul24_81V_920mV_0kV_3-6-8_IW114-0004"]
inputFileList = ["12Jul24_81V_Dark_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_Dark_1.2kV_3-6-8_31deg_IW114-0004",
                 "12Jul24_81V_805mV_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_805mV_1.2kV_3-6-8_31deg_IW114-0004", 
                 "12Jul24_81V_810mV_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_810mV_1.2kV_3-6-8_31deg_IW114-0004",
                 "12Jul24_81V_820mV_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_820mV_1.2kV_3-6-8_31deg_IW114-0004",
                 "12Jul24_81V_830mV_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_830mV_1.2kV_3-6-8_31deg_IW114-0004", 
                 "12Jul24_81V_840mV_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_840mV_1.2kV_3-6-8_31deg_IW114-0004",
                 "12Jul24_81V_850mV_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_850mV_1.2kV_3-6-8_31deg_IW114-0004",
                 "12Jul24_81V_860mV_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_860mV_1.2kV_3-6-8_31deg_IW114-0004", 
                 "12Jul24_81V_870mV_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_870mV_1.2kV_3-6-8_31deg_IW114-0004",
                 "12Jul24_81V_880mV_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_880mV_1.2kV_3-6-8_31deg_IW114-0004",
                 "12Jul24_81V_890mV_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_890mV_1.2kV_3-6-8_31deg_IW114-0004", 
                 "12Jul24_81V_900mV_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_900mV_1.2kV_3-6-8_31deg_IW114-0004",
                 "12Jul24_81V_905mV_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_905mV_1.2kV_3-6-8_31deg_IW114-0004",
                 "12Jul24_81V_920mV_1.2kV_3-6-8_31deg_IW098-0028", "12Jul24_81V_920mV_1.2kV_3-6-8_31deg_IW114-0004"]

###############################################################################
###                       Data extraction functions                         ###
###############################################################################

def byteBin(byte):
    return '{0:08b}'.format(ord(byte))

def byteHex(byte):
    return '{0:02x}'.format(ord(byte))

def bytesBin(f,n):
    s = ""
    for i in range(n):
        s += byteBin(f.read(1))
    return s

def bytesInt(f,n):
    s = ""
    for i in range(n):
        s += byteHex(f.read(1))
    return int(s,16)

def bytesHex(f,n):
    s = ""
    for i in range(n):
        s += byteHex(f.read(1))
    return s

def bytesTwos(f,n):
    hexStr = bytesHex(f,n)

    value = int(hexStr, 16)
    if value & (1 << (n * 8 - 1)):
        value -= 1 << n * 8
    return value

def bytesString(f, n = 0): # xxx: if n = 0, read until 0 byte, else read n bytes
    c = f.read(1)
    s = ''
    while c != b'\0':
        s += c.decode()
        c = f.read(1)
    return s

def adc2mv(value, range):
    return (value / 32512) * ps6000VRanges[range]

def readHeader(f):
    d = {}
    nCh = 4
    b = f.read(1)
    d['timebase'] = ord(b) >> 4
    d['activeChannels'] = byteBin(b)[4:]
    d['activeTriggers'] = byteBin(f.read(1))[3:]
    d['auxTriggerThreshold'] = adc2mv(bytesTwos(f,2),6)
    for i in range(nCh):
        d['ch' + chr(ord('A') + i) + 'TriggerThreshold'] = adc2mv(bytesTwos(f,2),6)
    vRanges = bytesBin(f,2)
    for i in range(nCh):
        d['ch' + chr(ord('A') + i) + 'VRange'] = int(vRanges[4*i:4*(i + 1)],2)
    for i in range(nCh):
        d['ch' + chr(ord('A') + i) + 'Samples'] = bytesInt(f,2)
    d['preTriggerSamples'] = bytesInt(f,2)
    d['numWaveforms'] = bytesInt(f,4)
    d['timestamp'] = bytesTwos(f,4)
    
    d['modelNumber'] = bytesString(f)
    d['serialNumber'] = bytesString(f)

    return d

def readData(f, d):

    data = [] 

    for ch in range(4):
        if d['activeChannels'][ch] == '0':
            continue
        nWf = d['numWaveforms']
        nSamples = d['ch' + chr(ord('A') + ch) + 'Samples']
        chADCData = np.fromfile(f, dtype='>i2', count=nWf * nSamples).reshape((nWf,nSamples))
        chData = adc2mv(chADCData, d['ch' + chr(ord('A') + ch) + 'VRange'])

        data.append(chData)

    return data
    
def getTimebase(f, d):
    return (2**d['timebase']) * 0.2
    
###############################################################################
###                           Pre-analysis functions                        ###
###############################################################################  
 
def histogramming (d, numBins):

    binWidth = len(d) * 1.0 / numBins
    data = []
    xAxis = 0
    upperEdge = binWidth
    count = 0
    value = 0
    for ch in d:
        if count < upperEdge:
            value += ch
        else:
            data.append(value)
            value = ch
            upperEdge += binWidth
        count += 1
        #value /= binWidth
    data.append(value)

    return data
  
def moving_average(d, n, i1, i2):
    return [np.mean(d[i-n:i+n]) for i in range(max(i1, 0)+n, min(i2, len(d))-n-1)]  

def lowerBaseline(d, numSigma, i1, i2):
    mu = np.mean(d[max(i1, 0):min(i2, len(d))])
    sigma = np.std(d[max(i1, 0):min(i2, len(d))])
    return mu-numSigma*sigma
    
def charge_integration(d, i1, i2, upperLimit):
    summingCondition = (d[max(0,i1):min(len(d)-1,i2)] < upperLimit)
    return np.sum(d[max(0,i1):min(len(d)-1,i2)]*timeBase, where=summingCondition)
    
###############################################################################
###                             Fitting functions                           ###
###############################################################################

def normalize(x, y):
    return sum( [ y[i]*(x[i+1]-x[i]) for i in range(len(x)-1) ] )

def binx(bins):
    x = []
    for i in range(len(bins)-1):
        x.append( (bins[i]+bins[i+1])/2 )
    return np.array(x)

def normal(x, mu, sg):
    return np.exp( -0.5*(x-mu)**2 / sg**2 ) / ( sg*np.sqrt(2*np.pi) )

def distrib(q, lb, q0, sg0, mu, sg):
    ### charge distribution 
    ### following a poisson number of pe (lb), with normal gain (mu, sg)
    ### and noise factor noisefac
    P = 0
    for k in range(kmax):
        P += ( lb**k * np.exp(-lb) / math.factorial(k) ) * normal( q, k*mu+q0, np.sqrt( (k*sg)**2 + sg0**2 ) )
    return P      

###############################################################################
###                        Plotting and saving functions                    ###
###############################################################################

def plotting_plot (d, plotTitle, xLabel, yLabel, showing, ls='-'):
    fig = plt.figure()
    plt.plot(d, ls)
    plt.xlabel(xLabel)
    plt.ylabel(yLabel)
    plt.title(plotTitle)
    if showing ==  0:

    	plt.show()
    plt.close()
    return fig

def plotting_plots (D, plotTitle, xLabel, yLabel, showing, nplots, ls='.'):
    fig = plt.figure()
    plotcounter = 0
    for d in D:
        if plotcounter > nplots:
            break
        plt.plot(d, ls)
        plotcounter += 1
    plt.xlabel(xLabel)
    plt.ylabel(yLabel)
    plt.title(plotTitle)
    if showing ==  0:
        plt.show()
    plt.close()
    return fig
    

def plotting_hist(d, numBins, plotTitle, xLabel, yLabel, showing):

    fig = plt.figure()
    plt.hist(d, numBins)
    plt.xlabel(xLabel)
    plt.ylabel(yLabel)
    plt.title(plotTitle)
    if showing ==  0:
    	plt.show()
    plt.close()
    return fig
    
def plotting_fit (d, nbins, beginRange, endRange, x, typeFit, par, plotTitle, xLabel, yLabel, showing):
    fig = plt.figure()
    if typeFit == "Poiss-Gauss":
        yfit = distrib(x[i1:i2], *par)
        plt.hist(d, nbins, range=[beginRange, endRange], histtype='step', density=1, label=plotTitle
                 + '\n' + r'$N_\text{bins}$ = ' + str(nbins))
        plt.plot(x[i1:i2], yfit, ls='-', label='Poissonian-gaussian fit' 
                 + '\n' + r'[$\lambda$, $Q_0$, $\sigma_0$, $\mu$, $\sigma$] ='
                 + '\n[{:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f}]'.format(*par))
    elif typeFit == "Gauss":
        yfit = normal(x[i1:i2], *par)
        plt.hist(d, nbins, range=[beginRange, endRange], histtype='step', density=1, label=plotTitle
                 + '\n' + r'$N_\text{bins}$ = ' + str(nbins))
        plt.plot(x[i1:i2], yfit, ls='-', label='Gaussian fit' 
                 + '\n' + r'[$\mu$, $\sigma$] = [{:.3f}, {:.3f}]'.format(*par))
    plt.xlabel(xLabel)
    plt.ylabel(yLabel)
    plt.legend()
    if showing ==  0:
    	plt.show()
    plt.close()
    return fig
    
def saving_plots(plots, outputFile):
    pdfFile = PdfPages(outputFile)
    for plot in plots:
        pdfFile.savefig(plot)
    pdfFile.close() 

###############################################################################
###                                 Main                                    ###
###############################################################################

analysisType = sys.argv[1]
if analysisType == "pre-analyse":
    print("\n###### Starting extraction and pre-analysis...\n")
    for inputFile in inputFileList: 
        inputFilePath = inputFile + ".dat"
        print("### Extracting data from '" + inputFilePath + "'...")
        f = open(inputDirectory + inputFilePath, 'rb')
        header = readHeader(f)
        print(header)
        data = readData(f, header)
        timebase = getTimebase(f, header)
        ch = 0
        for chData in data:
            print("### Analyzing channel " + chr(ord('A') + ch) + "...")
            countMaWfPlots = 0
            countWfPlots = 0
            plots = []
            maChData = []
            minimumMaChData = []
            minimumChData = []
            indexMinMaChData = []
            indexMinChData = []
            inteChargeChData = []
            fullInteChargeChData = []
            eventcounter = 0
            minimumAllWf = np.min(chData) 
            for wfData in chData:
                minimumWf = np.min(wfData)
                minimumChData.append(minimumWf)
                minimumIndex = np.where(wfData == minimumWf)[0]
                for index in minimumIndex:
                    indexMinChData.append(index)
                if (np.min(wfData) == minimumAllWf and countWfPlots < 3):
                    plots.append(plotting_plot(wfData, 'Largest charge waveform for ch' + chr(ord('A') + ch), 'binned time', 'charge [mV]', showing))
                    countWfPlots += 1
                if withMa ==0:
                    maWfData = moving_average(wfData, 10, minimumIndex[0]-windowlow, minimumIndex[0]+windowhigh)
                    maChData.append(maWfData)
                nSig = lowerBaseline(wfData, numSigmaBaseline, 0, 100)
                integratedCharge = charge_integration(wfData, minimumIndex[0]-windowlow, minimumIndex[0]+windowhigh, nSig)
                inteChargeChData.append(integratedCharge)
                fullIntegratedCharge = charge_integration(wfData, 0, len(wfData)-1, nSig)
                fullInteChargeChData.append(fullIntegratedCharge)
                eventcounter += 1
                if eventcounter%5000==0:
            	    print("Processed {} events".format(eventcounter))
            plots.append(plotting_plots(chData, 'First {} waveforms for ch{}'.format(nplots, chr(ord('A') + ch)), 'binned time', 'charge [mV]', showing, nplots))              
            nBins = round((np.max(indexMinChData) - np.min(indexMinChData))/2)
            if nBins < 1:
                nBins = 1
            plots.append(plotting_hist(indexMinChData, nBins, 
                         'Charge peak index for ch' + chr(ord('A') + ch), 
                         'minimum charge bin index', 'frequency', showing))
            nBins = round((np.max(minimumChData) - np.min(minimumChData)) / 0.8)
            if nBins < 1:
                nBins = 1
            plots.append(plotting_hist(minimumChData, nBins, 
                         'Charge peak frequency for ch' + chr(ord('A') + ch), 
                         'minimum charge [mV]', 'frequency', showing))
            nBins = round((np.max(inteChargeChData) - np.min(inteChargeChData)) / 8)
            if nBins < 1:
                nBins = 1
            plots.append(plotting_hist(inteChargeChData, nBins, 
                         'Integrated charge frequency for ch' + chr(ord('A') + ch), 
                         'Integrated charge [mV.ns]', 'frequency', showing))
            nBins = round((np.max(fullInteChargeChData) - np.min(fullInteChargeChData)) / 8)
            if nBins < 1:
                nBins = 1
            plots.append(plotting_hist(fullInteChargeChData, nBins, 
                         'Full integrated charge frequency for ch' + chr(ord('A') + ch), 
                         'Full integrated charge [mV.ns]', 'frequency', showing))
            if withMa == 0:
                minimumAllMaWf = np.min(maChData) 
                for maWfData in maChData:
                    minimumMaWf = np.min(maWfData)
                    minimumMaChData.append(minimumMaWf)
                    minimumIndexMa = np.where(maWfData == minimumMaWf)[0]
                    for indexMa in minimumIndexMa:
                        indexMinMaChData.append(indexMa)
                    if (np.min(maWfData) == minimumAllMaWf and countMaWfPlots < 3):
                        plots.append(plotting_plot(maWfData, 'Largest charge waveform for ch' + chr(ord('A') + ch), 
                                                   'binned time', 'charge [mV]', showing))
                        countMaWfPlots += 1
                nBins = round((np.max(indexMinMaChData) - np.min(indexMinMaChData)))
                if nBins < 1:
                    nBins = 1
                plots.append(plotting_hist(indexMinMaChData, nBins, 
                             'Charge peak index for ch' + chr(ord('A') + ch), 
                             'minimum charge bin index', 'frequency', showing))
                nBins = round((np.max(minimumMaChData) - np.min(minimumMaChData)) / 0.4)
                if nBins < 1:
                    nBins = 1
                plots.append(plotting_hist(minimumMaChData, nBins, 
                             'Charge peak frequency for ch' + chr(ord('A') + ch), 
                             'minimum charge [mV]', 'frequency', showing))
            if saving == 0:
                print("### Saving...")
                if withMa == 0:
                    chOutputFileNp = npyDirectory + inputFile[0:7] + "/" + inputFile + "_ch" + chr(ord('A') + ch) + "_minimumMaChData.npy"
                    np.save(chOutputFileNp, np.array(minimumMaChData))
                chOutputFileNp = npyDirectory + inputFile[0:7] + "/" + inputFile + "_ch" + chr(ord('A') + ch) + "_inteChargeChData.npy"
                np.save(chOutputFileNp, np.array(inteChargeChData))
                chOutputFilePdf = pdfDirectory + inputFile[0:7] + "/" + analysisType + "/" + inputFile + "_ch" + chr(ord('A') + ch) + ".pdf"
                saving_plots(plots, chOutputFilePdf)
            ch += 1
    print("\n###### Extraction and analysis finished !\n")
elif analysisType == "analyse":
    typeFit = sys.argv[2]
    print("\n###### Starting analysis...\n")
    for inputFile in inputFileList:
        plots = []
        ch = 0
        inputfilepath = npyDirectory + inputFile[0:7] + "/" + inputFile + "_ch" + chr(ord('A') + ch) + "_minimumMaChData.npy"
        existingFile = os.path.exists(inputfilepath)
        while existingFile == True:
            minima = np.load(inputfilepath)
            if beginRange > minima.min():
                beginRange = minima.min()
            if endRange > minima.max():
                endRange = minima.max()
            ch += 1
            inputfilepath = npyDirectory + inputFile[0:7] + "/" + inputFile + "_ch" + chr(ord('A') + ch) + "_minimumMaChData.npy"
            existingFile = os.path.exists(inputfilepath)
        ch = 0
        inputfilepath = npyDirectory + inputFile[0:7] + "/" + inputFile + "_ch" + chr(ord('A') + ch) + "_minimumMaChData.npy"
        existingFile = os.path.exists(inputfilepath)
        while existingFile == True:
            print("### Analysing '"+ inputfilepath +"'")
            minima = np.load(inputfilepath)
            hist, bins = np.histogram(minima, nbins, range=[beginRange, endRange])
            x = binx(bins)
            histnorm = hist/normalize(x, hist)
            par, cov = optimize.curve_fit(distrib, x[i1:i2], histnorm[i1:i2], p0=p0_chargeDist, bounds=p0bounds_chargeDist)
            plots.append(plotting_fit(minima, nbins, beginRange, endRange,
                         x, typeFit, par, "Charge distribution ch" + chr(ord('A') + ch), 
                         "Minimum charge [mV]", "Normalised frequency", showing))
            ch += 1
            inputfilepath = npyDirectory + inputFile[0:7] + "/" + inputFile + "_ch" + chr(ord('A') + ch) + "_minimumMaChData.npy"
            existingFile = os.path.exists(inputfilepath)
        beginRange = 0
        endRange = 0
        ch = 0
        inputfilepath = npyDirectory + inputFile[0:7] + "/" + inputFile + "_ch" + chr(ord('A') + ch) + "_inteChargeChData.npy"
        existingFile = os.path.exists(inputfilepath)
        while existingFile == True:
            inteCharge = np.load(inputfilepath)
            if beginRange > inteCharge.min():
                beginRange = inteCharge.min()
            if endRange < inteCharge.max():
                endRange = inteCharge.max()
            ch += 1
            inputfilepath = npyDirectory + inputFile[0:7] + "/" + inputFile + "_ch" + chr(ord('A') + ch) + "_inteChargeChData.npy"
            existingFile = os.path.exists(inputfilepath)
        ch = 0
        inputfilepath = npyDirectory + inputFile[0:7] + "/" + inputFile + "_ch" + chr(ord('A') + ch) + "_inteChargeChData.npy"
        existingFile = os.path.exists(inputfilepath)
        while existingFile == True:
            print("### Analysing '"+ inputfilepath +"'")
            inteCharge = np.load(inputfilepath)
            if ch!=3:
                hist, bins = np.histogram(inteCharge, nbins, range=[beginRange, endRange])
                x = binx(bins)
                histnorm = hist/normalize(x, hist)
                if typeFit == "Gauss": 
                    par, cov = optimize.curve_fit(normal, x[i1:i2], histnorm[i1:i2],
                                                  p0=p0_chargeInteHighPE, bounds=p0bounds_chargeInteHighPE)
                    plots.append(plotting_fit(inteCharge, nbins, beginRange, endRange,
                                 x, typeFit, par, "Integrated charge distribution ch" + chr(ord('A') + ch), 
                                 "Integrated charge [mV.ns]", "Normalised frequency", showing))
                elif typeFit == "Poiss-Gauss":
                    par, cov = optimize.curve_fit(distrib, x[i1:i2], histnorm[i1:i2],
                                                  p0=p0_chargeInte, bounds=p0bounds_chargeInte)
                    plots.append(plotting_fit(inteCharge, nbins, beginRange, endRange,
                                 x, typeFit, par, "Integrated charge distribution ch" + chr(ord('A') + ch), 
                                 "Integrated charge [mV.ns]", "Normalised frequency", showing))
            else:
                nBins = round((np.max(inteCharge) - np.min(inteCharge)) / 2)
                if nBins < 1:
                    nBins = 1
                plots.append(plotting_hist(inteCharge, nBins,
                             'Integrated charge frequency for ch' + chr(ord('A') + ch), 
                             'Integrated charge [mV.ns]', 'frequency', showing))
            ch += 1
            inputfilepath = npyDirectory + inputFile[0:7] + "/" + inputFile + "_ch" + chr(ord('A') + ch) + "_inteChargeChData.npy"
            existingFile = os.path.exists(inputfilepath)
        if saving == 0:
            print("### Saving...")
            chOutputFilePdf = pdfDirectory  + inputFile[0:7] + "/" + analysisType + "/" + inputFile + ".pdf"
            saving_plots(plots, chOutputFilePdf)
    print("\n###### Analysis finished !\n")
else:
    print("ERROR: You should either 'pre-analyse' or 'analyse' files...")
