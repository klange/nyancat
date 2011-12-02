#!/bin/bash

screen -X -p 0 stuff "= RESETTING ="
screen -X -p 0 stuff $'\012'

killall cgiserver 2>/dev/null
