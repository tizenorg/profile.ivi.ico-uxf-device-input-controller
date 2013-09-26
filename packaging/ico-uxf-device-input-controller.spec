Name:       ico-uxf-device-input-controller
Summary:    Device Input Controller
Version:    0.9.02
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
BuildRequires: ico-uxf-weston-plugin-devel >= 0.9.05
BuildRequires: ico-uxf-utilities-devel >= 0.2.04
Requires: weston >= 1.2.1
Requires: ico-uxf-weston-plugin >= 0.9.05
Requires: ico-uxf-utilities >= 0.2.04

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
install -m 0644 settings/drivingforcegt.conf %{buildroot}%{ictl_conf}
install -m 0644 settings/g27racingwheel.conf %{buildroot}%{ictl_conf}
install -m 0755 settings/set_daynight.sh %{buildroot}%{ictl_conf}
install -m 0755 settings/set_navi_busguide.sh %{buildroot}%{ictl_conf}
install -m 0755 settings/set_navi_destination.sh %{buildroot}%{ictl_conf}
install -d %{buildroot}/%{_unitdir_user}/weston.target.wants
install -m 644 settings/ico-device-input-controller.service %{buildroot}%{_unitdir_user}/ico-device-input-controller.service
install -m 644 settings/ico-dic-wait-weston-ready.path %{buildroot}%{_unitdir_user}/ico-dic-wait-weston-ready.path
ln -sf ../ico-dic-wait-weston-ready.path %{buildroot}/%{_unitdir_user}/weston.target.wants/

%files
%defattr(-,root,root,-)
%{_bindir}/ico_dic-gtforce
%{ictl_conf}/drivingforcegt.conf
%{ictl_conf}/g27racingwheel.conf
%{ictl_conf}/set_daynight.sh
%{ictl_conf}/set_navi_busguide.sh
%{ictl_conf}/set_navi_destination.sh
/usr/lib/systemd/user/ico-device-input-controller.service
/usr/lib/systemd/user/ico-dic-wait-weston-ready.path
/usr/lib/systemd/user/weston.target.wants/ico-dic-wait-weston-ready.path
