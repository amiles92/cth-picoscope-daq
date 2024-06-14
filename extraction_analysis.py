import numpy as np
from scipy.stats import norm
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages

###############################################################################
###                               Parameters                                ###
###############################################################################

### Data extraction
ps6000VRanges = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 
                 10000, 20000, 50000]
                 
### Analysis          
showing = 1
saving = 0
inputDirectory = "../data-files/"
#inputFileList = ["BothLED_81V_3.25V.dat", "RightLED_81V_3.25V.dat", 
#                 "LeftLED_81V_3.25V.dat", "exampleNoise.dat", "example4.dat"]
inputFileList = ["BothLED_81V_3.25V.dat", "RightLED_81V_3.25V.dat", "LeftLED_81V_3.25V.dat"]
outputDirectory = "plots/"

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
    
###############################################################################
###                   Analysis and plot saving functions                    ###
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
    
def plotting_plot (d, plotTitle, xLabel, yLabel, showing):

    fig = plt.figure()
    plt.plot(d)
    plt.xlabel(xLabel)
    plt.ylabel(yLabel)
    plt.title(plotTitle)
    if showing ==  0:
    	plt.show()
    plt.close()
    return fig
   
    
def plotting_hist (d, numBins, plotTitle, xLabel, yLabel, showing):

    fig = plt.figure()
    plt.hist(d, numBins)
    plt.xlabel(xLabel)
    plt.ylabel(yLabel)
    plt.title(plotTitle)
    plt.close()
    if showing ==  0:
    	plt.show()
    return fig
    
def moving_average(d, n):

    d = np.sort(d, axis=None)
    newD = []
    for i in range(len(d)):
        D = d[i]
        if (i >= n and i <= len(d) - (n + 1)):
            for j in range(n):
                D += d[i+j] + d[i-j]
            newD.append(D / (2*n+1))
        elif i < n:
            for j in range(n):
                D += d[i+j]
            j = i - 1
            count = 0
            while j > 0:
                D += d[j]
                count += 1
                j -= 1
            newD.append(D / (n + 1 + count))
        else:
            for j in range(n):
                D += d[i-j]
            j = i + 1
            count = 0
            while j < len(d):
                D += d[j]
                count += 1
                j += 1
            newD.append(D / (n + 1 + count))
            
    return newD
    
def saving_plots(plots, outputFile):

    pdfFile = PdfPages(outputFile)
    for plot in plots:
        pdfFile.savefig(plot)
    pdfFile.close() 

###############################################################################
###                                 Main                                    ###
###############################################################################

print("\nStarting extraction and analysis...\n")
for inputFile in inputFileList:
    
    outputFile = outputDirectory + inputFile.removesuffix('.dat')
    print("### Extracting data from '" + inputFile + "'...")
    f = open(inputDirectory + inputFile, 'rb')
    header = readHeader(f)
    data = readData(f, header)
    ch = 0
    for chData in data:
        print("###### Analyzing channel " + chr(ord('A') + ch) + "...")
        chOutputFile = outputFile + "_ch" + chr(ord('A') + ch) + ".pdf"
        countWfPlots = 0
        plots = []
        newChData = []
        minimumData = []
        indexMinData = []
        for wfData in chData:
            newWfData = histogramming(wfData, 500)
            newChData.append(newWfData)
        minimumAllWf = np.min(newChData) 
        for wfData in newChData:
            minimumWf = np.min(wfData)
            minimumData.append(minimumWf)
            minimumIndex = np.where(wfData == minimumWf)[0]
            for index in minimumIndex:
                indexMinData.append(index)
            if (np.min(wfData) == minimumAllWf and countWfPlots < 3):
                plots.append(plotting_plot(wfData, 'Largest charge waveform for ch' + chr(ord('A') + ch), 'binned time', 'charge [mV]', showing))
                countWfPlots += 1
        plots.append(plotting_hist(indexMinData, 500, 'Charge peak index for ch' + chr(ord('A') + ch), 'minimum charge bin index', 'frequency', showing))
        #minimumData = moving_average(minimumData, 10)
        nBins = round((np.max(minimumData) - np.min(minimumData)) / 0.4)
        if nBins < 1:
            nBins = 1
        plots.append(plotting_hist(minimumData, nBins, 
                      'Charge peak frequency for ch' + chr(ord('A') + ch), 
                      'minimum charge [mV]', 'frequency', showing))
        ch += 1
        if saving == 0:
            print("###### Saving...")
            saving_plots(plots, chOutputFile)
    print("")
print("Extraction and analysis finished !\n")
