// drivercfg.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "drivercfg.h"

#include <iostream> 
#include <fstream> 
#include <Shlobj.h>
#include "Shlwapi.h"

using namespace std;
#include "utf8conv.h" 
using namespace utf8util;

void load_midisynths(HWND hwnd);
void set_midisynth(HWND hwnd);
void write_sflist(HWND hwnd);
void read_sflist(HWND hwnd, TCHAR* sfpath);

LRESULT CALLBACK MainDlgProc(HWND hWndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND lbxSoundFonts;
	TCHAR szFileName[MAX_PATH] = L"";
	lbxSoundFonts = GetDlgItem(hWndDlg, IDC_SFLIST);
	int amount_items = 0;
	switch(msg)
	{
	case WM_INITDIALOG:

		TCHAR sfpath[MAX_PATH];
		if(SUCCEEDED(SHGetFolderPath(NULL, 
			CSIDL_WINDOWS|CSIDL_FLAG_CREATE, 
			NULL, 
			0, 
			sfpath))) 
		{
			load_midisynths(hWndDlg);
			PathAppend(sfpath,L"bassmidi.sflist");
			read_sflist(hWndDlg, sfpath);
		}
		break;
	case WM_COMMAND:
		switch(wParam)
		{  
		case IDC_ADDSF:
			OPENFILENAME ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hWndDlg;
			ofn.lpstrFilter =L"SoundFonts (*.sf2)\0*.sf2\0SoundFont list files (*.sflist)\0*.sflist\0";
			ofn.lpstrFile = szFileName;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			ofn.lpstrDefExt = L"sf2";
			if(GetOpenFileName(&ofn))
			{
				const TCHAR * ext = _tcsrchr( szFileName, _T('.') );
				if ( ext ) ext++;
				if ( !_tcsicmp( ext, _T("sf2")))
				{

					ListBox_AddString(lbxSoundFonts, szFileName);
				}
				else if ( !_tcsicmp( ext, _T("sflist") ))
				{
					read_sflist(hWndDlg,szFileName);
				}

			}
			break;
		case IDC_RMSF:
			int iSelectedIndex;
			iSelectedIndex = ListBox_GetCurSel(lbxSoundFonts);
			ListBox_DeleteString(lbxSoundFonts,iSelectedIndex);
			break;
		case IDC_DOWNSF:
		amount_items = ListBox_GetCount(lbxSoundFonts);
		amount_items--;
		TCHAR SelectedItem[MAX_PATH];
		int SelectedIndex;
		SelectedIndex = ListBox_GetCurSel(lbxSoundFonts);
		if (SelectedIndex != amount_items && SelectedIndex != -1)
		{
			ListBox_GetText(lbxSoundFonts, SelectedIndex, (LPCTSTR)SelectedItem);
			ListBox_DeleteString(lbxSoundFonts,SelectedIndex);
			ListBox_InsertString(lbxSoundFonts,SelectedIndex+1,SelectedItem);
			ListBox_SetCurSel(lbxSoundFonts,SelectedIndex+1);
		}
		break;
		case IDC_UPSF:
		amount_items = ListBox_GetCount(lbxSoundFonts);
		SelectedIndex = ListBox_GetCurSel(lbxSoundFonts);
		if (SelectedIndex != 0)
		{
			ListBox_GetText(lbxSoundFonts, SelectedIndex, (LPCTSTR)SelectedItem);
			ListBox_DeleteString(lbxSoundFonts,SelectedIndex);
			ListBox_InsertString(lbxSoundFonts,SelectedIndex-1,SelectedItem);
			ListBox_SetCurSel(lbxSoundFonts,SelectedIndex-1);

		}
		break;
		case IDC_SFAPPLY:
		write_sflist(hWndDlg);
		break;
		case IDC_SNAPPLY:
		set_midisynth(hWndDlg);
		break;
		case IDOK:
			EndDialog(hWndDlg, 0);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		PostQuitMessage(WM_QUIT);
		break;
	}

	return FALSE;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)MainDlgProc);
}

void read_sflist(HWND hwnd, TCHAR* sfpath)
{
	HWND lbxSoundFonts = GetDlgItem(hwnd, IDC_SFLIST);
	ListBox_ResetContent(lbxSoundFonts);
	int font_count;
	if (sfpath && *sfpath)
	{
		FILE * fl = _tfopen( sfpath,L"r, ccs=UTF-8" );
		font_count = 0;
		if ( fl)
		{
			TCHAR path[1024], fontname[1024], temp[1024];
			const TCHAR * filename = _tcsrchr( sfpath, _T('\\') ) + 1;
			if ( filename == (void*)1 ) filename = _tcsrchr( sfpath, _T(':') ) + 1;
			if ( filename == (void*)1 ) filename = sfpath;
			_tcsncpy( path, sfpath, filename - sfpath );
			path[ filename - sfpath ] = 0;
			while ( !feof( fl ))
			{
				TCHAR * cr;
				if( !_fgetts( fontname, 1024, fl ) ) break;
				fontname[1023] = 0;
				cr = _tcsrchr( fontname, _T('\n') );
				if ( cr ) *cr = 0;
				if ( isalpha( fontname[0] ) && fontname[1] == _T(':'))
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
				ListBox_AddString(lbxSoundFonts, cr);
			}
			fclose( fl );
		}
	}
}

void write_sflist(HWND hwnd)
{
	HWND lbxSoundFonts = GetDlgItem(hwnd, IDC_SFLIST);
	int filecount = ListBox_GetCount(lbxSoundFonts);
	TCHAR file_name[MAX_PATH];
	ZeroMemory(file_name,sizeof(file_name));
	TCHAR sfpath[MAX_PATH];

	if(SUCCEEDED(SHGetFolderPath(NULL, 
		CSIDL_WINDOWS|CSIDL_FLAG_CREATE, 
		NULL, 
		0, 
		sfpath))) 
	{
		PathAppend(sfpath,L"bassmidi.sflist");
	}
	ofstream out;
	out.open(sfpath);
	for (int i=0;i<filecount;++i)
	{
		ListBox_GetText(lbxSoundFonts,i,file_name);
		string utf8 = utf8_from_utf16(file_name);
		out << utf8 << "\n";
	}
	out.close(); 
	MessageBox(NULL,L"SoundFont list set!",L"Notice.",MB_ICONINFORMATION);
}

void load_midisynths(HWND hwnd)
{
	HKEY hKey, hSubKey;
	LONG lResult;
	DWORD device_id = 0;
	TCHAR device_name[MAX_PATH];
	DWORD dwSize;
	DWORD dwType=REG_SZ;
	ZeroMemory(device_name,sizeof(device_name));
	int device_count = midiOutGetNumDevs();
	HWND device_list = GetDlgItem(hwnd, IDC_SYNTHLIST);
	lResult  = RegCreateKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia", &hKey);
	lResult = RegCreateKey(hKey, L"MIDIMap", &hSubKey); 
	RegQueryValueEx(hSubKey, L"szPname", NULL, &dwType,(LPBYTE)device_name, &dwSize);
	RegCloseKey( hKey);
	for (int i = 0; i < device_count; ++i) {
		MIDIOUTCAPS Caps;
		ZeroMemory(&Caps, sizeof(Caps));
		MMRESULT Error = midiOutGetDevCaps(i, &Caps, sizeof(Caps));
		if (Error == MMSYSERR_NOERROR) {
			SendMessage(device_list, CB_ADDSTRING, 0, (LPARAM) Caps.szPname);
		}
	}
	int index = SendMessage(device_list, CB_FINDSTRINGEXACT, -1, (LPARAM)device_name);
	SendMessage(device_list, CB_SETCURSEL, 0, (LPARAM) index);
}


void set_midisynth(HWND hwnd)
{
	HKEY hKey, hSubKey;
	long lRet;
	DWORD dwDisp;
	TCHAR device_name[MAX_PATH];
	memset(device_name,0,MAX_PATH);
	HWND device_list = GetDlgItem(hwnd, IDC_SYNTHLIST);
	int selection = SendMessage(device_list, CB_GETCURSEL, 0, (LPARAM) 0);
	SendMessage(device_list,CB_GETLBTEXT,selection,(LPARAM)&device_name);
	int text_len = (lstrlen(device_name) + 1) * sizeof(TCHAR);
	lRet = RegCreateKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia", &hKey);
	lRet = RegDeleteKey(hKey,L"MIDIMap");
	lRet = RegCreateKey(hKey, L"MIDIMap", &hSubKey); 
	lRet = RegSetValueEx (hSubKey, L"szPname", 0, REG_SZ, (const BYTE*)device_name, (lstrlen(device_name) + 1) * sizeof(TCHAR));
	if (lRet == ERROR_SUCCESS)
	{
		MessageBox(NULL,L"MIDI synth set!",L"Notice.",MB_ICONINFORMATION);
	}
	else
	{
		MessageBox(NULL,L"Can't set MIDI registry key",L"Damn!",MB_ICONSTOP);
	}
	RegCloseKey( hKey);

}