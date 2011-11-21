#!/bin/sh

ndk-build  && \
ant clean debug && \
adb install -r bin/TouchpadNAActivity-debug.apk
