#!/bin/sh
#Script for netscape plugin for gqmpeg to play icecast(.org) and
#shoutcast(.com) files. Also reads .m3u files now.

COMMAND_FILE=$HOME/.gqmpeg/command

IS_M3U=$(echo $1 | grep ".m3u")

if [ -n "$IS_M3U" ] ; then
	PLS_LINE=`cat $1 | grep "http"`
	PARSED=$PLS_LINE
else
	PLS_LINE=`cat $1 | grep "File1"`
	PARSED=$(echo $PLS_LINE | cut -d'=' -f2)
fi

exec gqmpeg -f $PARSED 2>&1 > /dev/null &

# you know, for .m3u this would also work:
# exec gqmpeg --playlist $1 2>&1 > /dev/null &
# or better, this (discards temp filename):
# exec gqmpeg --playlist_append $1 2>&1 > /dev/null &

