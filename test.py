# import sys, os
# sys.setdlopenflags(os.RTLD_GLOBAL | os.RTLD_LAZY)

import daq
outFile = "./out.dat"

serials = daq.getSerials()
picoscopes = serials.split(",")
if len(picoscopes) > 1:
    print(picoscopes)

ps = daq.runDAQ(outFile,
                10, 2, 100,
                0, 99, 0,
                0, 99, 0,
                0, 99, 0,
                0, 2, 100, 0, picoscopes[0]
                )
