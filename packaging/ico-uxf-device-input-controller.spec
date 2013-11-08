Name:       ico-uxf-device-input-controller
Summary:    Device Input Controller
Version:    0.9.03
Release:    1.1
Group:      System/GUI
License:    Apache License, Version 2.0
URL:        ""
Source0:    %{name}-%{version}.tar.bz2

BuildRequires: pkgconfig(wayland-client) >= 1.2.0
BuildRequires: mesa-devel
BuildRequires: pkgconfig(egl)
BuildRequires: pkgconfig(glesv2)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: ico-uxf-weston-plugin-devel >= 0.9.07
BuildRequires: ico-uxf-utilities-devel >= 0.9.01
Requires: weston >= 1.2.2
Requires: ico-uxf-weston-plugin >= 0.9.07
Requires: ico-uxf-utilities >= 0.9.01

%description
Device Input Controller for ico-uxf-weston-plugin(Multi Input Manager)

%prep
%setup -q -n %{name}-%{version}

%build
%autogen

%configure
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%make_install

# configurations
%define ictl_conf /opt/etc/ico/device-input-controller
mkdir -p %{buildroot}%{ictl_conf}
mkdir -p %{buildroot}%{_unitdir_user}
install -m 0644 settings/drivingforcegt.conf %{buildroot}%{ictl_conf}
install -m 0644 settings/g27racingwheel.conf %{buildroot}%{ictl_conf}
install -m 0755 settings/set_daynight.sh %{buildroot}%{ictl_conf}
install -m 644 settings/ico-device-input-controller.service %{buildroot}%{_unitdir_user}/ico-device-input-controller.service

%files
%defattr(-,root,root,-)
%{_bindir}/ico_dic-gtforce
%{ictl_conf}/drivingforcegt.conf
%{ictl_conf}/g27racingwheel.conf
%{ictl_conf}/set_daynight.sh
/usr/lib/systemd/user/ico-device-input-controller.service
