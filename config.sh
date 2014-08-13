#!/bin/bash

# Get normalized path to this script, excluding file name.
TRAKTOR_HOME="`dirname \"$BASH_SOURCE\"`"
TRAKTOR_HOME="`(cd \"$TRAKTOR_HOME\" && pwd)`"

export TRAKTOR_HOME
export STEAMWORKS_SDK=$TRAKTOR_HOME/3rdp/steamworks-sdk-128
export BULLET_SDK=$TRAKTOR_HOME/3rdp/bullet3-master
export FBX_SDK=$TRAKTOR_HOME/3rdp/FbxSdk/2014.2
