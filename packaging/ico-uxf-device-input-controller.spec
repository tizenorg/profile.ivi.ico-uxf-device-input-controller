Name:       ico-uxf-device-input-controller
Summary:    Device Input Controller
Version:    0.5.05
Release:    1.1
Group:      System/GUI
License:    Apache License, Version 2.0
URL:        ""
Source0:    %{name}-%{version}.tar.bz2

BuildRequires: pkgconfig(wayland-client) >= 1.0
BuildRequires: pkgconfig(wayland-egl)
BuildRequires: pkgconfig(egl)
BuildRequires: pkgconfig(glesv2)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: ico-uxf-weston-plugin-devel >= 0.5.05
Requires: weston >= 1.0
Requires: ico-uxf-weston-plugin >= 0.5.05

%description
Device Input Controller for ico-uxf-weston-plugin(Multi Input Manager)

%prep
%setup -q -n %{name}-%{version}

%build
autoreconf --install

%autogen --prefix=/usr

%configure
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%make_install

# configurations
%define ictl_conf /opt/etc/ico-uxf-device-input-controller
mkdir -p %{buildroot}%{ictl_conf}
install -m 0644 joystick_gtforce.conf %{buildroot}%{ictl_conf}
install -m 0644 egalax_calibration.conf %{buildroot}%{ictl_conf}

%files
%defattr(-,root,root,-)
%{_bindir}/ico_ictl-joystick_gtforce
%{_bindir}/ico_ictl-touch_egalax
%{_bindir}/ico_ictl-egalax_calibration
%{ictl_conf}/joystick_gtforce.conf
%{ictl_conf}/egalax_calibration.conf

