Name:       sailfishai
Summary:    AI Companion for Sailfish OS (full access)
Version:    0.5.1
Release:    1
License:    MIT
URL:        https://github.com/silly82/sailfish-ai-companion
Source0:    %{name}-%{version}.tar.bz2

Requires:   sailfishsilica-qt5 >= 0.10.9
Requires:   libcommhistory-qt5
Requires:   mkcal-qt5
Recommends: sailfishai-llama

BuildRequires: pkgconfig(sailfishapp) >= 1.0.2
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(commhistory-qt5)
BuildRequires: pkgconfig(libmkcal-qt5)
BuildRequires: pkgconfig(KF5CalendarCore)

%description
Vollzugriffs-Variante fuer OpenRepos. Laeuft unsandboxed und bietet
zusaetzlich SMS-, Kalender- und Dateisystemzugriff sowie lokale Inference
ueber das Paket sailfishai-llama.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 CONFIG+=fullaccess
%make_build

%install
%qmake5_install

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
