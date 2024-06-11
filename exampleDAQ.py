import daq6000a as daq

outFile = r"./example.dat"

# serials = daq.getSerials()
picoscopes = ['IW114/0004']#serials.split(",")
# picoscopes = daq.getSerials().split(",")
# if len(picoscopes) > 1:
print(picoscopes)
ps = 0
ps = daq.runFullDAQ(outFile,
            0, 2, 1100,
            0, 2, 1000,
            0, 2, 1000,
            0, 2, 1000,
            100, 2, 20000, 0, picoscopes[0])

daq.seriesInitDaq(picoscopes[0])
daq.seriesSetDaqSettings(
            0, 2, 1100,
            0, 2, 1000,
            0, 2, 1000,
            0, 2, 1000,
            100, 2, 20000, 0)
daq.seriesCollectData(outFile)
daq.seriesCloseDaq()

print("\nDAQ response:", ps)
