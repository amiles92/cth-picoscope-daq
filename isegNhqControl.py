import time
import serial

################################################################################
##########        Serial connection functions
################################################################################

def initSerial(portNum):
    if portNum not in range(10):
        print("Invalid port number:", portNum)
    ser = serial.Serial("/dev/ttyUSB" + str(portNum), baudrate=9600)
    testResponse = isegGetSystemInformation(ser)
    if len(testResponse) != 4:
        print("Incorrect port configuration!")
        print("Test response:",testResponse)
        return None
    return ser

def slowWrite(ser, msg, delay=0.05):
    c = 0
    msg += '\r\n'
    for letter in msg:
        c += ser.write(str.encode(letter))
        time.sleep(delay)
    return c

def readCommand(ser, command):
    slowWrite(ser, command)
    commLine = ser.readline().strip().decode()
    responseLine = ser.readline().strip().decode()
    if '?' in commLine or '?' in responseLine:
        print("Error ocurred in issuing command:", command)
        print("Command Returned:", commLine)
        print("Response Returned:", responseLine)
        return 1
    if commLine != command:
        print("Mismatched issued command and returned command lines")
        print("Issued command:", command)
        print("Command returned:", commLine)
        return 2
    return responseLine

def writeCommand(ser, command):
    slowWrite(ser, command)
    commLine = ser.readline().strip().decode()
    responseLine = ser.readline().strip().decode()
    if '?' in commLine or '?' in responseLine:
        print("Error ocurred in issuing command:", command)
        print("Command Returned:", commLine)
        print("Response Returned:", responseLine)
        return 1
    if commLine != command:
        print("Mismatched issued command and returned command lines")
        print("Issued command:", command)
        print("Command returned:", commLine)
        return 2
    if responseLine != "":
        print("Unexpected response from command:", command)
        print("Command Returned:", commLine)
        print("Response Returned:", responseLine)
        return 3
    return 0

################################################################################
##########        Read functions
################################################################################

def isegGetSystemInformation(ser):
    return readCommand(ser, "#").split(";")

def isegGetActualVoltage(ser):
    return int(readCommand(ser, "U1"))

def isegGetActualCurrent(ser):
    response = readCommand(ser, "I1")
    mantissa = float(response[:4])
    exponent = int(response[4:])
    return mantissa * 10**exponent

def isegGetVoltageLimit(ser):
    fraction = float(readCommand(ser, "M1"))
    maxVoltage = isegGetSystemInformation(ser)[2]
    return fraction * maxVoltage / 100.

def isegGetCurrentLimit(ser):
    fraction = float(readCommand(ser, "N1"))
    maxCurrent = isegGetSystemInformation(ser)[3]
    return fraction * maxCurrent / 100.

def isegGetSetVoltage(ser):
    return int(readCommand(ser, "D1"))

def isegGetVoltageRampSpeed(ser):
    return int(readCommand(ser, "V1"))

def isegGetStatus(ser):
    return readCommand(ser, "S1")

################################################################################
##########        Control and write functions
################################################################################

def intToConstLenString(n, length):
    intString = str(n)
    return "0" * (length - len(intString)) + intString

def isegSetVoltage(ser, voltage):
    command = "D1=" + intToConstLenString(voltage, 4)
    return writeCommand(ser, command)

def isegSetVoltageRampSpeed(ser, rampSpeed):
    command = "V1=" + intToConstLenString(rampSpeed, 3)
    return writeCommand(ser, command)

def isegStartVoltageRamp(ser):
    return readCommand(ser, "G1")

################################################################################
##########        Main
################################################################################

def main():
    ser = initSerial(0)
    isegGetSystemInformation(ser)

    isegSetVoltage(ser, 100)
    isegSetVoltageRampSpeed(ser, 10)
    isegStartVoltageRamp(ser)
    time.sleep(15)

    print(isegGetActualVoltage(ser))
    isegSetVoltage(ser, 0)
    isegStartVoltageRamp(ser)
    time.sleep(15)

    print(isegGetActualVoltage(ser))

if __name__ == "__main__":
    main()
