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

#Source Client
g++ -Wall \
	-o tcp_source_client tcp_source_client.c \
	tcpUtils.c paUtils.c sfUtils.cpp \
	-I$IDIR -I$IDIR/opus \
	-L$LDIR \
	-lsndfile -lportaudio -lopus
