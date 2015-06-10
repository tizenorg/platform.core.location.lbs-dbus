Name:		liblbs-dbus
Summary:	DBus interface for Location Based Service
Version:	0.3.2
Release:	1
Group:		Location/Libraries
License:	Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	%{name}.manifest
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:	cmake
BuildRequires:	pkgconfig(glib-2.0)
BuildRequires:	pkgconfig(gobject-2.0)
BuildRequires:	pkgconfig(dlog)
BuildRequires:	pkgconfig(gio-2.0)
BuildRequires:	pkgconfig(gio-unix-2.0)
BuildRequires:	pkgconfig(capi-appfw-app-manager)
BuildRequires:	pkgconfig(capi-appfw-package-manager)
BuildRequires:	pkgconfig(pkgmgr-info)

%description
DBus interface for Location Based Service
The package provides IPC between LBS Server and Location Manager.

%package devel
Summary:	LBS DBus Library (devel)
Group:		Location/Development
Requires:	%{name} = %{version}-%{release}

%description devel
LBS DBus Library (devel)
The package includes header files and pkgconfig file of LBS DBus interface.

%prep
%setup -q
cp %{SOURCE1001} .

%build
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"

export CFLAGS+=" -Wno-unused-local-typedefs "
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} \

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest %{name}.manifest
%license LICENSE
%defattr(-,root,root,-)
%{_libdir}/*.so.*
%{_prefix}/etc/dbus-1/system.d/*

%files devel
%defattr(-,root,root,-)
%{_includedir}/lbs-dbus/*.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/*.so
