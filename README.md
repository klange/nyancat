# Nyancat CLI

Nyancat rendered in your terminal.

[![Nyancats](http://i.imgur.com/snCOQl.png)](http://i.imgur.com/snCOQ.png)

## Setup

First build the C application:

    make

You can run the C application standalone. It will prompt you to select a color mode.

    ./nyancat

To run the telnet server, use:

    sudo ./nyancat.py

(The server will only try to start on port 23, so you must be root)
