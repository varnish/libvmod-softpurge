Summary: Soft purge support for Varnish VCL
Name: vmod-softpurge
Version: 0.2
Release: 1%{?dist}
License: BSD
Group: System Environment/Daemons
Source0: libvmod-softpurge.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires: varnish >= 4.0.2
BuildRequires: make
BuildRequires: python-docutils
BuildRequires: varnish >= 4.0.2
BuildRequires: varnish-libs-devel >= 4.0.2

%description
Soft purge support for Varnish VCL

%prep
%setup -n libvmod-softpurge-trunk

%build
%configure --prefix=/usr
make
make check

%install
make install DESTDIR=%{buildroot}
mv %{buildroot}/usr/share/doc/lib%{name} %{buildroot}/usr/share/doc/%{name}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_libdir}/varnis*/vmods/
%doc /usr/share/doc/%{name}/*
%{_mandir}/man?/*

%changelog
* Tue Nov 14 2012 Lasse Karstensen <lasse@varnish-software.com> - 0.1-0.20121114
- Initial version.
