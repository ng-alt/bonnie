Summary: A program for benchmarking hard drives and filesystems
Name: bonnie++
Version: 1.00d
Release: 1
Copyright: GPL
Group: Utilities/Benchmarking
Source: http://www.coker.com.au/bonnie++/bonnie++%{version}.tgz 
BuildRoot: /tmp/%{name}-buildroot

%description
Bonnie++ is a benchmark suite that is aimed at performing a number of simple
tests of hard drive and file system performance.

%prep
%setup -q

%build
./configure --prefix=${RPM_BUILD_ROOT}
make 

%install
rm -rf $RPM_BUILD_ROOT
DESTDIR=${RPM_BUILD_ROOT} make install
install -d ${RPM_BUILD_ROOT}/usr/man/man8
install -d ${RPM_BUILD_ROOT}/usr/man/man1
install -m 644 *.8 $RPM_BUILD_ROOT/usr/man/man8
install -m 644 *.1 $RPM_BUILD_ROOT/usr/man/man1

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc changelog.txt readme.html

/usr/sbin/bonnie++
/usr/sbin/zcav
/usr/bin/bon_csv2html
/usr/bin/bon_csv2txt
/usr/man/man1/bon_csv2html.1
/usr/man/man1/bon_csv2txt.1
/usr/man/man8/bonnie++.8
/usr/man/man8/zcav.8

%changelog
* Wed Sep 06 2000 Rob Latham <rlatham@plogic.com>
- first packaging
