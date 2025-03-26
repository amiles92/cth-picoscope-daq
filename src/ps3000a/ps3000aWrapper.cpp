#include <stdio.h>
#include <stdint.h>
#include <stdexcept>
#include <assert.h>
#include <bitset>
#include <vector>
#include <sstream>
#include <iostream>
#include <string>
#include <chrono>

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
#endif

#include <libps3000a/ps3000aApi.h>
#include <libps3000a/PicoStatus.h>
#include "ps3000a/ps3000aWrapper.h"

using namespace std;

BOOL	g_ready = FALSE;
uint8_t g_multiReady = 0;

void set_info(UNIT * unit)
{
	int8_t description [11][25]= { "Driver Version",
		"USB Version",
		"Hardware Version",
		"Variant Info",
		"Serial",
		"Cal Date",
		"Kernel Version",
		"Digital HW Version",
		"Analogue HW Version",
		"Firmware 1",
		"Firmware 2"};

	int16_t i = 0;
	int16_t requiredSize = 0;
	int8_t line [80];
	int32_t variant;
	PICO_STATUS status = PICO_OK;

	// Variables used for arbitrary waveform parameters
	int16_t			minArbitraryWaveformValue = 0;
	int16_t			maxArbitraryWaveformValue = 0;
	uint32_t		minArbitraryWaveformSize = 0;
	uint32_t		maxArbitraryWaveformSize = 0;

	//Initialise default unit properties and change when required
	unit->firstRange = (PS_RANGE) PS3000A_50MV;
	unit->lastRange = (PS_RANGE) PS3000A_20V;
	unit->channelCount = 4;
	unit->digitalPortCount = 2;

	if (unit->handle) 
	{
		printf("Device information:-\n\n");

		for (i = 0; i < 11; i++) 
		{
			status = ps3000aGetUnitInfo(unit->handle, line, sizeof (line), &requiredSize, i);

			// info = 3 - PICO_VARIANT_INFO
			if (i == PICO_VARIANT_INFO) 
			{
				variant = atoi((const char *) line);
				memcpy(&(unit->modelString), line, sizeof(unit->modelString)==5?5:sizeof(unit->modelString));

				unit->channelCount = (int16_t)line[1];
				unit->channelCount = unit->channelCount - 48; // Subtract ASCII 0 (48)

				// All models have 2 digital ports (MSO)
					unit->digitalPortCount = 2;
				
			}
			else if (i == PICO_BATCH_AND_SERIAL)	// info = 4 - PICO_BATCH_AND_SERIAL
			{
				memcpy(&(unit->serial), line, requiredSize);
			}

			printf("%s: %s\n", description[i], line);
		}
		printf("\n");
	}
}

/****************************************************************************
* SetDefaults - restore default settings
****************************************************************************/
void SetDefaults(UNIT *unit)
{
	PICO_STATUS status;
	int32_t i;

	for (i = PICO_CHANNEL_A; i < unit->channelCount; i++) // reset channels to most recent settings
	{
		if (unit->channelSettings[i].enabled)
		{
			status = ps3000aSetChannel(unit->handle, (PS3000A_CHANNEL) i, TRUE, PS3000A_AC,
				(PS3000A_RANGE) unit->channelSettings[i].range, 0);
		} else {
			status = ps3000aSetChannel(unit->handle, (PS3000A_CHANNEL) i, FALSE, PS3000A_AC,
				(PS3000A_RANGE) 0, 0);
		}
	}

}

/****************************************************************************
* Select input voltage ranges for channels
****************************************************************************/
void SetVoltages(UNIT *unit, int16_t ranges[4])
{
	int32_t ch;
	int32_t count = 0;

	for (ch = 0; ch < unit->channelCount; ch++) 
	{
		unit->channelSettings[ch].range = ranges[ch];

		if (unit->channelSettings[ch].range != 99) 
		{
			unit->channelSettings[ch].enabled = TRUE;
			count++;
		} 
		else 
		{
			unit->channelSettings[ch].enabled = FALSE;
		}
	}

	if (count == 0) {
		printf("No active channels\n");
		throw invalid_argument("No active channels");
	}

	SetDefaults(unit);	// Put these changes into effect
}


void SetTimebase(UNIT *unit, uint8_t timebase, uint16_t maxChSamples)
{
    // Not foolproof, should check vs range of selected picoscope
    assert(timebase <= 5 && timebase >= 0);

    float timeInterval = 0;
	int32_t maxSamples;
	int32_t noSamples = maxChSamples;
	uint32_t timebase32 = timebase;
	PICO_STATUS status;

	
    status = ps3000aGetTimebase2(unit->handle, timebase32, noSamples, 
			&timeInterval, 0, &maxSamples, 0);

    if(status != PICO_OK)
    {
		printf("Pico status: %d", status);
        throw invalid_argument("Invalid timebase given for this unit");
    }
	

	// bool oversample = TRUE;

	return;

}

vector<vector<void*>> SetDataBuffers(UNIT *unit, bitset<4> activeChannels, 
	vector<uint16_t> samplesPostPerChannel, int16_t samplesPreTrigger, 
	uint32_t numWaveforms, uint16_t maxPostSamples, bool bit8Buffers)
{//Using rapid block mode only for now
	vector<vector<void*>> outBuffers(activeChannels.count());

	int32_t nMaxSamples = activeChannels.count() * maxPostSamples;
	int32_t picoMaxSamples;
	uint8_t activeCh = 0;

	PICO_STATUS status = ps3000aMemorySegments(unit->handle, numWaveforms, &picoMaxSamples);
	if (status != PICO_OK)
	{
		printf("PICO status code: %d\n", status);
		throw runtime_error("Improperly set pico memory segments");
	}
	if (picoMaxSamples < nMaxSamples)
	{
		printf("\n\n\nFEWER SAMPLES PER WAVEFORM THAN NUMBER REQUESTED\n");
		printf("The program will still run with samples truncated\n"); //XXX: everything here
		printf("Pester Alex to add partial collections if it becomes a problem\n\n\n");		
	}
	ps3000aSetNoOfCaptures(unit->handle, numWaveforms);

	int active = 0;
	for (int i = 0; i < 4; i++)
	{
		if (!activeChannels.test(i)) {continue;}

		int32_t chSamples = samplesPreTrigger + samplesPostPerChannel.at(i);
		outBuffers.at(active) = vector<void*>(numWaveforms);

		for (int j = 0; j < numWaveforms; j++)
		{
			outBuffers.at(active).at(j) = calloc(chSamples, sizeof(int16_t));
			if (outBuffers.at(active).at(j) == NULL)
			{
				printf("Memory allocation failed: Ch %d, Wf %d\n", active, j);
			}
			ps3000aSetDataBuffer(unit->handle, (PS3000A_CHANNEL) (i),
				(int16_t*) outBuffers.at(active).at(j), chSamples, j, PS3000A_RATIO_MODE_NONE);
		}
		active++;
	}

	return outBuffers;
}

void SetSimpleChannelTrigger(UNIT *unit, int16_t threshold, 
		PS_THRESHOLD_DIRECTION dir, PS_CHANNEL ch)
{
	PICO_STATUS status = ps3000aSetSimpleTrigger(unit->handle, 1, (PS3000A_CHANNEL) ch, threshold, 
		(PS3000A_THRESHOLD_DIRECTION) dir, 0, 0);
	return;
}

void SetSimpleAuxTrigger(UNIT *unit)
{
	std::cout << "No Aux trigger available" << std::endl;
	CloseDevice(unit);
	throw "No Aux trigger available";
}

void disableTrigger(UNIT *unit)
{
	ps3000aSetSimpleTrigger(unit->handle, 0, PS3000A_CHANNEL_A, 0, PS3000A_RISING, 0, 0);
	return;
}

void SetDaqDelay(UNIT *unit, int16_t delay)
{
	assert(delay < 0);

	PICO_STATUS status;
	uint32_t u_delay = delay * -1;

	printf("u_delay: %i\n", (int) u_delay);

	status = ps3000aSetTriggerDelay(unit->handle, u_delay);
	printf("Status from set delay: %x\n", status);
	if (status != PICO_OK)
	{
		throw runtime_error("Delay failed");
	}
	return;
}

void SetMultiTriggerSettings(UNIT *unit, bitset<5> triggers, vector<int8_t> thresholds,
		int8_t auxThreshold)
{   // NOTE: Ignores external channel
	assert(thresholds.size() == 4);

	printf("I haven't made this function yet: SetMultiTriggerSettings\n");
	throw runtime_error("I haven't made this function yet: SetMultiTriggerSettings");

	// RECAP FROM LAST TIME: WRITING SIMPLE ONE TRIGGER AND COMBINED SIMPLE TRIGGERS
	// USING ARRAY OF PS6000_TRIGGER_CONDITIONS STRCTURES
	return;
}

void SetTriggers(UNIT *unit, bitset<5> triggers, vector<int16_t> chThreshold, int16_t auxThreshold)
{
	assert(chThreshold.size() == 4);
	
	if (triggers.count() == 0)
	{
		printf("No set triggers\n");
		throw runtime_error("No set triggers");
	}
	if (triggers.count() == 1)
	{
		for (int i = 0; i < 4; i++)
		{
			if (triggers.test(i + 1))
			{
				printf("Setting channel trigger\n");
				SetSimpleChannelTrigger(unit, chThreshold.at(i),
					((chThreshold.at(i) >= 0) ? PS_RISING : PS_FALLING), (PS_CHANNEL) i);
				return;
			}
		}
		if (triggers.test(0))
		{
			printf("Setting aux trigger\n");
			SetSimpleAuxTrigger(unit);
		}
	}
	else
	{
		printf("multi trig (currently unimplemented)\n");
		// SetMultiTriggerSettings(unit, triggers, chThreshold, auxThreshold);
	}
	return;
}

int32_t _getch()
{
        struct termios oldt, newt;
        int32_t ch;
        int32_t bytesWaiting;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~( ICANON | ECHO );
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        setbuf(stdin, NULL);
        do {
                ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
                if (bytesWaiting)
                        getchar();
        } while (bytesWaiting);

        ch = getchar();

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
}

int32_t _kbhit()
{
        struct termios oldt, newt;
        int32_t bytesWaiting;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~( ICANON | ECHO );
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        setbuf(stdin, NULL);
        ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return bytesWaiting;
}

void StartRapidBlock(UNIT *unit, int16_t preTrigger, uint16_t postTriggerMax,
	uint8_t timebase, uint32_t numWaveforms)
{
	PICO_STATUS status;
	int32_t timeIndisposed;
	uint32_t nCompletedCaptures;
	uint32_t timebase32 = timebase;
	int32_t preTrigger32 = max((int16_t) 0, preTrigger);
	uint32_t postTriggerMax32 = postTriggerMax;

	printf("\n\nStarting DAQ\n\n");

	chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	ps3000aRunBlock(unit->handle, preTrigger32, postTriggerMax32, timebase32, 0,
			&timeIndisposed, 0, CallBackBlock, NULL);
	g_ready = FALSE;

	while (!g_ready && !_kbhit()) // XXX: Should change to only cancel if getch == ctrl+c
	{
		usleep(0);
	}

	if (!g_ready)
	{
		_getch();
		status = ps3000aStop(unit->handle);
		status = ps3000aGetNoOfCaptures(unit->handle, &nCompletedCaptures);
		CloseDevice(unit);

		printf("Rapid capture aborted. %d complete blocks were captured\n", nCompletedCaptures);
		printf("Early abort writeout not yet supported\n");

		throw runtime_error("aborted, need to implement early cancellation writeout");

	}

	chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	int time = chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

	printf("Time taken: %d ms\n", time);
	printf("Trigger rate: %f Hz\n", (double) numWaveforms / time * 1.0e3);

	status = ps3000aGetNoOfCaptures(unit->handle, &nCompletedCaptures);


	uint32_t nSamples = preTrigger + postTriggerMax;

	// Get data
	status = ps3000aGetValuesBulk(unit->handle, &nSamples, 0, numWaveforms - 1, 
			1, PS3000A_RATIO_MODE_NONE, NULL); // XXX: Fix this for different sample lengths

	// Stop
	status = ps3000aStop(unit->handle);

	status = ps3000aMemorySegments(unit->handle, 1, NULL);
	status = ps3000aSetNoOfCaptures(unit->handle, 1);

	return;
}

void StartMultiRapidBlock(vector<UNIT *> vecUnit, vector<int16_t> vecPreTrigger,
	vector<uint16_t> vecPostTriggerMax, vector<uint8_t> vecTimebase,
	vector<uint32_t> vecNumWaveforms)
{
	uint8_t len = vecUnit.size();
	PICO_STATUS status;
	vector<int32_t> vecTimeIndisposed(len);
	vector<uint32_t> vecNCompletedCaptures(len);
	vector<uint32_t> vecTimebase32(len);
	vector<int32_t> vecPreTrigger32(len);
	vector<uint32_t> vecPostTriggerMax32(len);

	g_multiReady = 0;

	for (int i = 0; i < len; i++)
	{
		vecTimebase32.at(i) = vecTimebase.at(i);
		vecPreTrigger32.at(i) = max((int16_t) 0, vecPreTrigger.at(i));
		vecPostTriggerMax32.at(i) = vecPostTriggerMax.at(i);
	}

	printf("\n\nStarting DAQ\n\n");

	chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	for (int i = 0; i < len; i++)
	{
		intptr_t iPt = i;
		ps3000aRunBlock(vecUnit.at(i)->handle, vecPreTrigger32.at(i), 
			vecPostTriggerMax32.at(i), vecTimebase32.at(i), 0,
			&vecTimeIndisposed.at(i), 0, MultiCallBackBlock, (void*) iPt);
	}
	
	while (!(len <= __builtin_popcount(g_multiReady)) && !_kbhit() &&
		(5 > chrono::duration_cast<std::chrono::seconds>
		(std::chrono::steady_clock::now() - begin).count())) // XXX: Should change to only cancel if getch == ctrl+c
	{
		usleep(0);
	}

	if (!(len <= __builtin_popcount(g_multiReady)))
	{
		_getch();
		BOOL contAnyways = TRUE;
		for (int i = 0; i < len; i++)
		{
			status = ps3000aStop(vecUnit.at(i)->handle);
			status = ps3000aGetNoOfCaptures(vecUnit.at(i)->handle, &vecNCompletedCaptures.at(i));
			printf("Rapid capture aborted.\n");
			printf("%s: %d complete blocks were captured\n", vecUnit.at(i)->serial, 
			(int) vecNCompletedCaptures.at(i));
			if (vecNCompletedCaptures.at(i) < vecNumWaveforms.at(i))
			{
				contAnyways = FALSE;
			}
		}
		if (!contAnyways)
		{
			printf("Early abort writeout not yet supported\n");

			throw "aborted, need to implement early cancellation writeout";
		}
	}

	chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	int time = chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

	printf("Time taken: %d ms\n", time);
	for (int i = 0; i < len; i++)
	{
		printf("%s: Trigger rate: %f Hz\n", vecUnit.at(i)->serial,
			(double) vecNumWaveforms.at(i) / time * 1.0e3);

		status = ps3000aGetNoOfCaptures(vecUnit.at(i)->handle, &vecNCompletedCaptures.at(i));


		uint32_t nSamples = vecPreTrigger.at(i) + vecPostTriggerMax.at(i);

		// Get data
		status = ps3000aGetValuesBulk(vecUnit.at(i)->handle, &nSamples, 0, 
				vecNumWaveforms.at(i) - 1, 1, PS3000A_RATIO_MODE_NONE, NULL);

		// Stop
		status = ps3000aStop(vecUnit.at(i)->handle);

		status = ps3000aMemorySegments(vecUnit.at(i)->handle, 1, NULL);
		status = ps3000aSetNoOfCaptures(vecUnit.at(i)->handle, 1);
	}
}

PICO_STATUS OpenDevice(UNIT *unit, int8_t *serial)
{
	PICO_STATUS status;
	int16_t maxADCValue = 0;
	// int16_t minADCValue = 0;

	if (serial == NULL)
	{
		status = ps3000aOpenUnit(&unit->handle, NULL);
	}
	else
	{
		status = ps3000aOpenUnit(&unit->handle, serial);
	}

	ps3000aMaximumValue(unit->handle, &maxADCValue);
	// ps3000aMinimumValue(unit->handle, &minADCValue);

	disableTrigger(unit);

	unit->maxADCValue = maxADCValue;
	unit->openStatus = status;
	unit->complete = 1;

	return status;
}

PICO_STATUS HandleDevice(UNIT * unit)
{
	printf("Handle: %d\n", unit->handle);
	if (unit->openStatus != PICO_OK)
	{
		printf("Unable to open device\n");
		printf("Error code : 0x%08x\n", (uint32_t)unit->openStatus);
		throw;
	}

	printf("Device opened successfully\n\n");
	set_info(unit);

	SetDefaults(unit);

	disableTrigger(unit);

	return unit->openStatus;
}

void CloseDevice(UNIT *unit)
{
	ps3000aCloseUnit(unit->handle);
}

void findUnit(UNIT *unit, int8_t *serial)
{
	if (serial == NULL)
	{
		findUnit(unit);
		return;
	}

	PICO_STATUS status = OpenDevice(unit, serial);

	if (status == PICO_OK || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT)
	{
		status = HandleDevice(unit);

		if (status != PICO_OK)
		{
			printf("Picoscope devices open failed, error code 0x%x\n",(uint32_t)status);
		}

		return;
	}
}

void findUnit(UNIT *unit)
{
#define MAX_PICO_DEVICES 64
#define TIMED_LOOP_STEP 500

	int8_t ch;
	uint16_t devCount = 0, listIter = 0,	openIter = 0;
	// Device indexer -  64 chars - 64 is maximum number of picoscope devices handled by driver
	int8_t devChars[] = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#";
	PICO_STATUS status = PICO_OK;
	UNIT allUnits[MAX_PICO_DEVICES];


	do
	{
		memset(&(allUnits[devCount]),0,sizeof (UNIT));
		status = OpenDevice(&(allUnits[devCount]),NULL);
		if(status == PICO_OK || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT)
		{
			allUnits[devCount++].openStatus = status;
		}

	} while(status != PICO_NOT_FOUND);

	if (devCount == 0)
	{
		printf("Picoscope devices not found\n");
		return;
	}
	// If there is only one device, open and handle it here
	if (devCount == 1)
	{
		printf("Found one device, opening...\n\n");
		status = allUnits[0].openStatus;

		if (status == PICO_OK || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT)
		{
			status = HandleDevice(&allUnits[0]);
		}

		if (status != PICO_OK)
		{
			printf("Picoscope devices open failed, error code 0x%x\n",(uint32_t)status);
			return;
		}

		unit = &allUnits[0];
		return;
	} 
	else
	{
		printf("Too many picoscope devices found\n");
	}
}

void enumUnits(int16_t *count, char* outSerials, int16_t *serialLth)
{
	PICO_STATUS ps = ps3000aEnumerateUnits(count, (int8_t*) outSerials, serialLth);

	return;
}

/****************************************************************************
* Callback
* used by PS6000 data block collection calls, on receipt of data.
* used to set global flags etc checked by user routines
****************************************************************************/
void CallBackBlock(int16_t handle, PICO_STATUS status, void * pParameter)
{
	if (status != PICO_CANCELLED)
	{
		if (status != PICO_OK)
		{
			printf("Exited with status: 0x%.8X", status);
		}
		g_ready = TRUE;
	}
}

void MultiCallBackBlock(int16_t handle, PICO_STATUS status, void *pParameter)
{
	intptr_t runId = (intptr_t) pParameter;
	if (status != PICO_CANCELLED)
	{
		if (status != PICO_OK)
		{
			printf("Picoscope %i exited with status: 0x%.8X", handle, status);
		}
		printf("Run %i has finished\n", (int) runId);
		g_multiReady |= 1 << runId;
	}
}

/****************************************************************************
* mv_to_adc
*
* Convert a millivolt value into a 16-bit ADC count
*
*  (useful for setting trigger thresholds)
****************************************************************************/
int16_t mv_to_adc(int16_t mv, int16_t rangeIndex, UNIT *unit)
{
	return (mv / psmVRange.at(rangeIndex)) * unit->maxADCValue;
}
