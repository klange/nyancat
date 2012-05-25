# Nyancat CLI

Nyancat rendered in your terminal.

[![Nyancats](http://i.imgur.com/snCOQl.png)](http://i.imgur.com/snCOQ.png)

## Setup

First build the C application:

    make && cd src

You can run the C application standalone.

    ./nyancat

To use the telnet server, you need to add a configuration that runs:

    nyancat -t

We recommend `openbsd-inetd`, but `xinetd` will work as well, and you should be able to use any other compatible `inetd` flavor.

## Debian

If you are running Debian Sid ("unstable" as of writing this), you can install the `nyancat` package and the `nyancat-server` pseudo-package, the latter of which will install `openbsd-inetd` (unless you have another `inetd` installed) and set up `/etc/initd.conf` properly (note that it probably won't work if you have `xinetd`, so you're on your own in that case). I am not the maintainer of this package, please direct any questions or bugs to [jmccrohan](https://github.com/jmccrohan).

## Licenses, References, etc.

The original source of the Nyancat animation is [prguitarman](http://www.prguitarman.com/index.php?id=348).

The code provided here is provided under the terms of the [NCSA license](http://en.wikipedia.org/wiki/University_of_Illinois/NCSA_Open_Source_License).
