#include <stdio.h>
#include <stdint.h>
#include <stdexcept>
#include <assert.h>
#include <bitset>
#include <vector>
#include <sstream>
#include <iostream>

#ifdef _WIN32
#include "windows.h"
#include <conio.h>
#include "ps6000Api.h"
#else
#include <sys/types.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#ifndef PS6000API
#include <libps6000/ps6000Api.h>
#endif

#ifndef PICO_STATUS
#include <libps6000/PicoStatus.h>
#endif

#ifndef PS6000WRAPPER
#include "libps6000/ps6000Wrapper.h"
#endif

using namespace std;


uint16_t inputRanges [PS6000_MAX_RANGES] = {	10,
												20,
												50,
												100,
												200,
												500,
												1000,
												2000,
												5000,
												10000,
												20000,
												50000};
BOOL        g_ready = FALSE;
int64_t		g_times [PS6000_MAX_CHANNELS];
int16_t     g_timeUnit;
uint32_t    g_sampleCount;
uint32_t	g_startIndex;
int16_t     g_autoStopped;
int16_t     g_trig = 0;
uint32_t	g_trigAt = 0;
int16_t		g_overflow;

void set_info(UNIT *unit)
{
	int16_t i = 0;
	int16_t r = 20;
	int8_t line [20];
    char* charLine[20];
	int32_t variant;

	int8_t description [11][25]= { "Driver Version",
		"USB Version",
		"Hardware Version",
		"Variant Info",
		"Serial",
		"Cal Date",
		"Kernel",
		"Digital H/W",
		"Analogue H/W",
		"Firmware 1",
		"Firmware 2"};

	if (unit->handle) 
	{
		for (i = 0; i < 11; i++) 
		{
			ps6000GetUnitInfo(unit->handle, line, sizeof (line), &r, i);
		
			if (i == 3) 
			{
				// info = 3 - PICO_VARIANT_INFO
		
				variant = atoi((char *)line);
				memcpy(&(unit->modelString),line,sizeof(unit->modelString)==7?7:sizeof(unit->modelString));
		
				//To identify A or B model variants.....
				if (strlen((char *)line) == 4)							// standard, not A, B, C or D, convert model number into hex i.e 6402 -> 0x6402
				{
					variant += 0x4B00;
				}
				else
				{
					if (strlen((char *) line) == 5)						// A, B, C or D variant unit 
					{
						line[4] = toupper(line[4]);

						switch(line[4])
						{
							case 65: // i.e 6402A -> 0xA402
								variant += 0x8B00;
								break;
							case 66: // i.e 6402B -> 0xB402
								variant += 0x9B00;
								break;
							case 67: // i.e 6402C -> 0xC402
								variant += 0xAB00;
								break;
							case 68: // i.e 6402D -> 0xD402
								variant += 0xBB00;
								break;
							default:
								break;
						}
					}
				}
			}

			if (i == 4) 
			{
				// info = 4 - PICO_BATCH_AND_SERIAL
				ps6000GetUnitInfo(unit->handle, unit->serial, sizeof (unit->serial), &r, PICO_BATCH_AND_SERIAL);
			}

			printf("%s: %s\n", description[i], line);
		}

		switch (variant)
		{
			case MODEL_PS6402:
				unit->model		= MODEL_PS6402;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6402A:
				unit->model		= MODEL_PS6402A;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = FALSE;
				unit->awgBufferSize = 0;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6402B:
				unit->model		= MODEL_PS6402B;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6402C:
				unit->model		= MODEL_PS6402C;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = FALSE;
				unit->awgBufferSize = 0;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6402D:
				unit->model		= MODEL_PS6402D;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = PS640X_C_D_MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6403:
				unit->model		= MODEL_PS6403;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6403A:
				unit->model		= MODEL_PS6403;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = FALSE;
				unit->awgBufferSize = 0;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6403B:
				unit->model		= MODEL_PS6403B;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6403C:
				unit->model		= MODEL_PS6403C;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = FALSE;
				unit->awgBufferSize = 0;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6403D:
				unit->model		= MODEL_PS6403D;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = PS640X_C_D_MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6404:
				unit->model		= MODEL_PS6404;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6404A:
				unit->model		= MODEL_PS6404;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = FALSE;
				unit->awgBufferSize = 0;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6404B:
				unit->model		= MODEL_PS6404B;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6404C:
				unit->model		= MODEL_PS6404C;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = 0;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6404D:
				unit->model		= MODEL_PS6404D;
				unit->firstRange = PS6000_50MV;
				unit->lastRange = PS6000_20V;
				unit->channelCount = 4;
				unit->AWG = TRUE;
				unit->awgBufferSize = PS640X_C_D_MAX_SIG_GEN_BUFFER_SIZE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++)
				{
					unit->channelSettings[i].range = PS6000_5V;
					unit->channelSettings[i].DCcoupled = PS6000_DC_1M;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			case MODEL_PS6407:
				unit->model		= MODEL_PS6407;
				unit->firstRange = PS6000_100MV;
				unit->lastRange = PS6000_100MV;
				unit->channelCount = 4;
				unit->AWG = TRUE;

				for (i = 0; i < PS6000_MAX_CHANNELS; i++) 
				{
					unit->channelSettings[i].range = PS6000_100MV;
					unit->channelSettings[i].DCcoupled = PS6000_DC_50R;
					unit->channelSettings[i].enabled = TRUE;
				}
				break;

			default:
				break;
		}
	}
}

/****************************************************************************
* SetDefaults - restore default settings
****************************************************************************/
void SetDefaults(UNIT *unit)
{
	PICO_STATUS status;
	int32_t i;

	status = ps6000SetEts(unit->handle, PS6000_ETS_OFF, 0, 0, NULL); // Turn off ETS

	for (i = 0; i < unit->channelCount; i++) // reset channels to most recent settings
	{
		status = ps6000SetChannel(unit->handle, (PS6000_CHANNEL) (PS6000_CHANNEL_A + i),
			unit->channelSettings[PS6000_CHANNEL_A + i].enabled,
			(PS6000_COUPLING)unit->channelSettings[PS6000_CHANNEL_A + i].DCcoupled,
			(PS6000_RANGE)unit->channelSettings[PS6000_CHANNEL_A + i].range, 0, PS6000_BW_FULL);
	}

}

/****************************************************************************
* Select input voltage ranges for channels
****************************************************************************/
void SetVoltages(UNIT *unit, uint16_t ranges[4])
{
	int32_t i, ch;
	int32_t count = 0;
	int16_t loop;


	for (ch = 0; ch < unit->channelCount; ch++) 
	{
		assert(unit->firstRange <= ranges[ch] <= unit->lastRange);

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

	if (count == 0) {throw invalid_argument("No active channels");}


	SetDefaults(unit);	// Put these changes into effect
}


void SetTimebase(UNIT *unit, uint32_t timebase, uint16_t maxChSamples)
{
    // Not foolproof, should check vs range of selected picoscope
    assert(timebase <= 5 && timebase >= 0);

    float timeInterval = 0.00f;
	uint32_t maxSamples;
	PICO_STATUS status;

	
    status = ps6000GetTimebase2(unit->handle, timebase, maxChSamples, &timeInterval, 1, &maxSamples, 0);

    if(status == PICO_INVALID_TIMEBASE)
    {
        throw invalid_argument("Invalid timebase given for this unit");
    }
	

	// bool oversample = TRUE;

	return;

}

void SetSimpleTriggerSettings(UNIT *unit, uint16_t threshold, 
		PS6000_THRESHOLD_DIRECTION dir, PS6000_CHANNEL ch)
{

	return;
}

void disableTrigger(UNIT *unit)
{
	for (int i = 0; i < PS6000_MAX_TRIGGER_SOURCES; i++)
	{
		ps6000SetSimpleTrigger(unit->handle, 0, (PS6000_CHANNEL) (PS6000_CHANNEL_A + i), 0, PS6000_NONE, 0, 0);
	}
	return;
}

void SetTriggerSettings(UNIT *unit, uint16_t *thresholds[5], 
		PS6000_THRESHOLD_DIRECTION dirs[5])
{   // NOTE: For combined simple triggers, ignores external channel

	// RECAP FROM LAST TIME: WRITING SIMPLE ONE TRIGGER AND COMBINED SIMPLE TRIGGERS
	// USING ARRAY OF PS6000_TRIGGER_CONDITIONS STRCTURES
	return;
}

PICO_STATUS OpenDevice(UNIT *unit, int8_t *serial)
{
	PICO_STATUS status;

	if (serial == NULL)
	{
		status = ps6000OpenUnit(&unit->handle, NULL);
	}
	else
	{
		status = ps6000OpenUnit(&unit->handle, serial);
	}

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
	// setup device info - unless it's set already
	if (unit->model == MODEL_NONE)
	{
		set_info(unit);
	}


	SetDefaults(unit);

	/* Trigger disabled	*/
	disableTrigger(unit);

	return unit->openStatus;
}

void CloseDevice(UNIT *unit)
{
	ps6000CloseUnit(unit->handle);
}

void findUnit(UNIT *unit, int8_t *serial)
{
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
		printf("Too many picoscope devices found");
	}
}

void SetTriggers(bitset<5> triggers, vector<uint8_t> chThreshold, uint8_t auxThreshold)
{
	if (triggers.count() == 0)
	{
		throw "No set triggers";
	}
	if (triggers.count() == 1)
	{
		
	}
	return;
}

void enumUnits()
{
	int16_t *count;
	int16_t *serialLth = (int16_t*)128;
	int8_t  serials[*serialLth];

	PICO_STATUS ps = ps6000EnumerateUnits(count, serials, serialLth);

	stringstream ss;
	ss << (char*) serials;
	string line;

	cout << "Serial Numbers:" << endl;
	while (getline(ss, line, ','))
	{
		cout << line << endl;
	}
	return;
}



/****************************************************************************
* mv_to_adc
*
* Convert a millivolt value into a 16-bit ADC count
*
*  (useful for setting trigger thresholds)
****************************************************************************/
int16_t mv_to_adc(int16_t mv, int16_t ch)
{
	return (mv * PS6000_MAX_VALUE) / inputRanges[ch];
}