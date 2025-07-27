#!/bin/sh

if [[ $(arch) == 'i386' ]]; then
  	echo Intel Mac
	IDIR="/usr/local/include"
	LDIR="/usr/local/lib"
elif [[ $(arch) == 'arm64' ]]; then
  	echo M1 Mac
	IDIR="/opt/homebrew/include"
	LDIR="/opt/homebrew/lib"
else
	echo Win PC
	IDIR="/usr/include"
	LDIR="/usr/lib"
fi

#Client
g++ -Wall \
	-o tcp_client tcp_client.c circular_buf.cpp tcpUtils.c paUtils.c \
	-I$IDIR -I$IDIR/opus \
	-L$LDIR \
	-lportaudio -lopus
