import daq6000

outFile = r"./example.dat"

serials = daq6000.getSerials()
picoscopes = serials.split(",")
# if len(picoscopes) > 1:
print(picoscopes)
ps = 0
ps = daq6000.runDAQ(outFile,
            0, 2, 1100,
            0, 2, 1000,
            0, 2, 1000,
            0, 2, 1000,
            500, 2, 20000, 0, picoscopes[0])

print("\nDAQ response:", ps)
