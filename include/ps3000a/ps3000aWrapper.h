#ifndef PS3000AWRAPPER
#define PS3000AWRAPPER
#include <stdio.h>
#include <stdint.h>
#include <stdexcept>
#include <vector>
#include <bitset>
#include <string>

#ifdef _WIN32
#include "windows.h"
#include <conio.h>
#include "ps3000aApi.h"
#else
#include <sys/types.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <libps3000a/ps3000aApi.h>
#endif

#include <libps3000a/PicoStatus.h>

// #define PS3000A_MAX_CHANNELS 4

typedef enum enPSChannel
{
  PS_CHANNEL_A,
  PS_CHANNEL_B,
  PS_CHANNEL_C,
  PS_CHANNEL_D,
  PS_EXTERNAL,
  PS_MAX_CHANNELS = PS_EXTERNAL,
  PS_TRIGGER_AUX,
  PS_MAX_TRIGGER_SOURCES
} PS_CHANNEL;

typedef enum enPSRange
{
  PS_10MV,
  PS_20MV,
  PS_50MV,
  PS_100MV,
  PS_200MV,
  PS_500MV,
  PS_1V,
  PS_2V,
  PS_5V,
  PS_10V,
  PS_20V,
  PS_50V,
  PS_MAX_RANGES,
} PS_RANGE;

typedef enum enPSThresholdDirection
{
  PS_ABOVE,             //using upper threshold
  PS_BELOW,						 //using upper threshold
  PS_RISING,            // using upper threshold
  PS_FALLING,           // using upper threshold
  PS_RISING_OR_FALLING, // using both threshold
  PS_ABOVE_LOWER,       // using lower threshold
  PS_BELOW_LOWER,       // using lower threshold
  PS_RISING_LOWER,      // using lower threshold
  PS_FALLING_LOWER,     // using lower threshold
  // Windowing using both thresholds
  PS_INSIDE        = PS_ABOVE,
  PS_OUTSIDE       = PS_BELOW,
  PS_ENTER         = PS_RISING,
  PS_EXIT          = PS_FALLING,
  PS_ENTER_OR_EXIT = PS_RISING_OR_FALLING,
  PS_POSITIVE_RUNT = 9,
  PS_NEGATIVE_RUNT,
  // no trigger set
  PS_NONE = PS_RISING
} PS_THRESHOLD_DIRECTION;

typedef enum enBOOL{FALSE,TRUE} BOOL;

typedef struct
{
	int16_t DCcoupled;
	int16_t range;
	int16_t enabled;
}CHANNEL_SETTINGS;

typedef enum
{
	MODEL_NONE = 0,//this is used
//models values not used in the code
} MODEL_TYPE;

// typedef struct
// {
// 	int16_t 					handle;
// 	MODEL_TYPE					model;
// 	int8_t						modelString[8];
// 	int8_t						serial[10];
// 	int16_t						complete;
// 	int16_t						openStatus;
// 	int16_t						openProgress;
// 	PICO_CONNECT_PROBE_RANGE	firstRange;
// 	PICO_CONNECT_PROBE_RANGE	lastRange;
// 	int16_t						channelCount;
// 	int16_t						maxADCValue;
// 	CHANNEL_SETTINGS			channelSettings [PS6000A_MAX_CHANNELS];
// 	PICO_DEVICE_RESOLUTION		resolution;
// 	int16_t						digitalPortCount;
// }UNIT;

typedef struct
{
	int16_t					handle;
	int8_t					modelString[8];
    int8_t					serial[10];
	PS_RANGE				firstRange ;
	PS_RANGE				lastRange;
	int16_t					channelCount;
	int16_t					maxADCValue;
	int16_t					sigGen;
	int16_t					ETS;
	int32_t					AWGFileSize;
	CHANNEL_SETTINGS		channelSettings [PS_MAX_CHANNELS];
	int16_t					digitalPortCount;
	PICO_STATUS				openStatus;
	int8_t					complete;
} UNIT;

void set_info(UNIT *unit);

void SetDefaults(UNIT *unit);

void SetVoltages(UNIT *unit, int16_t ranges[4]);

std::vector<std::vector<void*>> SetDataBuffers(UNIT *unit, std::bitset<4> activeChannels, 
	std::vector<uint16_t> samplesPostPerChannel, int16_t samplesPreTrigger, 
	uint32_t numWaveforms, uint16_t maxPostSamples, bool bit8Buffers);

void SetTimebase(UNIT *unit, uint8_t timebase, uint16_t maxChSamples);

void SetSimpleChannelTrigger(UNIT *unit, int16_t threshold, 
		PS_THRESHOLD_DIRECTION dir, PS_CHANNEL ch);

void SetSimpleAuxTrigger(UNIT *unit);

void SetDaqDelay(UNIT *unit, int16_t delay);

void StartRapidBlock(UNIT *unit, int16_t preTrigger, uint16_t postTriggerMax,
	uint8_t timebase, uint32_t numWaveforms);

void StartMultiRapidBlock(std::vector<UNIT *> vecUnit, std::vector<int16_t> vecPreTrigger,
	std::vector<uint16_t> vecPostTriggerMax, std::vector<uint8_t> vecTimebase,
	std::vector<uint32_t> vecNumWaveforms);

void disableTrigger(UNIT *unit);

void SetMultiTriggerSettings(UNIT *unit, std::bitset<5> triggers, std::vector<int8_t> thresholds,
		int8_t auxThreshold);

void SetTriggers(UNIT *unit, std::bitset<5> triggers, std::vector<int16_t> chThreshold, int16_t auxThreshold);

PICO_STATUS OpenDevice(UNIT *unit, int8_t *serial);

PICO_STATUS HandleDevice(UNIT * unit);

void CloseDevice(UNIT *unit);

void findUnit(UNIT *unit);

void findUnit(UNIT *unit, int8_t *serial);

void enumUnits(int16_t *count, char* outSerials, int16_t *serialLth);

void CallBackBlock(int16_t handle, PICO_STATUS status, void * pParameter);

void MultiCallBackBlock(int16_t handle, PICO_STATUS status, void *pParameter);

int16_t mv_to_adc(int16_t mv, int16_t rangeIndex, UNIT *unit);

#endif
