Name:       harbour-nemoai
Summary:    AI Companion for Sailfish OS
Version:    0.1.0
Release:    1
License:    MIT
URL:        https://github.com/silly82/sailfish-ai-companion
Source0:    %{name}-%{version}.tar.bz2

Requires:   sailfishsilica-qt5 >= 0.10.9
Requires:   sailfishsecretsdaemon
Requires:   nemo-qml-plugin-contextkit-qt5
Requires:   nemo-qml-plugin-notifications-qt5

BuildRequires: pkgconfig(sailfishapp) >= 1.0.2
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: desktop-file-utils

%description
Nativer AI-Begleiter fuer Sailfish OS. Chat ueber OpenRouter mit
Systemintegration ueber Harbour-konforme APIs.

%prep
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5 CONFIG+=harbour
%qtc_make %{?_smp_mflags}

%install
%qmake5_install

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
