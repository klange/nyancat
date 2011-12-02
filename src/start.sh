#!/bin/bash
# Run me as root!

screen -d -m
sleep 1
screen -X -p 0 stuff "./run.sh"
screen -X -p 0 stuff $'\012'
