BASSMIDI SoundFont Compatible MIDI Synthesizer Driver for Windows XP/Vista/7
============================================================================
Description
-----------
The BASSMIDI MIDI synthesizer driver is a freeware SoundFont based Windows MIDI
system driver (created by Kode54 and MudLord) for Windows XP, Windows Vista,
and Windows 7. It uses the BASS and BASSMIDI audio libraries by Ian Luck
(Un4seen Developments) as the SoundFont synthesizer, and includes a BASSMIDI
Driver Configuration Utility, as well as a SoundFont Packer Utility.

Features
--------
* Dynamic sample caching (so it's super efficient with RAM usage).
* Unlimited polyphony.
* Support for compressed SoundFonts.
* SoundFont chaining and stacking (using a simple list file "BASSMIDI.SFList").
* Global volume slider to set the master BASSMIDI Driver MIDI playback volume.
* Supports sinc interpolation for SoundFont MIDI synthesis.
* Device selector to select the default Windows MIDI synthesizer device.
* Works on Windows XP, Windows Vista, and Windows 7, including 64-bit versions.
* Extremely low latency (almost zero, depending on CPU speed and operating
  system, and BASSMIDI Driver version); perfect for real-time performance, as
  well as MIDI sequencing (with a MIDI sequencer such as Cakewalk Professional
  v3.01).
* The best sounding SoundFont compatible software synthesizer, hands down...
  with the closest sounding MIDI playback to real E-mu SoundFont hardware
  available!

* Support for the following SF2Pack SoundFonts (with lossy or lossless
  compressed samples):
   FLAC
   LAME (Version 2)
   MusePack (Q5)
   Opus
   Wavpack (Lossy, Low)
   Wavpack (Lossy, Average)
   Wavpack (Lossy, HQ)
   Wavpack (Lossless)
   Vorbis (Q3)

* Support for the following MIDI controllers and events:
   MIDI note events.
   MIDI program events.
   Master volume level.
   Channel pressure.
   Pitch wheel.
   Bank change MSB (CC#0).
   Modulation (CC#1).
   Portamento (CC#65, CC#84, and CC#5).
   Volume (CC#7).
   Panning (CC#10).
   Expression (CC#11).
   Sustain pedal (CC#64).
   Soft pedal (CC#67).
   Low-pass filter resonance (CC#71 or NRPN 121h).
   Release time (CC#72 or NRPN 166h).
   Attack time (CC#73 or NRPN 163h).
   Low-pass filter cut-off (CC#74 or NRPN 120h).
   Reverb send level (CC#91).
   Chorus send level (CC#93).
   Stop all sounds (CC#120).
   Reset all controllers (CC#121).
   Release all keys (CC#123).
   Mono/poly mode (CC#126 and CC#127, respectively).
   Pitch wheel range (RPN 0).
   Fine tuning (RPN 1).
   Coarse tuning (RPN 2).

* Support for the following MIDI events (if GS or GM2 mode is activated):
   Drum key low-pass filter cut-off (NRPN 14knh).
   Drum key low-pass filter resonance (NRPN 15knh).
   Drum key coarse tune (NRPN 18knh).
   Drum key fine tune (NRPN 19knh).
   Drum key volume level (NRPN 1Aknh).
   Drum key panning (NRPN 1Cknh).
   Drum key reverb send level (NRPN 1Dknh).
   Drum key chorus send level (NRPN 1Eknh).

The BASSMIDI SoundFont compatible MIDI synthesizer driver supports GM, GM2, GS,
and XG reset System Exclusive messages; as well as the drum channel enabling
features of GS and XG when switched into those modes.

The driver also supports GS and XG reverb preset control messages (e.g. "Hall
1", "Plate", "Tunnel", etc...) for adjusting reverb time, reverb delay, reverb
low-pass cut-off, reverb high-pass cut-off, and reverb level; as well as chorus
preset control messages (e.g. "Chorus 1", "Celeste 1", "Flanger 1", etc...) for
adjusting chrorus delay, chorus depth, chorus rate, chorus feedback, chorus
level, and chorus to reverb send level.

Installation
------------
First-Time Installation:

1) Download the latest released version of the BASSMIDI Driver from
   http://www.mudlord.info/bassmididrv/bassmididrv.exe.

2) Run the installer (BASSMIDIDrv.exe). The installer will register the
   BASSMIDI Driver with the system.

3) Configure the desired SoundFont usage using the included BASSMIDI Driver
   Configuration Utility (BASSMIDIDrvcfg.exe).

Upgrade Installation:

1) Download the newly released version of the BASSMIDI Driver from
   http://www.mudlord.info/bassmididrv/bassmididrv.exe.

2) Run the new installer (BASSMIDIDrv.exe). You will be prompted to uninstall
   the previous version of the BASSMIDI Driver.

3) Run the new installer again (BASSMIDIDrv.exe), and follow the instructions
   for the "First-Time Installation" section above.

Announcements and Discussion
----------------------------
http://www.hydrogenaudio.org/forums/index.php?showtopic=87639
http://www.vgmusic.com/phpBB3/viewtopic.php?f=16&t=13967
http://www.un4seen.com/forum/?topic=5337.0

Additional Discussion
---------------------
http://www.doomworld.com/vb/everything-else/54851-bassmidi-soundfont-compatible-midi-synthesiser-driver-for-windows-xp-vista-7
http://my.noteworthysoftware.com/?topic=7672.0
http://queststudios.com/smf/index.php/topic,3329.0.html
http://www.synthesiagame.com/forum/viewtopic.php?f=3&t=2955
http://vogons.zetafleet.com/viewtopic.php?t=27975
http://forum.zdoom.org/viewtopic.php?f=4&t=29290

Download (Release Versions)
---------------------------
http://www.mudlord.info
http://www.mudlord.info/bassmididrv
http://www.mudlord.info/bassmididrv/bassmididrv.exe

Download (Previous Versions)
----------------------------
http://www.mudlord.info
http://www.mudlord.info/bassmididrv
http://www.mudlord.info/bassmididrv/archive

Download (Source Code)
----------------------
https://github.com/mudlord/BASSMIDI-Driver
https://github.com/mudlord/BASSMIDI-Driver/zipball/master

Documentation
-------------
http://www.mudlord.info/bassmididrv/
http://www.mudlord.info/bassmididrv/BASSMIDI_Driver_MIDI_Implementation_Chart.htm
http://www.mudlord.info/bassmididrv/BASSMIDI_Driver_Installation_and_Configuration.htm

https://raw.github.com/mudlord/BASSMIDI-Driver/master/README.txt
https://raw.github.com/mudlord/BASSMIDI-Driver/master/CHANGES.txt
https://raw.github.com/mudlord/BASSMIDI-Driver/master/LICENSE.txt

Legalities and Credits
----------------------
©Copyright 2011-2012, Brad Miller and Chris Moeller - All Rights Reserved

BASSMIDI Driver and SoundFont Packer Utility by Chris "Kode54" Moeller.
Installer and BASSMIDI Driver Configuration Utility by Brad "MudLord" Miller.
