Summary: varnish-vmod-softpurge
Name: varnish-vmod-softpurge
Version: 0.1
Release: 1%{?dist}
License: BSD
Group: System Environment/Daemons
Source0: ./libvmod-softpurge.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires: varnish > 3.0
BuildRequires: make, autoconf, automake, libtool, python-docutils

%description
Soft purge VMOD for Varnish

%prep
%setup -n libvmod-softpurge

%build
./autogen.sh
# this assumes that VARNISHSRC is defined on the rpmbuild command line, like this:
# rpmbuild -bb --define 'VARNISHSRC /home/user/rpmbuild/BUILD/varnish-3.0.3' redhat/*spec
./configure VARNISHSRC=%{VARNISHSRC} VMODDIR=/usr/lib64/varnish/vmods/ --prefix=/usr/
make

%install
make install DESTDIR=%{buildroot}
mkdir -p %{buildroot}/usr/share/doc/%{name}/ 
cp README.rst %{buildroot}/usr/share/doc/%{name}/
cp LICENSE %{buildroot}/usr/share/doc/%{name}/

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
/usr/lib64/varnish/vmods/
%doc /usr/share/doc/%{name}/*
# workaround to get manpath correct.

%if "%{RHVERSION}" == "EL5"
%{_mandir}/man3/*.3*
%else
/usr/share/man/man?/*
%endif 

%changelog
* Thu Oct 11 2012 Lasse Karstensen <lasse@varnish-software.com> - 0.1-0
- Initial version.
