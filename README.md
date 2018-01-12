# Nyancat CLI

Nyancat rendered in your terminal.

[![Nyancats](http://nyancat.dakko.us/nyancat.png)](http://nyancat.dakko.us/nyancat.png)

## Distributions

Nyancat is available in the following distributions:

- [Arch](https://www.archlinux.org/packages/?q=nyancat)
- [Debian](http://packages.qa.debian.org/n/nyancat.html)
- [Gentoo](http://packages.gentoo.org/package/games-misc/nyancat)
- [Mandriva](http://sophie.zarb.org/rpms/928724d4aea0efdbdeda1c80cb59a7d3)
- [Ubuntu](https://launchpad.net/ubuntu/+source/nyancat)

And also on some BSD systems:

- [FreeBSD](http://www.freshports.org/net/nyancat/)
- [OpenBSD](http://openports.se/misc/nyancat)
- [NetBSD](http://pkgsrc.se/misc/nyancat)

## Setup

First build the C application:

    make && cd src

You can run the C application standalone.

    ./nyancat

To use the telnet server, you need to add a configuration that runs:

    nyancat -t

For `openbsd-inetd`, add this to config and reload (change `/usr/local/bin/nyancat` to your path):

    telnet	stream	tcp	nowait	root	/usr/local/bin/nyancat	nyancat -t

We recommend `openbsd-inetd`, but both `xinetd` and `systemd` work as well. You
should be able to use any other compatible `inetd` flavor too.

## Distribution Specific Information

#### Debian/Ubuntu

Debian and Ubuntu provide the nyancat binary through the `nyancat` package. A
`nyancat-server` package is provided to automatically setup and enable a nyancat
telnet server upon installation. I am not the maintainer of these packages;
please direct any questions or bugs to the relevant distribution's bug tracking
system.

## Licenses, References, etc.

The original source of the Nyancat animation is
[prguitarman](http://www.prguitarman.com/index.php?id=348).

The code provided here is provided under the terms of the
[NCSA license](http://en.wikipedia.org/wiki/University_of_Illinois/NCSA_Open_Source_License).
