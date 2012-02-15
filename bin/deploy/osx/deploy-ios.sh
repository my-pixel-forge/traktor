#!/bin/sh

if [[ ${1} == "build" ]]; then

	# Build content.
	$DEPLOY_PROJECTROOT/bin/latest/osx/releaseshared/Traktor.Pipeline.App -p -s=Pipeline -l=Pipeline.log ${2}

elif [[ ${1} == "deploy" ]]; then

	# Deploy iPhone simulator launcher.
	pushd $TRAKTOR_HOME/bin/osx
	$TRAKTOR_HOME/bin/osx/RemoteDeploy $DEPLOY_TARGET_HOST waxsim > $DEPLOY_PROJECTROOT/deploy.log
	popd

	# Deploy iPhone binaries.
	pushd $DEPLOY_PROJECTROOT/bin/latest/iphone-simulator/releasestatic
	$TRAKTOR_HOME/bin/osx/RemoteDeploy -recursive $DEPLOY_TARGET_HOST * >> $DEPLOY_PROJECTROOT/deploy.log
	popd

elif [[ ${1} == "launch" ]]; then

	# Launch application with simulator launcher on target.
	$TRAKTOR_HOME/bin/osx/RemoteDeploy -target-base="$DEPLOY_EXECUTABLE.app" $DEPLOY_TARGET_HOST Application.config > $DEPLOY_PROJECTROOT/deploy.log
	$TRAKTOR_HOME/bin/osx/RemoteLaunch $DEPLOY_TARGET_HOST waxsim "$DEPLOY_EXECUTABLE.app" >> $DEPLOY_PROJECTROOT/deploy.log

fi
