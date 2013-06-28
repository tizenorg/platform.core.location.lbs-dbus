Name: lbs-dbus
Summary: Dbus interface for Location based service
Version: 0.1.6
Release:    1
Group:      Location/Service
License:    Apache-2.0
Source0:    lbs-dbus-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gobject-2.0)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(gio-unix-2.0)
BuildRequires:  python
BuildRequires:  python-xml

%description
LBS dbus interface

%package -n liblbs-dbus
Summary:    LBS dbus library
Group:      Location/Libraries

%description -n liblbs-dbus
LBS client API library

%package -n liblbs-dbus-devel
Summary:    Telephony client API (devel)
Group:      Development/Location
Requires:   liblbs-dbus = %{version}-%{release}

%description -n liblbs-dbus-devel
LBS client API library (devel)


%prep
%setup -q


%build
%cmake . 


make %{?jobs:-j%jobs}

%install
%make_install

%post -p /sbin/ldconfig -n liblbs-dbus

%postun -p /sbin/ldconfig -n liblbs-dbus


%files -n liblbs-dbus
%manifest liblbs-dbus.manifest
%license LICENSE
%defattr(-,root,root,-)
%{_libdir}/*.so.*
%{_sysconfdir}/dbus-1/system.d/*

%files -n liblbs-dbus-devel
%defattr(-,root,root,-)
%{_includedir}/lbs-dbus/*.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/*.so
