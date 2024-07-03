import daq6000a as daq

outFile1 = r"./data/example"

outFileList = [r"./data/03Jul24_81V_1.35V_0kV_3-6-8_Delay",
               r"./data/03Jul24_81V_1.34V_0kV_3-6-8_Delay",
               r"./data/03Jul24_81V_1.33V_0kV_3-6-8_Delay"
               ]

picoscopes = ['IW098/0028','IW114/0004']
print(picoscopes)

for ps in picoscopes:
    status = daq.multiSeriesInitDaq(ps)
    if status == 0:
        print("Python - Device not opened")
        exit()

daq.multiSeriesSetDaqSettings(
            0, 2, 600,
            0, 2, 600,
            0, 2, 600,
            0, 2, 600,
            1, 2, 50000, -150)

for out in outFileList:
    print("\n\n\nNext DAQ: %s" % out)
    input("Press enter to begin...")
    daq.multiSeriesCollectData(out)
# daq.multiSeriesCollectData(outFile1)

daq.multiSeriesCloseDaq()

