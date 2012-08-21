/*
    BASSMIDI Driver
*/

//#define DEBUG
#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#if __DMC__
unsigned long _beginthreadex( void *security, unsigned stack_size,
		unsigned ( __stdcall *start_address )( void * ), void *arglist,
		unsigned initflag, unsigned *thrdaddr );
void _endthreadex( unsigned retval );
#endif

#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <windows.h>
#include "mmddk.h"  
#include <mmsystem.h>
#include <tchar.h>
#include <Shlwapi.h>

#define BASSDEF(f) (WINAPI *f)	// define the BASS/BASSMIDI functions as pointers
#define BASSMIDIDEF(f) (WINAPI *f)	
#define LOADBASSFUNCTION(f) *((void**)&f)=GetProcAddress(bass,#f)
#define LOADBASSMIDIFUNCTION(f) *((void**)&f)=GetProcAddress(bassmidi,#f)
#include "../bass.h"
#include "../bassmidi.h"

#include "sound_out.h"

extern "C" {
BOOL WINAPI DriverCallback( DWORD dwCallBack, DWORD dwFlags, HDRVR hdrvr, DWORD msg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2 );	
}

#define MAX_DRIVERS 1
#define MAX_CLIENTS 1 // Per driver

struct Driver_Client {
	int allocated;
	DWORD instance;
	DWORD flags;
	DWORD_PTR callback;
};

struct Driver {
	int open;
	int clientCount;
	HDRVR hdrvr;
	struct Driver_Client clients[MAX_CLIENTS];
} drivers[MAX_DRIVERS];

int driverCount;

static volatile int OpenCount = 0;
static volatile int modm_closed = 1;

static CRITICAL_SECTION mim_section;
static volatile int stop_thread = 0;
static volatile int reset_synth = 0;
static HANDLE hCalcThread = NULL;
static DWORD processPriority;
static HANDLE load_sfevent = NULL; 


static unsigned int font_count = 0;
static HSOUNDFONT * hFonts = NULL;
static HSTREAM hStream = 0;

static BOOL com_initialized = FALSE;
static BOOL sound_out_float = FALSE;
static sound_out * sound_driver = NULL;

static HINSTANCE bass = 0;			// bass handle
static HINSTANCE bassmidi = 0;			// bassmidi handle
static HINSTANCE hinst = NULL;             //main DLL handle

static void DoStartDriver();
static void DoStopDriver();

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved ){
	if (fdwReason == DLL_PROCESS_ATTACH){
	    hinst = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}else if(fdwReason == DLL_PROCESS_DETACH){
		;
		DoStopDriver();
	}
	return TRUE;    
}

static void FreeFonts()
{
	unsigned i;
	if ( hFonts && font_count )
	{
		for ( i = 0; i < font_count; ++i )
		{
			BASS_MIDI_FontFree( hFonts[ i ] );
		}
		free( hFonts );
		hFonts = NULL;
		font_count = 0;
	}
}

void LoadFonts(const TCHAR * name)
{
	const DWORD bass_flags =
#ifdef UNICODE
		BASS_UNICODE
#else
		0
#endif
		;

	FreeFonts();

	if (name && *name)
	{
		const TCHAR * ext = _tcsrchr( name, _T('.') );
		if ( ext ) ext++;
		if ( !_tcsicmp( ext, _T("sf2") ) || !_tcsicmp( ext, _T("sf2pack") ) )
		{
			font_count = 1;
			hFonts = (HSOUNDFONT*)malloc( sizeof(HSOUNDFONT) );
			*hFonts = BASS_MIDI_FontInit( name, bass_flags );
		}
		else if ( !_tcsicmp( ext, _T("sflist") ) )
		{
			FILE * fl = _tfopen( name, _T("r, ccs=UTF-8") );
			font_count = 0;
			if ( fl )
			{
				TCHAR path[1024], fontname[1024], temp[1024];
				const TCHAR * filename = _tcsrchr( name, _T('\\') ) + 1;
				if ( filename == (void*)1 ) filename = _tcsrchr( name, _T(':') ) + 1;
				if ( filename == (void*)1 ) filename = name;
				_tcsncpy( path, name, filename - name );
				path[ filename - name ] = 0;
				while ( !feof( fl ) )
				{
					TCHAR * cr;
					if( !_fgetts( fontname, 1024, fl ) ) break;
					fontname[1023] = 0;
					cr = _tcsrchr( fontname, _T('\n') );
					if ( cr ) *cr = 0;
					if ( isalpha( fontname[0] ) && fontname[1] == _T(':') )
					{
						cr = fontname;
					}
					else
					{
						_tcscpy( temp, path );
						_tcscat( temp, fontname );
						cr = temp;
					}
					font_count++;
					hFonts = (HSOUNDFONT*)realloc( hFonts, sizeof(HSOUNDFONT) * font_count );
					hFonts[ font_count - 1 ] = BASS_MIDI_FontInit( cr, bass_flags );
				}
				fclose( fl );
			}
		}
	}
}

STDAPI_(LONG) DriverProc(DWORD dwDriverId, HDRVR hdrvr, UINT msg, LONG lParam1, LONG lParam2){
 
	switch(msg) {
	case DRV_FREE: // XXX never called
		return DRV_OK;
	case DRV_LOAD:
		memset(drivers, 0, sizeof(drivers));
		driverCount = 0;
		return DRV_OK;
	case DRV_OPEN:
		{
			int driverNum;
			if (driverCount == MAX_DRIVERS) {
				return 0;
			} else {
				for (driverNum = 0; driverNum < MAX_DRIVERS; driverNum++) {
					if (!drivers[driverNum].open) {
						break;
					}
					if (driverNum == MAX_DRIVERS) {
						return 0;
					}
				}
			}
			drivers[driverNum].open = 1;
			drivers[driverNum].clientCount = 0;
			drivers[driverNum].hdrvr = hdrvr;
			driverCount++;
		}
		return DRV_OK;
	case DRV_CLOSE: // XXX never called
		{
			int i;
			for (i = 0; i < MAX_DRIVERS; i++) {
				if (drivers[i].open && drivers[i].hdrvr == hdrvr) {
					drivers[i].open = 0;
					--driverCount;
					return DRV_OK;
				}
			}
		}
		return DRV_CANCEL;
	case DRV_CONFIGURE:
	case DRV_DISABLE:
	case DRV_ENABLE:
	case DRV_EXITSESSION:
	case DRV_REMOVE:
	case DRV_INSTALL:
	case DRV_POWER:
	case DRV_QUERYCONFIGURE:
		return DRV_OK;
	}
	return DRV_OK;
}

HRESULT modGetCaps(PVOID capsPtr, DWORD capsSize) {
	MIDIOUTCAPSA * myCapsA;
	MIDIOUTCAPSW * myCapsW;
	MIDIOUTCAPS2A * myCaps2A;
	MIDIOUTCAPS2W * myCaps2W;

	CHAR synthName[] = "BASSMIDI Driver\0";
	WCHAR synthNameW[] = L"BASSMIDI Driver\0";

	switch (capsSize) {
	case (sizeof(MIDIOUTCAPSA)):
		myCapsA = (MIDIOUTCAPSA *)capsPtr;
		myCapsA->wMid = 0xffff;
		myCapsA->wPid = 0xffff;
		memcpy(myCapsA->szPname, synthName, sizeof(synthName));
		myCapsA->wTechnology = MOD_MIDIPORT;
		myCapsA->vDriverVersion = 0x0090;
		myCapsA->wVoices = 0;
		myCapsA->wNotes = 0;
		myCapsA->wChannelMask = 0xffff;
		myCapsA->dwSupport = 0;
		return MMSYSERR_NOERROR;

	case (sizeof(MIDIOUTCAPSW)):
		myCapsW = (MIDIOUTCAPSW *)capsPtr;
		myCapsW->wMid = 0xffff;
		myCapsW->wPid = 0xffff;
		memcpy(myCapsW->szPname, synthNameW, sizeof(synthNameW));
		myCapsW->wTechnology = MOD_MIDIPORT;
		myCapsW->vDriverVersion = 0x0090;
		myCapsW->wVoices = 0;
		myCapsW->wNotes = 0;
		myCapsW->wChannelMask = 0xffff;
		myCapsW->dwSupport = 0;
		return MMSYSERR_NOERROR;

	case (sizeof(MIDIOUTCAPS2A)):
		myCaps2A = (MIDIOUTCAPS2A *)capsPtr;
		myCaps2A->wMid = 0xffff;
		myCaps2A->wPid = 0xffff;
		memcpy(myCaps2A->szPname, synthName, sizeof(synthName));
		myCaps2A->wTechnology = MOD_MIDIPORT;
		myCaps2A->vDriverVersion = 0x0090;
		myCaps2A->wVoices = 0;
		myCaps2A->wNotes = 0;
		myCaps2A->wChannelMask = 0xffff;
		myCaps2A->dwSupport = 0;
		return MMSYSERR_NOERROR;

	case (sizeof(MIDIOUTCAPS2W)):
		myCaps2W = (MIDIOUTCAPS2W *)capsPtr;
		myCaps2W->wMid = 0xffff;
		myCaps2W->wPid = 0xffff;
		memcpy(myCaps2W->szPname, synthNameW, sizeof(synthNameW));
		myCaps2W->wTechnology = MOD_MIDIPORT;
		myCaps2W->vDriverVersion = 0x0090;
		myCaps2W->wVoices = 0;
		myCaps2W->wNotes = 0;
		myCaps2W->wChannelMask = 0xffff;
		myCaps2W->dwSupport = 0;
		return MMSYSERR_NOERROR;

	default:
		return MMSYSERR_ERROR;
	}
}


struct evbuf_t{
	UINT uMsg;
	DWORD	dwParam1;
	DWORD	dwParam2;
	int exlen;
	unsigned char *sysexbuffer;
};
#define EVBUFF_SIZE 512
static struct evbuf_t evbuf[EVBUFF_SIZE];
static UINT  evbwpoint=0;
static UINT  evbrpoint=0;
static UINT evbsysexpoint;

int bmsyn_buf_check(void){
	int retval;
	EnterCriticalSection(&mim_section);
	retval = (evbrpoint != evbwpoint) ? ~0 :  0;
	LeaveCriticalSection(&mim_section);
	return retval;
}

static const unsigned char sysex_gm_reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const unsigned char sysex_gm2_reset[]= { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const unsigned char sysex_gs_reset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const unsigned char sysex_xg_reset[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static BOOL is_gs_reset(const unsigned char * data, unsigned size)
{
	if ( size != _countof( sysex_gs_reset ) ) return FALSE;

	if ( memcmp( data, sysex_gs_reset, 5 ) != 0 ) return FALSE;
	if ( memcmp( data + 7, sysex_gs_reset + 7, 2 ) != 0 ) return  FALSE;
	if ( ( ( data[ 5 ] + data[ 6 ] + 1 ) & 127 ) != data[ 9 ] ) return  FALSE;
	if ( data[ 10 ] != sysex_gs_reset[ 10 ] ) return  FALSE;

	return TRUE;
}

int bmsyn_play_some_data(void){
	UINT uMsg;
	DWORD	dwParam1;
	DWORD	dwParam2;
	
	UINT evbpoint;
	MIDIHDR *IIMidiHdr;
	int exlen;
	unsigned char *sysexbuffer;
	int played;
		
	played=0;
		if( !bmsyn_buf_check() ){ 
			played=~0;
			return played;
		}
		do{
			EnterCriticalSection(&mim_section);
			evbpoint=evbrpoint;
			if (++evbrpoint >= EVBUFF_SIZE)
					evbrpoint -= EVBUFF_SIZE;

			uMsg=evbuf[evbpoint].uMsg;
			dwParam1=evbuf[evbpoint].dwParam1;
			dwParam2=evbuf[evbpoint].dwParam2;
		    exlen=evbuf[evbpoint].exlen;
			sysexbuffer=evbuf[evbpoint].sysexbuffer;
			
			LeaveCriticalSection(&mim_section);
			switch (uMsg) {
			case MODM_DATA:
				dwParam2 = dwParam1 & 0xF0;
				exlen = ( dwParam2 == 0xC0 || dwParam2 == 0xD0 ) ? 2 : 3;
				BASS_MIDI_StreamEvents( hStream, BASS_MIDI_EVENTS_RAW, &dwParam1, exlen );
				break;
			case MODM_LONGDATA:
#ifdef DEBUG
	FILE * logfile;
	logfile = fopen("c:\\dbglog2.log","at");
	if(logfile!=NULL) {
		for(int i = 0 ; i < exlen ; i++)
			fprintf(logfile,"%x ", sysexbuffer[i]);
		fprintf(logfile,"\n");
	}
	fclose(logfile);
#endif
				BASS_MIDI_StreamEvents( hStream, BASS_MIDI_EVENTS_RAW, sysexbuffer, exlen );
				free(sysexbuffer);
				break;
			}
		}while(bmsyn_buf_check());	
	return played;
}

void load_settings()
{
	int config_volume;
	HKEY hKey, hSubKey;
	long lResult;
	DWORD dwType=REG_DWORD;
	DWORD dwSize=sizeof(DWORD);
	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\BASSMIDI Driver", 0, KEY_READ, &hKey);
	RegQueryValueEx(hKey, L"volume", NULL, &dwType,(LPBYTE)&config_volume, &dwSize);
	RegCloseKey( hKey);
	BASS_SetConfig(BASS_CONFIG_GVOL_STREAM,config_volume);
}

int check_sinc()
{
	int sinc = 0;
	HKEY hKey, hSubKey;
	long lResult;
	DWORD dwType=REG_DWORD;
	DWORD dwSize=sizeof(DWORD);
	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\BASSMIDI Driver", 0, KEY_READ, &hKey);
	RegQueryValueEx(hKey, L"sinc", NULL, &dwType,(LPBYTE)&sinc, &dwSize);
	RegCloseKey( hKey);
	return sinc;
}

BOOL load_bassfuncs()
{
		TCHAR installpath[1024] = {0};
		TCHAR basspath[1024] = {0};
		TCHAR bassmidipath[1024] = {0};
		TCHAR pluginpath[MAX_PATH] = {0};
		WIN32_FIND_DATA fd;
		HANDLE fh;
		int installpathlength;
		
		GetModuleFileName(hinst, installpath, 1024);
		PathRemoveFileSpec(installpath);

		lstrcat(basspath,installpath);
		lstrcat(basspath,L"\\bass.dll");
		if (!(bass=LoadLibrary(basspath))) {
			OutputDebugString(L"Failed to load BASS DLL!");
			return FALSE;
		}
		lstrcat(bassmidipath,installpath);
		lstrcat(bassmidipath,L"\\bassmidi.dll");
		if (!(bassmidi=LoadLibrary(bassmidipath))) {
			OutputDebugString(L"Failed to load BASSMIDI DLL!");
			return FALSE;
		}
		/* "load" all the BASS functions that are to be used */
		OutputDebugString(L"Loading BASS functions....");
		LOADBASSFUNCTION(BASS_ErrorGetCode);
		LOADBASSFUNCTION(BASS_SetConfig);
		LOADBASSFUNCTION(BASS_Init);
		LOADBASSFUNCTION(BASS_Free);
		LOADBASSFUNCTION(BASS_GetInfo);
		LOADBASSFUNCTION(BASS_StreamFree);
		LOADBASSFUNCTION(BASS_PluginLoad);
		LOADBASSFUNCTION(BASS_ChannelGetData);
		LOADBASSMIDIFUNCTION(BASS_MIDI_StreamCreate);
		LOADBASSMIDIFUNCTION(BASS_MIDI_FontInit);
		LOADBASSMIDIFUNCTION(BASS_MIDI_FontFree);
		LOADBASSMIDIFUNCTION(BASS_MIDI_StreamSetFonts);
		LOADBASSMIDIFUNCTION(BASS_MIDI_StreamEvents);
		LOADBASSMIDIFUNCTION(BASS_MIDI_StreamEvent);

		installpathlength=lstrlen(installpath)+1;
		lstrcat(pluginpath,installpath);
		lstrcat(pluginpath,L"\\bass*.dll");
		fh=FindFirstFile(pluginpath,&fd);
		if (fh!=INVALID_HANDLE_VALUE) {
			do {
				HPLUGIN plug;
				pluginpath[installpathlength]=0;
				lstrcat(pluginpath,fd.cFileName);
				plug=BASS_PluginLoad((char*)pluginpath,BASS_UNICODE);
			} while (FindNextFile(fh,&fd));
			FindClose(fh);
		}

		return TRUE;
}

BOOL IsVistaOrNewer(){
	OSVERSIONINFOEX osvi;
	BOOL bOsVersionInfoEx;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*) &osvi);
	if(bOsVersionInfoEx == FALSE) return FALSE;
	if ( VER_PLATFORM_WIN32_NT==osvi.dwPlatformId && 
		osvi.dwMajorVersion > 5 )
		return TRUE;
	return FALSE;
}


unsigned __stdcall threadfunc(LPVOID lpV){
	unsigned i;
	int opend=0;
	TCHAR config[1024];
	BASS_MIDI_FONT * mf;
	BASS_INFO info;
	BOOL isvista;
	;
	while(opend == 0 && stop_thread == 0) {
		Sleep(100);
		if (!com_initialized) {
			if (FAILED(CoInitialize(NULL))) continue;
			com_initialized = TRUE;
		}
		if (sound_driver == NULL) {
			sound_driver = create_sound_out_xaudio2();
			const char * err = sound_driver->open(GetDesktopWindow(), 44100, 2, sound_out_float = TRUE, 88 * 2, 30);
			if (err) {
				delete sound_driver;
				isvista = IsVistaOrNewer();
				sound_driver = create_sound_out_ds();
				err = sound_driver->open(GetDesktopWindow(), 44100, 2, sound_out_float = FALSE, 88 * 2, 50);
			}
			if (err) {
				delete sound_driver;
				sound_driver = NULL;
				continue;
			}
		}
		load_bassfuncs();
		BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
		BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);
		if ( BASS_Init( 0, 44100, 0, GetDesktopWindow(), NULL ) ) {
			hStream = BASS_MIDI_StreamCreate( 16, BASS_STREAM_DECODE | ( sound_out_float ? BASS_SAMPLE_FLOAT : 0 ) | (check_sinc()?BASS_MIDI_SINCINTER: 0), 44100 );
			if (!hStream) continue;
			BASS_MIDI_StreamEvent( hStream, 0, MIDI_EVENT_SYSTEMEX, MIDI_SYSTEM_GM1 );
			load_settings();
			if (GetWindowsDirectory(config, 1023 - 16))
			{
				_tcscat( config, _T("\\bassmidi.sflist") );
			}
			LoadFonts(config);
			if (font_count) {
				mf = (BASS_MIDI_FONT*)malloc( sizeof(BASS_MIDI_FONT) * font_count );
				for ( i = 0; i < font_count; ++i ) {
					mf[i].font = hFonts[ font_count - i - 1 ];
					mf[i].preset = -1;
					mf[i].bank = 0;
				}
				BASS_MIDI_StreamSetFonts( hStream, mf, font_count );
				free(mf);
			}
			SetEvent(load_sfevent);
			opend = 1;
			reset_synth = 0;
		}
	}

	while(stop_thread == 0){
		Sleep(1);
		if (reset_synth != 0){
			reset_synth = 0;
			BASS_MIDI_StreamEvent( hStream, 0, MIDI_EVENT_SYSTEMEX, MIDI_SYSTEM_GM1 );
		}
		bmsyn_play_some_data();
		if (sound_out_float) {
			float sound_buffer[88*2];
			int decoded = BASS_ChannelGetData( hStream, sound_buffer, BASS_DATA_FLOAT + 88 * 2 * sizeof(float) );
			if ( decoded < 0 ) Sleep(1);
			else sound_driver->write_frame( sound_buffer, decoded / sizeof(float), true );
		} else {
			short sound_buffer[88*2];
			int decoded = BASS_ChannelGetData( hStream, sound_buffer, 88 * 2 * sizeof(short) );
			if ( decoded < 0 ) Sleep(1);
			else sound_driver->write_frame( sound_buffer, decoded / sizeof(short), true );
		}
	}
	stop_thread=0;
	if (hStream)
	{
		BASS_StreamFree( hStream );
		hStream = 0;
	}
	if (bassmidi) {
		FreeFonts();
		FreeLibrary(bassmidi);
		bassmidi =0 ;
	}
	if ( bass ) {
		BASS_Free();
		FreeLibrary(bass);
		bass = 0;
	}
	if ( sound_driver ) {
		delete sound_driver;
		sound_driver = NULL;
	}
	if ( com_initialized ) {
		CoUninitialize();
		com_initialized = FALSE;
	}
	_endthreadex(0);
	return 0;
}

void DoCallback(int driverNum, int clientNum, DWORD msg, DWORD param1, DWORD param2) {
	struct Driver_Client *client = &drivers[driverNum].clients[clientNum];
	DriverCallback(client->callback, client->flags, drivers[driverNum].hdrvr, msg, client->instance, param1, param2);
}

void DoStartDriver() {
	if (modm_closed  == 1) {
		DWORD result;
		unsigned int thrdaddr;
		InitializeCriticalSection(&mim_section);
		processPriority = GetPriorityClass(GetCurrentProcess());
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
		load_sfevent = CreateEvent( 
			NULL,               // default security attributes
			TRUE,               // manual-reset event
			FALSE,              // initial state is nonsignaled
			TEXT("SoundFontEvent")  // object name
			); 
		hCalcThread=(HANDLE)_beginthreadex(NULL,0,threadfunc,0,0,&thrdaddr);
		SetPriorityClass(hCalcThread, REALTIME_PRIORITY_CLASS);
		SetThreadPriority(hCalcThread, THREAD_PRIORITY_TIME_CRITICAL);
		result = WaitForSingleObject(load_sfevent,INFINITE);
		if (result == WAIT_OBJECT_0)
		{
			CloseHandle(load_sfevent);
		}
		modm_closed = 0;
	}
}

void DoStopDriver() {
	if (modm_closed == 0){
		stop_thread = 1;
		WaitForSingleObject(hCalcThread, INFINITE);
		CloseHandle(hCalcThread);
		modm_closed = 1;
		SetPriorityClass(GetCurrentProcess(), processPriority);
	}
	DeleteCriticalSection(&mim_section);
}

void DoResetDriver(DWORD dwParam1, DWORD dwParam2) {
	reset_synth = 1;
}

LONG DoOpenDriver(struct Driver *driver, UINT uDeviceID, UINT uMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2) {
	int clientNum;
	MIDIOPENDESC *desc;
	if (driver->clientCount == 0) {
		DoStartDriver();
		DoResetDriver(dwParam1, dwParam2);
		clientNum = 0;
	} else if (driver->clientCount == MAX_CLIENTS) {
		return MMSYSERR_ALLOCATED;
	} else {
		int i;
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (!driver->clients[i].allocated) {
				break;
			}
		}
		if (i == MAX_CLIENTS) {
			return MMSYSERR_ALLOCATED;
		}
		clientNum = i;
	}
	desc = (MIDIOPENDESC *)dwParam1;
	driver->clients[clientNum].allocated = 1;
	driver->clients[clientNum].flags = HIWORD(dwParam2);
	driver->clients[clientNum].callback = desc->dwCallback;
	driver->clients[clientNum].instance = desc->dwInstance;
	*(LONG *)dwUser = clientNum;
	driver->clientCount++;
	SetPriorityClass(GetCurrentProcess(), processPriority);
	DoCallback(uDeviceID, clientNum, MOM_OPEN, 0, 0);
	return MMSYSERR_NOERROR;
}

LONG DoCloseDriver(struct Driver *driver, UINT uDeviceID, UINT uMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2) {
	if (!driver->clients[dwUser].allocated) {
		return MMSYSERR_INVALPARAM;
	}

	driver->clients[dwUser].allocated = 0;
	driver->clientCount--;
	if(driver->clientCount <= 0) {
		DoResetDriver(dwParam1, dwParam2);
		driver->clientCount = 0;
	}
	DoCallback(uDeviceID, dwUser, MOM_CLOSE, 0, 0);
	return MMSYSERR_NOERROR;
}

STDAPI_(LONG) midMessage(UINT uDeviceID, UINT uMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2) {
	struct Driver *driver = &drivers[uDeviceID];
	switch (uMsg) {
	case MIDM_OPEN:
		return DoOpenDriver(driver, uDeviceID, uMsg, dwUser, dwParam1, dwParam2);

	case MIDM_CLOSE:
		return DoCloseDriver(driver, uDeviceID, uMsg, dwUser, dwParam1, dwParam2);

	default:
		return MMSYSERR_NOERROR;
		break;
	}
}

STDAPI_(LONG) modMessage(UINT uDeviceID, UINT uMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2){
	MIDIHDR *IIMidiHdr;	
	UINT evbpoint;
	struct Driver *driver = &drivers[uDeviceID];
	int exlen = 0;
	unsigned char *sysexbuffer = NULL ;
	DWORD result = 0;
	switch (uMsg) {
	case MODM_OPEN:
		return DoOpenDriver(driver, uDeviceID, uMsg, dwUser, dwParam1, dwParam2);
		break;
	case MODM_PREPARE:
		return MMSYSERR_NOTSUPPORTED;
		break;
	case MODM_UNPREPARE:
		return MMSYSERR_NOTSUPPORTED;
		break;
	case MODM_GETDEVCAPS:
		return modGetCaps((PVOID)dwParam1, dwParam2);
		break;
	case MODM_RESET:
		DoResetDriver(dwParam1, dwParam2);
		return MMSYSERR_NOERROR;
	case MODM_LONGDATA:
		IIMidiHdr = (MIDIHDR *)dwParam1;
		if( !(IIMidiHdr->dwFlags & MHDR_PREPARED) ) return MIDIERR_UNPREPARED;
		IIMidiHdr->dwFlags &= ~MHDR_DONE;
		IIMidiHdr->dwFlags |= MHDR_INQUEUE;
		IIMidiHdr = (MIDIHDR *) dwParam1;
		exlen=(int)IIMidiHdr->dwBufferLength;
		if( NULL == (sysexbuffer = (unsigned char *)malloc(exlen * sizeof(char)))){
			return MMSYSERR_NOMEM;
		}else{
			memcpy(sysexbuffer,IIMidiHdr->lpData,exlen);
#ifdef DEBUG
	FILE * logfile;
	logfile = fopen("c:\\dbglog.log","at");
	if(logfile!=NULL) {
		fprintf(logfile,"sysex %d byete\n", exlen);
		for(int i = 0 ; i < exlen ; i++)
			fprintf(logfile,"%x ", sysexbuffer[i]);
		fprintf(logfile,"\n");
	}
	fclose(logfile);
#endif
		}
		IIMidiHdr->dwFlags &= ~MHDR_INQUEUE;
		IIMidiHdr->dwFlags |= MHDR_DONE;
		DoCallback(uDeviceID, dwUser, MOM_DONE, dwParam1, 0);
	case MODM_DATA:
		EnterCriticalSection(&mim_section);
		evbpoint = evbwpoint;
		if (++evbwpoint >= EVBUFF_SIZE)
			evbwpoint -= EVBUFF_SIZE;
		evbuf[evbpoint].uMsg = uMsg;
		evbuf[evbpoint].dwParam1 = dwParam1;
		evbuf[evbpoint].dwParam2 = dwParam2;
		evbuf[evbpoint].exlen=exlen;
		evbuf[evbpoint].sysexbuffer=sysexbuffer;
		LeaveCriticalSection(&mim_section);
		return MMSYSERR_NOERROR;
		break;		
	case MODM_GETNUMDEVS:
		return 0x1;
		break;
	case MODM_CLOSE:
		return DoCloseDriver(driver, uDeviceID, uMsg, dwUser, dwParam1, dwParam2);
		break;
	default:
		return MMSYSERR_NOERROR;
		break;
	}
}

