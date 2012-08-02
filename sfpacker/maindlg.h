// maindlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINDLG_H__6920296A_4C3F_11D1_AA9A_000000000000__INCLUDED_)
#define AFX_MAINDLG_H__6920296A_4C3F_11D1_AA9A_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <iostream>
#include <fstream> 
#include "utf8conv.h"
#include "DropFileTarget.h"



#define BASSDEF(f) (WINAPI *f)	// define the BASS/BASSMIDI functions as pointers
#define BASSMIDIDEF(f) (WINAPI *f)
#define LOADBASSFUNCTION(f) *((void**)&f)=GetProcAddress(bass,#f)
#define LOADBASSMIDIFUNCTION(f) *((void**)&f)=GetProcAddress(bassmidi,#f)
#include "../bass.h"
#include "../bassmidi.h"
using namespace std;
using namespace utf8util;

const TCHAR* pack_descr[] = 
{ 
  L"FLAC", 
  L"LAME (V2)",
  L"Musepack (Q5)",
  L"WavPack (lossless)",
  L"WavPack (lossy, HQ)",
  L"WavPack (lossy, average)",
  L"WavPack (lossy, low)",
  L"Vorbis (Q4)"

};
const char* pack_cmdline[] = 
{ "flac --best -", 
  "lame -v2 -",
  "mpcenc -q5 -",
  "wavpack -h -",
  "wavpack -hb384 -",
  "wavpack -hb256 -", 
  "wavpack -hb128 -" 
};

class CMainDlg : public CDialogImpl<CMainDlg>, public CDropFileTarget<CMainDlg>, public CMessageFilter
{
	HMODULE bass, bassmidi;
	CComboBox compress_type;
	CStatic   compress_str, filename;
	CButton  pack_sf;
	
public:
	enum { IDD = IDD_DIALOG };

	CMainDlg()
	{
		
	}

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_PACK, OnPack)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		CHAIN_MSG_MAP(CDropFileTarget<CMainDlg>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// center the dialog on the screen
		CenterWindow();
		// register object for message filtering
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		pLoop->AddMessageFilter(this);

		RegisterDropTarget();

		filename = GetDlgItem(IDC_FILENAME);
		compress_str = GetDlgItem(IDC_COMPSTR);
		pack_sf = GetDlgItem(ID_PACK);
		compress_type = GetDlgItem(IDC_COMPTYPE);

		for (int i=0;i<_countof(pack_descr);i++)compress_type.AddString((TCHAR*)pack_descr[i]);

		TCHAR installpath[1024] = {0};
		TCHAR basspath[1024] = {0};
		TCHAR bassmidipath[1024] = {0};
		TCHAR bassflacpath[1024] = {0};
		TCHAR basswvpath[1024] = {0};
		GetModuleFileName(GetModuleHandle(0), installpath, 1024);
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
		LOADBASSFUNCTION(BASS_PluginGetInfo);
		LOADBASSMIDIFUNCTION(BASS_MIDI_FontInit);
		LOADBASSMIDIFUNCTION(BASS_MIDI_FontLoad);
		LOADBASSMIDIFUNCTION(BASS_MIDI_FontGetInfo);
		LOADBASSMIDIFUNCTION(BASS_MIDI_FontPack);
		LOADBASSMIDIFUNCTION(BASS_MIDI_FontUnpack);
		LOADBASSMIDIFUNCTION(BASS_MIDI_FontFree);

		BASS_Init(0,44100,0,0,NULL);
		BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD,0);

		WIN32_FIND_DATA fd;
		HANDLE fh;
		TCHAR pluginpath[MAX_PATH] = {0};
		lstrcat(pluginpath,installpath);
		lstrcat(pluginpath,L"\\bass*.dll");
		int installpathlength=lstrlen(installpath)+1;
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

	void ProcessFile(LPCTSTR lpszPath)
	{
		filename.SetWindowText(lpszPath);
		TCHAR new_fname[MAX_PATH] = {0};
		const TCHAR * ext = _tcsrchr( lpszPath, _T('.') );
		if ( ext ) ext++;
		if ( !_tcsicmp( ext, _T("sf2")) || !_tcsicmp( ext, _T("sf2pack")) )
		{
			BASS_MIDI_FONTINFO info;
			HSOUNDFONT sf2 = BASS_MIDI_FontInit(lpszPath,BASS_UNICODE);
			const TCHAR* compression;
			BASS_MIDI_FontGetInfo(sf2,&info);

			if ( ((!_tcsicmp( ext, _T("sf2")) && info.samtype != 0)) ||  (!_tcsicmp( ext, _T("sf2pack")) && info.samtype == 0) ) //must rename file on packing/depacking
			{
			    BASS_MIDI_FontFree(sf2);
				int iResponse = MessageBox(L"The file you loaded is not what the extension suggests. Rename the file?",L"WARNING",MB_YESNO|MB_ICONINFORMATION);
				if (iResponse == IDYES)
				{
					_tcscpy(new_fname,lpszPath);
					PathRemoveExtension(new_fname);

					if (!_tcsicmp( ext, L"sf2"))
					{

						lstrcat(new_fname,L".sf2pack");
					}
					else
					{
						lstrcat(new_fname,L".sf2");
					}
					int res = MoveFile(lpszPath,new_fname);
					if (!res)  
					{
						MessageBox(L"Failed to rename the file",L"Error",MB_ICONSTOP);
						return;
					}
					MessageBox(L"File renamed. Now you can convert the file",L"Success",MB_ICONINFORMATION);
					ProcessFile(new_fname);
				}
			}

			wstring utf8 = utf16_from_utf8(info.name);
			switch (info.samtype)
			{
			default:
				compression = L"Unsupported";
				break;
			case -1:
				compression = L"Unknown";
				break;
			case 0:
				compression = L"None";
				break;
			case 0x10002:
				compression = L"Vorbis";
				break;
			case 0x10003:
				compression = L"MP1";
				break;
			case 0x10004:
				compression = L"MP2";
				break;
			case 0x10005:
				compression = L"MP3";
				break;
			case 0x10900:
				compression = L"FLAC";
				break;
			case 0x10901:
				compression = L"OggFLAC";
				break;
			case 0x10500:
				compression = L"WavPack";
				break;
			case 0x10a00:
				compression = L"Musepack";
				break;
			case 0x11200:
				compression = L"Opus";
				break;
			}
			BASS_MIDI_FontFree(sf2);

			if (info.samtype == 0){
				pack_sf.SetWindowText(L"Pack!");
				compress_type.EnableWindow(true);
			}
			else
			{
				pack_sf.SetWindowText(L"Unpack!");
				compress_type.EnableWindow(false);
			}
			compress_str.SetWindowText(compression);
		}	
	}

	int do_fileops(TCHAR* sf_path)
	{
		TCHAR new_fname[MAX_PATH] = {0};
		BASS_MIDI_FONTINFO info;
		HSOUNDFONT sf2 = BASS_MIDI_FontInit(sf_path,BASS_UNICODE);
		BASS_MIDI_FontGetInfo(sf2,&info);
	

		if (info.samtype == 0) //pack!
		{
			_tcscpy(new_fname,sf_path);
			PathRemoveExtension(new_fname);
			lstrcat(new_fname,L".sf2pack");

		}
		else //depack!
		{
			_tcscpy(new_fname,sf_path);
			PathRemoveExtension(new_fname);
			lstrcat(new_fname,L".sf2");
			if (!BASS_MIDI_FontUnpack(sf2,new_fname,0)) {
				MessageBox(L"SoundFont unpacking failed",L"Error",MB_ICONSTOP);
				BASS_MIDI_FontFree(sf2);
				return 1;
			}
			MessageBox(L"SoundFont unpacking succeeded",L"Success!",MB_ICONINFORMATION);

		}
		BASS_MIDI_FontFree(sf2);
		return 0;
	}

	LRESULT OnPack(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
	    TCHAR sf_path[MAX_PATH] ={0};
		filename.GetWindowText(sf_path, sizeof(sf_path));   
		do_fileops(sf_path);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{

		CloseDialog(wID);
		return 0;
	}

	void CloseDialog(int nVal)
	{
		DestroyWindow();
		::PostQuitMessage(nVal);
	}
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINDLG_H__6920296A_4C3F_11D1_AA9A_000000000000__INCLUDED_)
