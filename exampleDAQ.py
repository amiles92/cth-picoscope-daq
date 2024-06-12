import daq6000a as daq

outFile1 = r"./example1.dat"
outFile2 = r"./example2.dat"
outFile3 = r"./example3.dat"
outFile4 = r"./example4.dat"

# picoscopes = ['IW114/0004']
picoscopes = ['IW098/0028']
# picoscopes = daq.getSerials().split(",")
# if len(picoscopes) > 1:
print(picoscopes)
status = 0
# status = daq.runFullDAQ(outFile1,
#             0, 2, 1100,
#             0, 2, 1000,
#             0, 2, 1000,
#             0, 2, 1000,
#             100, 2, 20, 0, picoscopes[0])

status = daq.seriesInitDaq(picoscopes[0])
if status == 0:
    print("Python - Device not opened")
    exit()
print("Python - Opened PS")
daq.seriesSetDaqSettings(
            0, 2, 1100,
            0, 2, 1000,
            0, 2, 1000,
            0, 2, 1000,
            100, 2, 20000, 100)
print("Python - Daq settings set")
daq.seriesCollectData(outFile1)
daq.seriesCollectData(outFile2)
print("Python - Daq data collected")
daq.seriesSetDaqSettings(
            0, 2, 900,
            0, 2, 900,
            0, 2, 900,
            0, 2, 900,
            100, 2, 50000, 0)
daq.seriesCollectData(outFile3)
daq.seriesCollectData(outFile4)
daq.seriesCloseDaq()

print("\nPython - Daq response:", status)
