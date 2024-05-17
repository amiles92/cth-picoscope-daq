# import sys, os
# sys.setdlopenflags(os.RTLD_GLOBAL | os.RTLD_LAZY)

import daq
outFile = "./out.dat"

serials = daq.getSerials()
picoscopes = serials.split(",")
# if len(picoscopes) > 1:
print(picoscopes)
ps = 0
ps = daq.runDAQ(outFile,
            0, 2, 8,
            0, 99, 0,
            0, 99, 0,
            0, 99, 0,
            500, 2, 10, 0, picoscopes[0]
            )
print(ps)

print("Continued anyways")
# check outfile

