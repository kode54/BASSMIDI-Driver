#if !defined(AFX_VIEWS_H__20020629_8D64_963C_A351_0080AD509054__INCLUDED_)
#define AFX_VIEWS_H__20020629_8D64_963C_A351_0080AD509054__INCLUDED_

#include <iostream>
#include <fstream> 

#include "utf8conv.h"
using namespace std;
using namespace utf8util;
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
class CView1 : public CDialogImpl<CView1>
{
	CListBox listbox;
	CButton addsf,removesf, upsf,downsf,applysf;
public:
   enum { IDD = IDD_MAIN };
   BEGIN_MSG_MAP(CView1)
	   MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialogView1)
	   COMMAND_ID_HANDLER(IDC_ADDSF,OnButtonAdd)
	   COMMAND_ID_HANDLER(IDC_RMSF,OnButtonRemove)
	   COMMAND_ID_HANDLER(IDC_DOWNSF,OnButtonDown)
	   COMMAND_ID_HANDLER(IDC_UPSF,OnButtonUp)
	   COMMAND_ID_HANDLER(IDC_SFAPPLY,OnButtonApply)
   END_MSG_MAP()

	LRESULT OnInitDialogView1(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		listbox = GetDlgItem(IDC_SFLIST);
		addsf = GetDlgItem(IDC_ADDSF);
		removesf = GetDlgItem(IDC_RMSF);
		upsf = GetDlgItem(IDC_UPSF);
		downsf = GetDlgItem(IDC_DOWNSF);
		applysf = GetDlgItem(IDC_SFAPPLY);

		TCHAR sfpath[MAX_PATH];
		if(SUCCEEDED(SHGetFolderPath(NULL, 
			CSIDL_WINDOWS|CSIDL_FLAG_CREATE, 
			NULL, 
			0, 
			sfpath))) 
		{
			PathAppend(sfpath,L"bassmidi.sflist");
			read_sflist(sfpath);
		}
		return TRUE;
	}

	LRESULT OnButtonAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
	{
		TCHAR szFileName[MAX_PATH];
		LPCTSTR sFiles = 
			L"SoundFonts (*.sf2)\0*.sf2\0"
			L"SoundFont list files (*.sflist)\0*.sflist\0"
			L"All Files (*.*)\0*.*\0\0";
	    CFileDialog dlg( TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, sFiles);
		
		if (dlg.DoModal() == IDOK)
		{
			lstrcpy(szFileName,dlg.m_szFileName);
			const TCHAR * ext = _tcsrchr( szFileName, _T('.') );
			if ( ext ) ext++;
			if ( !_tcsicmp( ext, _T("sf2")))
			{
				listbox.AddString(szFileName);
			}
			else if ( !_tcsicmp( ext, _T("sflist") ))
			{
				read_sflist(szFileName);
			}
			// do stuff
		}
		return 0;

	}

	LRESULT OnButtonRemove( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
	{
		int selectedindex = listbox.GetCurSel();
		listbox.DeleteString(selectedindex);
		return 0;

	}

	LRESULT OnButtonDown( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
	{
		TCHAR SelectedItem[MAX_PATH];
		int amount_items = listbox.GetCount();
		amount_items--;
		int selectedindex = listbox.GetCurSel();
		if (selectedindex != amount_items && selectedindex != -1)
		{
			listbox.GetText(selectedindex,SelectedItem);
			listbox.DeleteString(selectedindex);
			listbox.InsertString(selectedindex+1,SelectedItem);
			listbox.SetCurSel(selectedindex+1);
		}
		return 0;

	}

	LRESULT OnButtonUp( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
	{
		int amount_items = listbox.GetCount();
		TCHAR SelectedItem[MAX_PATH];
		int selectedindex = listbox.GetCurSel();
		if (selectedindex != 0)
		{
			listbox.GetText(selectedindex,SelectedItem);
			listbox.DeleteString(selectedindex);
			listbox.InsertString(selectedindex-1,SelectedItem);
			listbox.SetCurSel(selectedindex-1);
		}
		return 0;

	}

	LRESULT OnButtonApply( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
	{
		write_sflist();
		return 0;

	}

   void read_sflist(TCHAR * sfpath)
   {
	   int font_count;
	   listbox.ResetContent();
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
				  listbox.AddString(cr);
			   }
			   fclose( fl );
		   }
	   }
   }

   void write_sflist()
   {
	   int file_count = listbox.GetCount();
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
	   for (int i=0;i<file_count;++i)
	   {
		   listbox.GetText(i,file_name);
		   string utf8 = utf8_from_utf16(file_name);
		   out << utf8 << "\n";
	   }
	   out.close(); 
	   MessageBox(L"SoundFont list set!",L"Notice.",MB_ICONINFORMATION);
   }
};




class CView2 : public CDialogImpl<CView2>
{
	CComboBox synthlist;
	CTrackBarCtrl slider_volume;
	CButton apply;
	CButton apply2;
	CButton sinc_inter;


public:
   enum { IDD = IDD_ADVANCED };
   BEGIN_MSG_MAP(CView1)
	   MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialogView2)
	    COMMAND_ID_HANDLER(IDC_SNAPPLY,OnButtonApply)
		COMMAND_ID_HANDLER(IDC_APPLY,OnButtonApply2)
   END_MSG_MAP()

   LRESULT OnInitDialogView2(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
	   slider_volume = GetDlgItem(IDC_VOLUME);
	   slider_volume.SetRange(0,10000);
	   synthlist = GetDlgItem(IDC_SYNTHLIST);
	   apply = GetDlgItem(IDC_SNAPPLY);
	   apply2 = GetDlgItem(IDC_APPLY);
	   sinc_inter = GetDlgItem(IDC_SINC);
	   load_settings();
	   load_midisynths();
	   return TRUE;
   }

   LRESULT OnButtonApply( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
   {
	   set_midisynth();
	   return 0;

   }

   LRESULT OnButtonApply2( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
   {
	   save_settings();
	   return 0;

   }

   void load_settings()
   {
		long lResult;
		DWORD volume;
		DWORD sinc;
		CRegKeyEx reg;
		lResult = reg.Create(HKEY_LOCAL_MACHINE, L"Software\\BASSMIDI Driver");
		reg.QueryDWORDValue( L"volume",volume);
		reg.QueryDWORDValue( L"sinc",volume);
		reg.Close();
		slider_volume.SetPos(volume);
		sinc_inter.SetCheck(sinc);
   }

   void save_settings()
   {
	   DWORD volume = slider_volume.GetPos();
	   DWORD sinc = sinc_inter.GetCheck();
	   HKEY hKey, hSubKey;
	   long lResult;
	   CRegKeyEx reg;
	   lResult = reg.Create(HKEY_LOCAL_MACHINE, L"Software\\BASSMIDI Driver");
	   lResult = reg.SetDWORDValue(L"volume",volume);
	   lResult = reg.SetDWORDValue(L"sinc",sinc);

	   if (lResult == ERROR_SUCCESS)
	   {
		   MessageBox(L"Settings saved!",L"Notice.",MB_ICONINFORMATION);
	   }
	   else
	   {
		   MessageBox(L"Can't save settings",L"Damn!",MB_ICONSTOP);
	   }

	   reg.Close();
   }

   void set_midisynth()
   {
	   CRegKeyEx reg;
	   CRegKeyEx subkey;
	   CString device_name;
	   long lRet;
	   int selection = synthlist.GetCurSel();
	   int n = synthlist.GetLBTextLen(selection);
	   synthlist.GetLBText(selection,device_name.GetBuffer(n));
	   lRet = reg.Create(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia");
	   lRet = reg.DeleteSubKey(L"MIDIMap");
	   lRet = subkey.Create(reg,L"MIDIMap");
	   lRet = subkey.SetStringValue(L"szPname",device_name);
	   if (lRet == ERROR_SUCCESS)
	   {
		   MessageBox(L"MIDI synth set!",L"Notice.",MB_ICONINFORMATION);
	   }
	   else
	   {
		   MessageBox(L"Can't set MIDI registry key",L"Damn!",MB_ICONSTOP);
	   }
	   device_name.ReleaseBuffer(n);
	   subkey.Close();
	   reg.Close();
   }

	void load_midisynths()
	{
		LONG lResult;
		CRegKeyEx reg;
		CString device_name;
		ULONG size;
		lResult = reg.Create(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia\\MIDIMap");
		reg.QueryStringValue(L"szPname",device_name.GetBuffer(size),&size);
		reg.Close();
		int device_count = midiOutGetNumDevs();
		for (int i = 0; i < device_count; ++i) {
			MIDIOUTCAPS Caps;
			ZeroMemory(&Caps, sizeof(Caps));
			MMRESULT Error = midiOutGetDevCaps(i, &Caps, sizeof(Caps));
			if (Error != MMSYSERR_NOERROR)
				continue;
			synthlist.AddString(Caps.szPname);
		}
		int index = 0;
		index = synthlist.FindStringExact(-1,device_name);
		if (index == CB_ERR) index = 0;
		synthlist.SetCurSel(index);
		device_name.ReleaseBuffer(size);
	}
};



#endif // !defined(AFX_VIEWS_H__20020629_8D64_963C_A351_0080AD509054__INCLUDED_)

