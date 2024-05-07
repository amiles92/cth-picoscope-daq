import sys, os
sys.setdlopenflags(os.RTLD_GLOBAL | os.RTLD_LAZY)
sys.path.append("tmp/")

import daq

ps = daq.runDAQ()

