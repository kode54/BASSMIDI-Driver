!include "x64.nsh"
!include MUI2.nsh
!include WinVer.nsh
; The name of the installer
Name "BASSMIDI System Synth"

!ifdef INNER
  !echo "Inner invocation"
  OutFile "$%temp%\tempinstaller.exe"
  SetCompress off
!else
  !echo "Outer invocation"

  !system "$\"${NSISDIR}\makensis$\" /DINNER bassmididrv.nsi" = 0

  !system "$%TEMP%\tempinstaller.exe" = 2

  !system "e:\signit.bat $%TEMP%\bassmididrvuninstall.exe" = 0

  ; The file to write
  OutFile "bassmididrv.exe"

  ; Request application privileges for Windows Vista
  RequestExecutionLevel admin
  SetCompressor /solid lzma 
!endif

;--------------------------------
; Pages
!insertmacro MUI_PAGE_WELCOME
Page Custom LockedListShow
!insertmacro MUI_PAGE_INSTFILES
UninstPage Custom un.LockedListShow
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

!macro DeleteOnReboot Path
  IfFileExists `${Path}` 0 +3
    SetFileAttributes `${Path}` NORMAL
    Delete /rebootok `${Path}`
!macroend
!define DeleteOnReboot `!insertmacro DeleteOnReboot`

Function LockedListShow
  ${If} ${RunningX64}
    File /oname=$PLUGINSDIR\LockedList64.dll `${NSISDIR}\Plugins\LockedList64.dll`
  ${EndIf}
  !insertmacro MUI_HEADER_TEXT `File in use check` `Drive use check`
  LockedList::AddModule \bassmididrv.dll
  LockedList::Dialog  /autonext   
  Pop $R0
FunctionEnd
Function un.LockedListShow
  ${If} ${RunningX64}
    File /oname=$PLUGINSDIR\LockedList64.dll `${NSISDIR}\Plugins\LockedList64.dll`
  ${EndIf}
  !insertmacro MUI_HEADER_TEXT `File in use check` `Drive use check`
  LockedList::AddModule \bassmididrv.dll
  LockedList::Dialog  /autonext   
  Pop $R0
FunctionEnd
;--------------------------------
Function .onInit

!ifdef INNER
  WriteUninstaller "$%TEMP%\bassmididrvuninstall.exe"
  Quit
!endif

ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth" "UninstallString"
  StrCmp $R0 "" done
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "The MIDI driver is already installed. $\n$\nClick `OK` to remove the \
  previous version or `Cancel` to cancel this upgrade." \
  IDOK uninst
  Abort
;Run the uninstaller
uninst:
  ClearErrors
  Exec $R0
  Abort
done:
   MessageBox MB_YESNO "This will install the BASSMIDI System Synth. Continue?" IDYES NoAbort
     Abort ; causes installer to quit.
   NoAbort:
 FunctionEnd
; The stuff to install
Section "Needed (required)"
  SectionIn RO
  ; Copy files according to whether its x64 or not.
   DetailPrint "Copying driver and synth..."
   ${If} ${RunningX64}
   SetOutPath "$WINDIR\SysWow64\bassmididrv"
   File output\bass.dll 
   File output\bassflac.dll
   File output\basswv.dll
   File output\bassopus.dll   
   File output\bass_mpc.dll 
   File output\bassmidi.dll 
   File output\bassmididrv.dll 
   File output\bassmididrvcfg.exe
   File output\sfpacker.exe
!ifndef INNER
   File $%TEMP%\bassmididrvuninstall.exe
!endif
   SetOutPath "$WINDIR\SysNative\bassmididrv"
   File output\64\bass.dll
   File output\64\bassflac.dll
   File output\64\basswv.dll
   File output\64\bassopus.dll
   File output\64\bass_mpc.dll
   File output\64\bassmidi.dll
   File output\64\bassmididrv.dll
   ;check if already installed
   StrCpy  $1 "0"
LOOP1:
  ;k not installed, do checks
  IntOp $1 $1 + 1
  ClearErrors
  ReadRegStr $0  HKLM "Software\Microsoft\Windows NT\CurrentVersion\Drivers32" "midi$1"
  StrCmp $0 "" INSTALLDRIVER NEXTCHECK
  NEXTCHECK:
  StrCmp $0 "wdmaud.drv" 0  NEXT1
INSTALLDRIVER:
  WriteRegStr HKLM "Software\Microsoft\Windows NT\CurrentVersion\Drivers32" "midi$1" "bassmididrv\bassmididrv.dll"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth\Backup" \
      "MIDI" "midi$1"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth\Backup" \
      "MIDIDRV" "$0"
  Goto REGDONE32
NEXT1:
  StrCmp $1 "9" 0 LOOP1
  ; check if 64 is installed
REGDONE32:
  SetRegView 64
  StrCpy  $1 "0"
LOOP2:
  ;k not installed, do checks
  IntOp $1 $1 + 1
  ClearErrors
  ReadRegStr $0  HKLM "Software\Microsoft\Windows NT\CurrentVersion\Drivers32" "midi$1"
  StrCmp $0 "" INSTALLDRIVER2 NEXTCHECK2
  NEXTCHECK2:
  StrCmp $0 "wdmaud.drv" 0  NEXT2
INSTALLDRIVER2:
  WriteRegStr HKLM "Software\Microsoft\Windows NT\CurrentVersion\Drivers32" "midi$1" "bassmididrv\bassmididrv.dll"
  SetRegView 32
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth\Backup" \
      "MIDI64" "midi$1"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth\Backup" \
      "MIDIDRV64" "$0"
  Goto REGDONE
NEXT2:
  StrCmp $1 "9" 0 LOOP2
   ${Else}
   SetOutPath "$WINDIR\System32\bassmididrv"
   File output\bass.dll 
   File output\bassflac.dll
   File output\basswv.dll
   File output\bassopus.dll   
   File output\bass_mpc.dll 
   File output\bassmidi.dll 
   File output\bassmididrv.dll 
   File output\bassmididrvcfg.exe
   File output\sfpacker.exe
   ;check if already installed
   StrCpy  $1 "0"
LOOP3:
  ;k not installed, do checks
  IntOp $1 $1 + 1
  ClearErrors
  ReadRegStr $0  HKLM "Software\Microsoft\Windows NT\CurrentVersion\Drivers32" "midi$1"
  StrCmp $0 "" INSTALLDRIVER3 NEXTCHECK3
  NEXTCHECK3:
  StrCmp $0 "wdmaud.drv" 0  NEXT3
INSTALLDRIVER3:
  WriteRegStr HKLM "Software\Microsoft\Windows NT\CurrentVersion\Drivers32" "midi$1" "bassmididrv\bassmididrv.dll"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth\Backup" \
      "MIDI" "midi$1"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth\Backup" \
      "MIDIDRV" "$0"
  Goto REGDONE
NEXT3:
  StrCmp $1 "9" 0 LOOP3
   ${EndIf}
REGDONE:
  ; Write the uninstall keys
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth" "DisplayName" "BASSMIDI System Synth"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth" "NoRepair" 1
  WriteRegDWORD HKLM "Software\BASSMIDI Driver" "volume" "10000"
  CreateDirectory "$SMPROGRAMS\BASSMIDI System Synth"
 ${If} ${RunningX64}
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth" "UninstallString" '"$WINDIR\SysWow64\bassmididrv\bassmididrvuninstall.exe"'
   WriteRegStr HKLM "Software\BASSMIDI Driver" "path" "$WINDIR\SysWow64\bassmididrv"
   CreateShortCut "$SMPROGRAMS\BASSMIDI System Synth\Uninstall.lnk" "$WINDIR\SysWow64\bassmididrv\bassmididrvuninstall.exe" "" "$WINDIR\SysWow64\bassmididrvuninstall.exe" 0
   CreateShortCut "$SMPROGRAMS\BASSMIDI System Synth\SoundFont Packer.lnk" "$WINDIR\SysWow64\bassmididrv\sfpacker.exe" "" "$WINDIR\SysWow64\sfpacker.exe" 0
   CreateShortCut "$SMPROGRAMS\BASSMIDI System Synth\Configure BASSMIDI Driver.lnk" "$WINDIR\SysWow64\bassmididrv\bassmididrvcfg.exe" "" "$WINDIR\SysWow64\bassmididrv\bassmididrvcfg.exe" 0
   ${Else}
   WriteRegStr HKLM "Software\BASSMIDI Driver" "path" "$WINDIR\System32\bassmididrv"
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth" "UninstallString" '"$WINDIR\System32\bassmididrv\bassmididrvuninstall.exe"'
   CreateShortCut "$SMPROGRAMS\BASSMIDI System Synth\Uninstall.lnk" "$WINDIR\System32\bassmididrv\bassmididrvuninstall.exe" "" "$WINDIR\System32\bassmididrv\bassmididrvuninstall.exe" 0
   CreateShortCut "$SMPROGRAMS\BASSMIDI System Synth\SoundFont Packer.lnk" "$WINDIR\System32\bassmididrv\sfpacker.exe" "" "$WINDIR\System32\sfpacker.exe" 0
   CreateShortCut "$SMPROGRAMS\BASSMIDI System Synth\Configure BASSMIDI Driver.lnk" "$WINDIR\System32\bassmididrv\bassmididrvcfg.exe" "" "$WINDIR\System32\bassmididrv\bassmididrvcfg.exe" 0
   ${EndIf}
   MessageBox MB_OK "Installation complete! Use the driver configuration tool which is in the 'BASSMIDI System Synth' program shortcut directory to configure the driver."

SectionEnd
;--------------------------------

; Uninstaller

!ifdef INNER
Section "Uninstall"
   ; Remove registry keys
    ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth\Backup" \
       "MIDI"
  ReadRegStr $1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth\Backup" \
      "MIDIDRV"
  WriteRegStr HKLM "Software\Microsoft\Windows NT\CurrentVersion\Drivers32" "$0" "$1"
 ${If} ${RunningX64}
     ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth\Backup" \
       "MIDI64"
  ReadRegStr $1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth\Backup" \
      "MIDIDRV64"
  SetRegView 64
  WriteRegStr HKLM "Software\Microsoft\Windows NT\CurrentVersion\Drivers32" "$0" "$1"
  SetRegView 32
${EndIf}
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASSMIDI System Synth"
  DeleteRegKey HKLM "Software\BASSMIDI Driver"
  RMDir /r "$SMPROGRAMS\BASSMIDI System Synth"
 ${If} ${RunningX64}
 ${If} ${AtLeastWinVista}
  RMDir /r  "$WINDIR\SysWow64\bassmididrv"
  RMDir /r  "$WINDIR\SysNative\bassmididrv"
${Else}
  MessageBox MB_OK "Note: The uninstaller will reboot your system to remove drivers."
  ${DeleteOnReboot} $WINDIR\SysWow64\bassmididrv\bass.dll
  ${DeleteOnReboot} $WINDIR\SysWow64\bassmididrv\bassmidi.dll
  ${DeleteOnReboot} $WINDIR\SysWow64\bassmididrv\bassmididrv.dll
   ${DeleteOnReboot} $WINDIR\SysWow64\bassmididrv\bass_mpc.dll 
  ${DeleteOnReboot} $WINDIR\SysWow64\bassmididrv\bassmididrvuninstall.exe
  ${DeleteOnReboot} $WINDIR\SysWow64\bassmididrv\bassmididrvcfg.exe
   ${DeleteOnReboot} $WINDIR\SysWow64\bassmididrv\bassflac.dll
   ${DeleteOnReboot} $WINDIR\SysWow64\bassmididrv\basswv.dll
   ${DeleteOnReboot} $WINDIR\SysWow64\bassmididrv\bassopus.dll
   ${DeleteOnReboot} $WINDIR\SysWow64\bassmididrv\sfpacker.exe
  ${DeleteOnReboot} $WINDIR\SysNative\bassmididrv\bass.dll
  ${DeleteOnReboot} $WINDIR\SysNative\bassmididrv\bassmidi.dll
  ${DeleteOnReboot} $WINDIR\SysNative\bassmididrv\bassmididrv.dll
   ${DeleteOnReboot} $WINDIR\SysNative\bassmididrv\bass_mpc.dll 
   ${DeleteOnReboot} $WINDIR\SysNative\bassmididrv\bassflac.dll
   ${DeleteOnReboot} $WINDIR\SysNative\bassmididrv\basswv.dll
   ${DeleteOnReboot} $WINDIR\SysNative\bassmididrv\bassopus.dll
  Reboot
${Endif}
${Else}
${If} ${AtLeastWinVista}
  RMDir /r  "$WINDIR\System32\bassmididrv"
${Else}
  MessageBox MB_OK "Note: The uninstaller will reboot your system to remove drivers."
  ${DeleteOnReboot} $WINDIR\System32\bassmididrv\bass.dll
  ${DeleteOnReboot} $WINDIR\System32\bassmididrv\bassmidi.dll
  ${DeleteOnReboot} $WINDIR\System32\bassmididrv\bassmididrv.dll
  ${DeleteOnReboot} $WINDIR\System32\bassmididrv\bass_mpc.dll 
  ${DeleteOnReboot} $WINDIR\System32\bassmididrv\bassmididrvuninstall.exe
  ${DeleteOnReboot} $WINDIR\System32\bassmididrv\bassmididrvcfg.exe
  ${DeleteOnReboot} $WINDIR\System32\bassmididrv\bassflac.dll
  ${DeleteOnReboot} $WINDIR\System32\bassmididrv\basswv.dll
  ${DeleteOnReboot} $WINDIR\System32\bassmididrv\bassopus.dll
  ${DeleteOnReboot} $WINDIR\System32\bassmididrv\sfpacker.exe
  Reboot
${Endif}
${EndIf}
SectionEnd
!endif
