#include <stdio.h>
#include <stdint.h>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>
#include <bitset>
#include <vector>
#include <assert.h>


#ifndef PS6000API
#include <libps6000/ps6000Api.h>
#endif

#ifndef PICO_STATUS
#include <libps6000/PicoStatus.h>
#endif

#ifndef PS6000WRAPPER
#include "libps6000/ps6000Wrapper.h"
#endif

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

using namespace std;
namespace py = pybind11;
constexpr auto byref = py::return_value_policy::reference_internal;

#define Sleep(a) usleep(1000*a)
#define scanf_s scanf
#define fscanf_s fscanf
#define memcpy_s(a,b,c,d) memcpy(a,c,d)

constexpr uint8_t b0{1 << 0};
constexpr uint8_t b1{1 << 1};
constexpr uint8_t b2{1 << 2};
constexpr uint8_t b3{1 << 3};
constexpr uint8_t b4{1 << 4};
constexpr uint8_t b5{1 << 5};
constexpr uint8_t b6{1 << 6};
constexpr uint8_t b7{1 << 7};


// typedef enum enBOOL{FALSE,TRUE} BOOL;

uint8_t N;
uint32_t numberWaveforms;
uint32_t samplesPreTrigger;

uint32_t	timebase = 8;
int16_t		oversample = 1;
int32_t      scaleVoltages = TRUE;

class dataCollectionConfig 
{
public:
    ofstream ostream;
    UNIT *unit;
    bitset<4> activeChannels;
    bitset<5> activeTriggers;
    bitset<5> timebase;
    bitset<16> chVoltageRanges;
    vector<uint8_t> chTriggerThresholdADC;
    uint8_t auxTriggerThresholdADC;
    vector<uint16_t> chSamplesPerWaveform;
    uint16_t samplesPreTrigger;
    uint16_t maxChSamples;
    dataCollectionConfig(UNIT *unit)
    {
        this->unit = unit;
    }
};


void openFile(const char * b, const char * c)
{
    ofstream outputFile;
    outputFile.open(b, ios::in | ios::binary);
    return;
}

void writeDataHeader(ofstream &output, dataCollectionConfig &dcc)
{
    // Bit layout, in order
    // 4 bits: ch1-4 active
    // 5 bits: ch1-4, aux trigger active
    // 5 bits: timebase (from 0-4 for ps6000)
    // 2 bits: padding
    // 8 bits: aux trigger threshold
    // 32 bits: number of waveforms
    // 32 (8*4) bits: trigger threshold (ch1-4)
    // 16 (4*4): ch1-4 voltage ranges (aux is always +-1V range)
    // 64 (4*16) bits: number of samples per waveform
    // 16 bits: number of samples before trigger
    // 8 bits: model string
    // total bits 160 (20 bytes)
    // 

    uint16_t chData = dcc.activeChannels.to_ullong() << 12 |
                      dcc.activeTriggers.to_ullong() <<  7 | 
                      dcc.timebase.to_ullong()       <<  2 ;
    uint8_t auxTrigThreshold = dcc.auxTriggerThresholdADC;
    uint32_t chTrigThreshold = dcc.chTriggerThresholdADC[0] << 24 |
                               dcc.chTriggerThresholdADC[1] << 16 |
                               dcc.chTriggerThresholdADC[2] <<  8 |
                               dcc.chTriggerThresholdADC[3] <<  0 ;
    uint16_t chVRange = dcc.chVoltageRanges.to_ullong();
    uint32_t numWaveforms;
    uint64_t numSamplesPerWf;
    uint16_t samplesPreTrig;

    ostringstream ss (ostream::binary);

    // ss << 


    // output 
}

void writeData(ofstream &output, uint32_t data)
{
    // output
}

void writeBitset(ofstream &output, bitset<8> bits)
{

}

void setActiveChannels(dataCollectionConfig &dcc, 
                       uint16_t *aChVoltage,
                       uint16_t *bChVoltage,
                       uint16_t *cChVoltage,
                       uint16_t *dChVoltage)
{
    bitset<4> activeChannels;
    if (*aChVoltage != 99) {activeChannels.set(3);}
    if (*bChVoltage != 99) {activeChannels.set(2);}
    if (*cChVoltage != 99) {activeChannels.set(1);}
    if (*dChVoltage != 99) {activeChannels.set(0);}
    dcc.activeChannels = activeChannels;

    uint16_t *chVoltage[4] = {aChVoltage, bChVoltage, cChVoltage, dChVoltage};
    SetVoltages(dcc.unit, *chVoltage);
    return;
}

void setTriggerConfig(dataCollectionConfig &dcc, 
                      int16_t *aTrigVoltageMv,
                      int16_t *bTrigVoltageMv,
                      int16_t *cTrigVoltageMv,
                      int16_t *dTrigVoltageMv,
                      int16_t *auxTrigVoltageMv)
{
    bitset<5> activeTriggers;
    if ((  aTrigVoltageMv != NULL) && (dcc.activeChannels[3] == 1)) 
    {
        activeTriggers.set(4);
        dcc.chTriggerThresholdADC.at(0) = mv_to_adc(*aTrigVoltageMv, 
            dcc.unit->channelSettings[PS6000_CHANNEL_A].range);
    }

    if ((  bTrigVoltageMv != NULL) && (dcc.activeChannels[2] == 1)) 
    {
        activeTriggers.set(3);
        dcc.chTriggerThresholdADC.at(1) = mv_to_adc(*bTrigVoltageMv, 
            dcc.unit->channelSettings[PS6000_CHANNEL_B].range);
    }

    if ((  cTrigVoltageMv != NULL) && (dcc.activeChannels[1] == 1)) 
    {
        activeTriggers.set(2);
        dcc.chTriggerThresholdADC.at(2) = mv_to_adc(*cTrigVoltageMv, 
            dcc.unit->channelSettings[PS6000_CHANNEL_C].range);
    }

    if ((  dTrigVoltageMv != NULL) && (dcc.activeChannels[0] == 1)) 
    {
        activeTriggers.set(1);
        dcc.chTriggerThresholdADC.at(3) = mv_to_adc(*dTrigVoltageMv, 
            dcc.unit->channelSettings[PS6000_CHANNEL_D].range);
    }

    if (auxTrigVoltageMv != NULL) 
    {
        activeTriggers.set(0);
        dcc.auxTriggerThresholdADC = mv_to_adc(*dTrigVoltageMv, 
            dcc.unit->channelSettings[PS6000_CHANNEL_D].range);
    }
    dcc.activeTriggers = activeTriggers;

    SetTriggers(activeTriggers, dcc.chTriggerThresholdADC, dcc.auxTriggerThresholdADC);

    return;
}

ofstream setDataOutput(char *outputFileName)
{
    ofstream output;
    output.open(outputFileName, ofstream::out | ofstream::binary);
    return output;
}

void setDataConfig( uint8_t vibes
                    
                    )
{
    return;
}

void collectDataStreamed()
{
    return;
}

// to be run from python side
void runDAQ(char *outputFile,
            int chATrigger, uint chAVRange, uint chAWfSamples,
            int chBTrigger, uint chBVRange, uint chBWfSamples,
            int chCTrigger, uint chCVRange, uint chCWfSamples,
            int chDTrigger, uint chDVRange, uint chDWfSamples,
            int auxTrigger, uint timebase,
            uint numWaveforms, uint samplesPreTrigger, int8_t *serial = NULL
            )
{
    UNIT *unit;
    findUnit(unit, serial);
    dataCollectionConfig dcc(unit);
    dcc.ostream = setDataOutput(outputFile);

    return;
}

void getSerials()
{
    enumUnits();
}


// class picoscopeDAQ
// {
// public:
//     void run(char *outputFile,
//              int chATrigger, uint chAVRange, uint chAWfSamples,
//              int chBTrigger, uint chBVRange, uint chBWfSamples,
//              int chCTrigger, uint chCVRange, uint chCWfSamples,
//              int chDTrigger, uint chDVRange, uint chDWfSamples,
//              int auxTrigger, uint timebase,
//              uint numWaveforms, uint samplesPreTrigger
//              )
//     {
//         runDAQ(outputFile,
//                chATrigger, chAVRange, chAWfSamples,
//                chBTrigger, chBVRange, chBWfSamples,
//                chCTrigger, chCVRange, chCWfSamples,
//                chDTrigger, chDVRange, chDWfSamples,
//                auxTrigger, timebase,
//                numWaveforms, samplesPreTrigger
//                );
//         return;
//     }
// };

PYBIND11_MODULE(daq, m)
{
    m.doc() = "Picoscope DAQ System";

    m.def("runDAQ", &runDAQ, py::arg("serials") = NULL);
    m.def("getSerials", &getSerials);

    // py::class_<picoscopeDAQ>(m, "picoscopeDAQ")
    // .def(py::init())
    // .def("run", &picoscopeDAQ::run, py::call_guard<py::gil_scoped_release>())
    // ;
}

