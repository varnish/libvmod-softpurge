Summary: varnish-vmod-softpurge
Name: varnish-vmod-softpurge
Version: 0.1
Release: 1%{?dist}
License: BSD
Group: System Environment/Daemons
Source0: ./libvmod-softpurge.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
# we need EPEL, or at least mhash + mhash-devel from it:
# rpm -Uvh http://download.fedoraproject.org/pub/epel/6/i386/epel-release-6-7.noarch.rpm
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
# /opt/varnish/lib/varnish/vmods/
/usr/lib64/varnish/vmods/
%doc /usr/share/doc/%{name}/*
#%{_mandir}/man3/*.3*
/usr/man/man?/*gz

%changelog
* Thu Oct 11 2012 Lasse Karstensen <lasse@varnish-software.com> - 0.1-0
- Initial version.
