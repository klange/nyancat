# Nyancat CLI

Nyancat rendered in your terminal.

[![Nyancats](http://i.imgur.com/snCOQl.png)](http://i.imgur.com/snCOQ.png)

## Distributions

Nyancat is available in the following distributions:

- [Arch](http://aur.archlinux.org/packages.php?ID=55279)
- [Debian](http://packages.qa.debian.org/n/nyancat.html)
- [Gentoo](http://packages.gentoo.org/package/games-misc/nyancat)
- [Mandriva](http://sophie.zarb.org/rpms/928724d4aea0efdbdeda1c80cb59a7d3)
- [Ubuntu](https://launchpad.net/ubuntu/+source/nyancat)

## Setup

First build the C application:

    make && cd src

You can run the C application standalone.

    ./nyancat

To use the telnet server, you need to add a configuration that runs:

    nyancat -t

We recommend `openbsd-inetd`, but `xinetd` will work as well, and you should be able to use any other compatible `inetd` flavor.

## Distribution Specific Information

#### Debian/Ubuntu

Debian and Ubuntu provide the nyancat binary through the `nyancat` package.
A `nyancat-server` package is provided to automatically setup and enable a
nyancat telnet server upon installation. I am not the maintainer of these
package, please direct any questions or bugs to the relevant distribution's bug
tracking system.

## Licenses, References, etc.

The original source of the Nyancat animation is [prguitarman](http://www.prguitarman.com/index.php?id=348).

The code provided here is provided under the terms of the [NCSA license](http://en.wikipedia.org/wiki/University_of_Illinois/NCSA_Open_Source_License).
