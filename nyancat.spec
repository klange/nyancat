Name:		nyancat
Version:	1.5.1
Release:	1%{?dist}
Summary:	Nyancat in your terminal, rendered through ANSI escape sequences.

Group:		Misc
License:	NCSA
URL:		https://github.com/froller/nyancat
Source:		%{name}-%{version}.tar.gz
Patch0:		%{name}-0.patch

#BuildRequires:	
Requires:	systemd

%description

nyancat is an animated, color, ANSI-text program that renders a loop of the
classic Nyan Cat animation. 

nyancat makes use of various ANSI escape sequences to render color, or in the case
of a VT220, simply dumps text to the screen.

%prep
%setup -q
%patch -P 0 -p1

%build
make %{?_smp_mflags}

%install
install -m 0755 -D -t $RPM_BUILD_ROOT/usr/bin $RPM_BUILD_DIR/%{name}-%{version}/src/%{name}
install -m 0644 -D -t $RPM_BUILD_ROOT/lib/systemd/system $RPM_BUILD_DIR/%{name}-%{version}/systemd/%{name}.socket
install -m 0644 -D -t $RPM_BUILD_ROOT/lib/systemd/system $RPM_BUILD_DIR/%{name}-%{version}/systemd/%{name}@.service
install -m 0644 -D -t $RPM_BUILD_ROOT/usr/share/man/man1 $RPM_BUILD_DIR/%{name}-%{version}/%{name}.1

%post
systemctl daemon-reload

%preun
systemctl stop %{name}.socket
systemctl disable %{name}.socket

%postun
systemctl daemon-reload

%files
/usr/bin/%{name}
/lib/systemd/system/%{name}.socket
/lib/systemd/system/%{name}@.service
/usr/share/man/man1/%{name}.1.gz
%doc

%changelog
