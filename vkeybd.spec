%define ver	0.1.8

Summary: Virtual Keyboard
Name: vkeybd
Version: %ver
Release: 1
Copyright: GPL
Group: Applications/Sound
Source: http://members.tripod.de/iwai/vkeybd-%{ver}.tgz
BuildRoot: /tmp/rpmtest
URL: http//members.tripod.de/iwai/awedrv.html
%description
====================================================================
		VIRTUAL KEYBOARD ver.0.1.9
		Takashi Iwai  <tiwai@suse.de>
====================================================================

This is a virtual keyboard for AWE, MIDI and ALSA drivers.
It's a simple fake of a MIDI keyboard on X-windows system.
Enjoy a music with your mouse and "computer" keyboard :-)

The program requires Tcl7.5/Tk4.1 or newer libraries.
This program supports:
  - ALSA driver ver.0.5.x (default)
  - AWE driver ver.0.4.3 on OSS
  - Raw MIDI device on OSS and ALSA
%prep
%setup
%build
make PREFIX=/usr

%install
make PREFIX="$RPM_BUILD_ROOT"/usr install-all

%clean
rm -rf $RPM_BUILD_ROOT

%files
/usr/bin/*
/usr/man/*
%dir /usr/share/vkeybd

%doc README

%package -n vkeybd-ossonly
====================================================================
		VIRTUAL KEYBOARD ver.0.1.9
		Takashi Iwai  <tiwai@suse.de>
====================================================================

This is a virtual keyboard for AWE, MIDI and ALSA drivers.
It's a simple fake of a MIDI keyboard on X-windows system.
Enjoy a music with your mouse and "computer" keyboard :-)

The program requires Tcl7.5/Tk4.1 or newer libraries.
This program supports:
  - AWE driver ver.0.4.3 on OSS
  - Raw MIDI device on OSS
%description
====================================================================
		VIRTUAL KEYBOARD ver.0.1.9
		Takashi Iwai  <tiwai@suse.de>
====================================================================

This is a virtual keyboard for AWE, MIDI and ALSA drivers.
It's a simple fake of a MIDI keyboard on X-windows system.
Enjoy a music with your mouse and "computer" keyboard :-)

The program requires Tcl7.5/Tk4.1 or newer libraries.
This program supports:
  - ALSA driver ver.0.5.x (default)
  - AWE driver ver.0.4.3 on OSS
  - Raw MIDI device on OSS and ALSA
%prep
%setup
%build
make PREFIX=/usr USE_ALSA=0

%install
make PREFIX="$RPM_BUILD_ROOT"/usr USE_ALSA=0 install-all

%clean
rm -rf $RPM_BUILD_ROOT

%files
/usr/bin/*
/usr/man/*
%dir /usr/share/vkeybd

%doc README

