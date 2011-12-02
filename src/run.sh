#!/bin/bash

while [ 1 == 1 ]; do
	killall nyancat
	killall python
	./nyancat.py 2>/dev/null
done
