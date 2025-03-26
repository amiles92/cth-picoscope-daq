import numpy as np
import matplotlib.pyplot as plt

psVRanges = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 
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

def adc2mv16Bit(value, range):
    return (value / 32512) * psVRanges[range]

def adc2mv8Bit(value, range):
    return (value / 128) * psVRanges[range]

def readHeader(f):
    d = {}
    nCh = 4
    b = f.read(1)
    d['timebase'] = ord(b) >> 4
    d['activeChannels'] = byteBin(b)[4:]
    trigAndDataSize = byteBin(f.read(1))
    d['activeTriggers'] = trigAndDataSize[3:]
    d['8bitData'] = bool(int(trigAndDataSize[2]))
    d['auxTriggerThreshold'] = adc2mv16Bit(bytesTwos(f,2),6)
    
    tmpChTriggerThreshold = []
    for i in range(nCh):
        tmpChTriggerThreshold.append(bytesTwos(f,2))
    print(tmpChTriggerThreshold)
    vRanges = bytesBin(f,2)
    for i in range(nCh):
        vRange = int(vRanges[4*i:4*(i + 1)],2)
        d['ch' + chr(ord('A') + i) + 'VRange'] = vRange
        d['ch' + chr(ord('A') + i) + 'TriggerThreshold'] = adc2mv16Bit(tmpChTriggerThreshold[i],vRange)
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
        if not d['8bitData']:
            chADCData = np.fromfile(f, dtype='>i2', count=nWf * nSamples).reshape((nWf,nSamples))
            chData = adc2mv16Bit(chADCData, d['ch' + chr(ord('A') + ch) + 'VRange'])
        else:
            chADCData = np.fromfile(f, dtype='i1', count=nWf * nSamples).reshape((nWf,nSamples))
            chData = adc2mv8Bit(chADCData, d['ch' + chr(ord('A') + ch) + 'VRange'])
        

        data.append(chData)

    return data

outFile = r"./data/08Jul24_81V_900mV_0kV_3-6-8_IW098-0028.dat"

f = open(outFile, 'rb')

header = readHeader(f)

data = readData(f, header)

for chData in data:
    print(np.max(chData))
    print(np.shape(chData))

plt.figure()
for i in range(100):
    plt.plot(data[0][i], alpha=0.2)
plt.figure()
for i in range(100):
    plt.plot(data[1][i], alpha=0.2)
plt.show()
