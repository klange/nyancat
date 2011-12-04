# Nyancat CLI

Nyancat rendered in your terminal.

[![Nyancats](http://i.imgur.com/snCOQl.png)](http://i.imgur.com/snCOQ.png)

## Setup

First build the C application:

    make && cd src

You can run the C application standalone. It will prompt you to select a color mode.

    ./nyancat

To use the telnet server, you need to add a configuration that runs:

    nyancat -t

... to either an `inetd` or `xinetd` server. I am using `openbsd-inetd`, which you must give *both* arguments (`nyancat` and `-t`) to, which I found odd.

## Licenses, References, etc.

The original source of the Nyancat animation is [prguitarman](http://www.prguitarman.com/index.php?id=348).

The code provided here is provided under the terms of the [NCSA license](http://en.wikipedia.org/wiki/University_of_Illinois/NCSA_Open_Source_License).
