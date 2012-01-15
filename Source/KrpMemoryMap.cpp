//
// KartRacingProTelemSample.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

/*
X+ is right, Y+ is top and Z+ is forward.
*/

typedef struct
{
	char m_szDriverName[100];
	char m_szKartName[100];
	int m_iDriveType;												/* 0 = direct; 1 = clutch; 2 = shifter */
	int m_iNumberOfGears;
	float m_fMaxRPM;
	float m_fLimiter;
	float m_fShiftRPM;
	float m_fEngineOptTemperature;									/* degrees Celsius */
	float m_afEngineTemperatureAlarm[2];							/* degrees Celsius. Lower and upper limits */
	float m_fMaxFuel;												/* liters */
	char m_szCategory[100];
	char m_szDash[100];
	char m_szTrackName[100];
	float m_fTrackLength;											/* centerline length. meters */
} SPluginsKartEvent_t;

typedef struct
{
	int m_iSession;													/* 0 = testing; 1 = practice; 2 = qualify; 3 = pre-final; 4 = final */
	int m_iConditions;												/* 0 = sunny; 1 = cloudy; 2 = rainy */
	float m_fAirTemperature;										/* degrees Celsius */
	float m_fTrackTemperature;										/* degrees Celsius */
} SPluginsKartSession_t;

typedef struct
{
	float m_fRPM;													/* engine rpm */
	float m_fEngineTemperature;										/* degrees Celsius */
	float m_fWaterTemperature;										/* degrees Celsius */
	int m_iGear;													/* 0 = Neutral */
	float m_fFuel;													/* liters */
	float m_fSpeedometer;											/* meters/second */
	float m_fPosX,m_fPosY,m_fPosZ;									/* world position of a reference point attached to chassis ( not CG ) */
	float m_fVelocityX,m_fVelocityY,m_fVelocityZ;					/* velocity of CG in world coordinates. meters/second */
	float m_fAccelerationX,m_fAccelerationY,m_fAccelerationZ;		/* acceleration of CG local to chassis rotation, expressed in G ( 9.81 m/s2 ) and averaged over the latest 10ms */
	float m_aafRot[3][3];											/* rotation matrix of the chassis */
	float m_fYaw,m_fPitch,m_fRoll;									/* degrees, -180 to 180 */
	float m_fYawVelocity,m_fPitchVelocity,m_fRollVelocity;			/* degress / second */
	float m_fInputSteer;											/* degrees. Negative = left */
	float m_fInputThrottle;											/* 0 to 1 */
	float m_fInputBrake;											/* 0 to 1 */
	float m_fInputFrontBrakes;										/* 0 to 1 */
	float m_fInputClutch;											/* 0 to 1. 0 = Fully engaged */
	float m_afWheelSpeed[4];										/* meters/second. 0 = front-left; 1 = front-right; 2 = rear-left; 3 = rear-right */
} SPluginsKartData_t;

typedef struct
{
	int m_iLapTime;													/* milliseconds */
	int m_iBest;													/* 1 = best lap */
	int m_iLapNum;
} SPluginsKartLap_t;

typedef struct
{
	int m_iSplit;
	int m_iSplitTime;												/* milliseconds */
	int m_iBestDiff;												/* milliseconds. Difference with best lap */
} SPluginsKartSplit_t;

typedef struct
{
	UINT8 pluginVersion;
	UINT8 state;
	UINT32 eventId;
	UINT32 sessionId;
	UINT32 dataId;
	UINT32 lapId;
	UINT32 splitId;
	SPluginsKartEvent_t event;
	SPluginsKartSession_t session;
	float onTrackTime;
	float trackPositionPct;
	SPluginsKartData_t data;
	SPluginsKartLap_t lap;
	SPluginsKartSplit_t split;
} SPluginsKartCombined_t;

void UpdateState(int state);
int IncrementId(UINT32 id);

const int mapOffsetPluginVersion = 0;
const int mapOffsetState = 1;
const int mapOffsetEventId = 2;
const int mapOffsetSessionId = 6;
const int mapOffsetDataId = 10;
const int mapOffsetLapId = 14;
const int mapOffsetSplitId = 18;
const int mapOffsetEvent = 22;
const int mapOffsetSession = 562;
const int mapOffsetOnTrackTime = 578;
const int mapOffsetTrackPositionPct = 582;
const int mapOffsetData = 586;
const int mapOffsetLap = 742;
const int mapOffsetSplit = 754;

extern "C" __declspec(dllexport) char *GetModID()
{
	return "krp";
}

extern "C" __declspec(dllexport) int Version()
{
	return 6;
}

FILE *g_hTestLog;

TCHAR szName[] = TEXT("Local\\KartRacingProMemoryMap");
BYTE *pBuf;
HANDLE hMapFile;
SPluginsKartCombined_t stCombined;
void* psCombined;

const UINT8 STATE_NOT_RUNNING = 0;
const UINT8 STATE_MAIN_MENU = 1;
const UINT8 STATE_IN_GARAGE = 2;
const UINT8 STATE_ON_TRACK = 3;
const UINT8 STATE_ON_TRACK_PAUSED = 4;

/* called when software is started */
extern "C" __declspec(dllexport) int Startup(char *_szSavePath)
{
	int BUF_SIZE = sizeof(SPluginsKartCombined_t);
	psCombined = &stCombined;
	hMapFile = CreateFileMapping(
					INVALID_HANDLE_VALUE,    // use paging file
					NULL,                    // default security
					PAGE_READWRITE,          // read/write access
					0,                       // maximum object size (high-order DWORD)
					BUF_SIZE,                // maximum object size (low-order DWORD)
					szName);                 // name of mapping object

	if (hMapFile == NULL)
	{
		return -1;
	}
	else
	{
		pBuf = (BYTE *) MapViewOfFile(hMapFile,   // handle to map object
							FILE_MAP_ALL_ACCESS, // read/write permission
							0,
							0,
							BUF_SIZE);

		if (pBuf == NULL)
		{
			CloseHandle(hMapFile);
			return -1;
		}
		else
		{
			UINT8 version = 1;
			CopyMemory((PVOID)(pBuf+mapOffsetPluginVersion), &version, sizeof(version));
			UpdateState(STATE_MAIN_MENU);
		}
	}

	/*
	return value is requested rate
	0 = 100hz; 1 = 50hz; 2 = 20hz; 3 = 10hz; -1 = disable
	*/
	return 0;
}

void UpdateState(int state) {
	stCombined.state = state;
	CopyMemory((PVOID)(pBuf+mapOffsetState), &stCombined.state, sizeof(stCombined.state));
}

void UpdateEvent(SPluginsKartEvent_t* psEventData) {
	stCombined.eventId = IncrementId(stCombined.eventId);
	CopyMemory((PVOID)(pBuf+mapOffsetEventId), &stCombined.eventId, sizeof(stCombined.eventId));

	stCombined.event = *psEventData;
	CopyMemory((PVOID)(pBuf+mapOffsetEvent), psEventData, sizeof(SPluginsKartEvent_t));
}

void UpdateSession(SPluginsKartSession_t* psSessionData) {
	stCombined.sessionId = IncrementId(stCombined.sessionId);
	CopyMemory((PVOID)(pBuf+mapOffsetSessionId), &stCombined.sessionId, sizeof(stCombined.sessionId));

	stCombined.session = *psSessionData;
	CopyMemory((PVOID)(pBuf+mapOffsetSession), psSessionData, sizeof(SPluginsKartSession_t));
}

void UpdateLap(SPluginsKartLap_t* psLapData) {
	stCombined.lapId = IncrementId(stCombined.lapId);
	CopyMemory((PVOID)(pBuf+mapOffsetLapId), &stCombined.lapId, sizeof(stCombined.lapId));

	stCombined.lap = *psLapData;
	CopyMemory((PVOID)(pBuf+mapOffsetLap), psLapData, sizeof(SPluginsKartLap_t));
}

void UpdateSplit(SPluginsKartSplit_t* psSplitData) {
	stCombined.splitId = IncrementId(stCombined.splitId);
	CopyMemory((PVOID)(pBuf+mapOffsetSplitId), &stCombined.splitId, sizeof(stCombined.splitId));

	stCombined.split = *psSplitData;
	CopyMemory((PVOID)(pBuf+mapOffsetSplit), psSplitData, sizeof(SPluginsKartSplit_t));
}

void UpdateTelemetry(float time, float pos, SPluginsKartData_t* psKartData) {
	stCombined.dataId = IncrementId(stCombined.dataId);
	CopyMemory((PVOID)(pBuf+mapOffsetDataId), &stCombined.dataId, sizeof(stCombined.dataId));

	stCombined.onTrackTime = time;
	CopyMemory((PVOID)(pBuf+mapOffsetOnTrackTime), &stCombined.onTrackTime, 4);
	stCombined.trackPositionPct = pos;
	CopyMemory((PVOID)(pBuf+mapOffsetTrackPositionPct), &stCombined.trackPositionPct, 4);

	stCombined.data = *psKartData;
	CopyMemory((PVOID)(pBuf+mapOffsetData), psKartData, sizeof(SPluginsKartData_t));
}

int IncrementId(UINT32 id) {
	if (id == UINT_MAX) id = 0;
	else id++;
	return id;
}

/* called when software is closed */
extern "C" __declspec(dllexport) void Shutdown()
{
	if (pBuf)
	{
		UpdateState(STATE_NOT_RUNNING);
		UnmapViewOfFile(pBuf);
		CloseHandle(hMapFile);
	}
}

/* called when event is initialized */
extern "C" __declspec(dllexport) void EventInit(void *_pData, int _iDataSize)
{
	SPluginsKartEvent_t *psEventData;

	psEventData = (SPluginsKartEvent_t*)_pData;
	
	if (pBuf)
	{
		UpdateState(STATE_IN_GARAGE);
		UpdateEvent(psEventData);
	}
}

/* called when kart goes to track */
extern "C" __declspec(dllexport) void RunInit(void *_pData, int _iDataSize)
{
	SPluginsKartSession_t *psSessionData;

	psSessionData = (SPluginsKartSession_t*)_pData;

	if (pBuf)
	{
		UpdateState(STATE_ON_TRACK);
		UpdateSession(psSessionData);
	}
}

/* called when kart leaves the track */
extern "C" __declspec(dllexport) void RunDeinit()
{
	if (pBuf) UpdateState(STATE_IN_GARAGE);
}

/* called when simulation is started / resumed */
extern "C" __declspec(dllexport) void RunStart()
{
	if (pBuf) UpdateState(STATE_ON_TRACK);
}

/* called when simulation is paused */
extern "C" __declspec(dllexport) void RunStop()
{
	if (pBuf) UpdateState(STATE_ON_TRACK_PAUSED);
}

/* called when a new lap is recorded */
extern "C" __declspec(dllexport) void RunLap(void *_pData, int _iDataSize)
{
	SPluginsKartLap_t *psLapData;

	psLapData = (SPluginsKartLap_t*)_pData;

	if (pBuf) UpdateLap(psLapData);
}

/* called when a split is crossed */
extern "C" __declspec(dllexport) void RunSplit(void *_pData, int _iDataSize)
{
	SPluginsKartSplit_t *psSplitData;

	psSplitData = (SPluginsKartSplit_t*)_pData;

	if (pBuf) UpdateSplit(psSplitData);
}

/* _fTime is the ontrack time, in seconds. _fPos is the position on centerline, from 0 to 1 */
extern "C" __declspec(dllexport) void RunTelemetry(void *_pData, int _iDataSize, float _fTime, float _fPos)
{
	SPluginsKartData_t *psKartData;

	psKartData = (SPluginsKartData_t*)_pData;

	if (pBuf) UpdateTelemetry(_fTime, _fPos, psKartData);
}
