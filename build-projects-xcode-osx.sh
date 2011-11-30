#!/bin/sh

export TRAKTOR_HOME=$PWD

CONFIG=$1
if [ -z $CONFIG ] ; then
	CONFIG="both"
fi

if [ $CONFIG == "static" ] ; then
	$TRAKTOR_HOME/bin/osx/SolutionBuilder -f=xcode -xcode-root-suffix=-static TraktorMacOSX.xms -d=DebugStatic -r=ReleaseStatic
elif [ $CONFIG == "shared" ] ; then
	$TRAKTOR_HOME/bin/osx/SolutionBuilder -f=xcode -xcode-root-suffix=-shared TraktorMacOSX.xms -d=DebugShared -r=ReleaseShared
else
	$TRAKTOR_HOME/bin/osx/SolutionBuilder -f=xcode -xcode-root-suffix=-static TraktorMacOSX.xms -d=DebugStatic -r=ReleaseStatic
	$TRAKTOR_HOME/bin/osx/SolutionBuilder -f=xcode -xcode-root-suffix=-shared TraktorMacOSX.xms -d=DebugShared -r=ReleaseShared
fi

