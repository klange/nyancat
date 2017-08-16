Name:		nyancat
Version:	1.5.1
Release:	2%{?dist}
Summary:	Nyancat in your terminal, rendered through ANSI escape sequences.

Group:		Misc
License:	NCSA
URL:		https://github.com/froller/nyancat
Source:		%{name}-%{version}.tar.gz
Patch0:		%{name}-0.patch
Patch1:		%{name}-1.patch

#BuildRequires:	
Requires:	systemd
Requires(post):	systemd
Requires(preun):	systemd
Requires(postun):	systemd

%description

nyancat is an animated, color, ANSI-text program that renders a loop of the
classic Nyan Cat animation. 

nyancat makes use of various ANSI escape sequences to render color, or in the case
of a VT220, simply dumps text to the screen.

%prep
%setup -q
%patch0 -p1 -b 0
%patch1 -p1 -b 1

%build
make %{?_smp_mflags}

%install
install -m 0755 -D -t %{buildroot}%{_bindir} %{_builddir}/%{name}-%{version}/src/%{name}
install -m 0644 -D -t %{buildroot}%{_sysconfdir}/sysconfig %{_builddir}/%{name}-%{version}/sysconfig/%{name}
install -m 0644 -D -t %{buildroot}%{_unitdir} %{_builddir}/%{name}-%{version}/systemd/%{name}.socket
install -m 0644 -D -t %{buildroot}%{_unitdir} %{_builddir}/%{name}-%{version}/systemd/%{name}@.service
install -m 0644 -D -t %{buildroot}%{_mandir}/man1 %{_builddir}/%{name}-%{version}/%{name}.1

%post
%systemd_post %{name}.socket

%preun
%systemd_preun %{name}.socket

%postun
%systemd_postun_with_restart %{name}.socket

%files
%{_bindir}/%{name}
%config(noreplace) %{_sysconfdir}/sysconfig/%{name}
%{_unitdir}/%{name}.socket
%{_unitdir}/%{name}@.service
#%{_datadir}/polkit-1/actions/org.fedoraproject.FirewallD1.desktop.policy
#%{_datadir}/polkit-1/actions/org.fedoraproject.FirewallD1.server.policy
#%if 0%{?fedora} > 21
#%ghost %{_datadir}/polkit-1/actions/org.fedoraproject.FirewallD1.policy
#%endif
%{_mandir}/man1/%{name}*
%doc

%changelog
