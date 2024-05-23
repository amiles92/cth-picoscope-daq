import daq

outFile = r"./out.dat"

serials = daq.getSerials()
picoscopes = serials.split(",")
# if len(picoscopes) > 1:
print(picoscopes)
ps = 0
ps = daq.runDAQ(outFile,
            0, 2, 800,
            0, 2, 800,#0, 99, 0,
            0, 2, 800,#0, 99, 0,
            0, 2, 800,#0, 99, 0,
            500, 2, 20000, 200, picoscopes[0]
            )
print("\nPicoscope response:", ps)
# check outfile
