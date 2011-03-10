#!/bin/bash

ffmpeg -f x11grab -r 25 -s xga -i :0.0 -vcodec libx264 -vpre ultrafast -b 1228800 -r 25 -f mpegts -y live.m2t
