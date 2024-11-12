import numpy as np
from scipy.stats import moyal
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt

ps6000VRanges = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 
                 10000, 20000, 50000]

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

def readDataAdc(f, d):
    
    data = []

    for ch in range(4):
        if d['activeChannels'][ch] == '0':
            continue
        nWf = d['numWaveforms']
        nSamples = d['ch' + chr(ord('A') + ch) + 'Samples']
        chADCData = np.fromfile(f, dtype='>i2', count=nWf * nSamples).reshape((nWf,nSamples))

        data.append(chADCData)

    return data

def readFile(fname):
    with open(fname, 'rb') as f:
        header = readHeader(f)
        data = readData(f, header)
    return header, data

# def fittingFn(x, dc, ampl, mu, sig):
#     return dc + ampl * moyal.pdf(x, mu, sig)
def fittingFn(x, dc, ampl, step, lb=0.05):
    # if x < step: return dc
    return dc + ampl * np.exp(-lb * (x-step)) / (1 + np.exp(-(x - step)))

def fittingFn2(x, dc, ampl, ampl2, step, step2):
    lb=0.05
    # if x < step: return dc
    if (abs(step - step2) < 3): return np.inf
    return dc + ampl * np.exp(-lb * (x-step)) / (1 + np.exp(-(x - step))) + \
                ampl2 * np.exp(-lb * (x-step2)) / (1 + np.exp(-(x - step2)))

def dcFitLol(x, dc):
    # if x < step: return dc
    return np.zeros(x.shape) + dc# + ampl * np.exp(-lb * (x-step)) / (1 + np.exp(-(x - step)))


fname = "../data/02Sep24/7-8-9/2024-09-02_81V_840mV_1.4kV_7-8-9_IW114-0004.dat"

h, d = readFile(fname)

chData = d[3]

# for wfN in range(12, chData.shape[0]):
#     plt.figure()
#     plt.plot(chData[wfN])
#     plt.title(wfN)
#     plt.show()
selections = {3, 4, 7, 9, 10, 11}

goodboi = 11

for s in range( chData.shape[0]):

    goodWf = np.array(chData[s][:50])
    xdata = np.array(range(len(goodWf)))

    print(goodWf.shape, xdata.shape)

    p0_1 = [0, -5, 25]#, 0.05]
    bounds_1 = ([-200, -200, -200], [200, -2, 200])
    popt_1, _ = curve_fit(fittingFn, xdata, goodWf, p0=p0_1, bounds=bounds_1)
    # np.sum((goodWf - fittingFn(xdata, *popt_1)) ** 2)) / np.sum((goodWf - np.mean(goodWf)) ** 2)
    print(popt_1, 1 - np.sum((goodWf - fittingFn(xdata, *popt_1)) ** 2) / np.sum((goodWf - np.mean(goodWf)) ** 2))

    p0_2 = [0, -5, -5, 5, 45]
    bounds_2 = (-200, [200, -2, -2, 200, 200])
    popt_2, _ = curve_fit(fittingFn2, xdata, goodWf, p0=p0_2, bounds=bounds_2)
    print(popt_2, 1 - np.sum((goodWf - fittingFn2(xdata, *popt_2)) ** 2) / np.sum((goodWf - np.mean(goodWf)) ** 2))

    popt_3, _ = curve_fit(dcFitLol, xdata, goodWf, p0=[0])
    print(popt_3, 1 - np.sum((goodWf - dcFitLol(xdata, *popt_3)) ** 2) / np.sum((goodWf - np.mean(goodWf)) ** 2))


    plt.figure()
    plt.plot(xdata, goodWf[:100])
    plt.plot(xdata, dcFitLol(xdata, *popt_3), 'k--')
    plt.plot(xdata, fittingFn2(xdata, *popt_2), 'g--')
    plt.plot(xdata, fittingFn(xdata, *popt_1), 'r--')
    plt.show()
