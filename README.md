# Nyancat CLI

Nyancat rendered in your terminal.

[![Nyancats](http://i.imgur.com/snCOQl.png)](http://i.imgur.com/snCOQ.png)

## Setup

First build the C application:

    make && cd src

You can run the C application standalone. It will prompt you to select a color mode.

    ./nyancat

To run the telnet server, use:

    sudo ./start.sh

(The server will only try to start on port 23, so you must be root)

## Licenses, References, etc.

The original source of the Nyancat animation is [prguitarman](http://www.prguitarman.com/index.php?id=348).

The code provided here is provided under the terms of the [NCSA license](http://en.wikipedia.org/wiki/University_of_Illinois/NCSA_Open_Source_License).
