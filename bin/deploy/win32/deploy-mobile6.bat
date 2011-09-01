@echo off

if "%1"=="build" (

	%DEPLOY_PROJECTROOT%\bin\latest\win32\releaseshared\Traktor.Pipeline.App -p -s=Pipeline -l=Pipeline.log %2

) else if "%1"=="deploy" (

	%TRAKTOR_HOME%bin\CeDeploy "\Program Files\%DEPLOY_PROJECTNAME%" %DEPLOY_PROJECTROOT:/=\%\bin\latest\mobile6\releaseshared\*.dll
	%TRAKTOR_HOME%bin\CeDeploy "\Program Files\%DEPLOY_PROJECTNAME%" %DEPLOY_PROJECTROOT:/=\%\bin\latest\mobile6\releaseshared\*.exe
	%TRAKTOR_HOME%bin\CeDeploy "\Program Files\%DEPLOY_PROJECTNAME%" Application.config

) else if "%1"=="launch" (

	%TRAKTOR_HOME%bin\CeLaunch "\Program Files\%DEPLOY_PROJECTNAME%\%DEPLOY_EXECUTABLE%" Application.config
	
) else if "%1"=="migrate" (

	%DEPLOY_PROJECTROOT%\bin\latest\win32\releaseshared\Traktor.Database.Migrate.App %DEPLOY_SOURCE_CS% %DEPLOY_TARGET_CS% %DEPLOY_MODULES% > %DEPLOY_PROJECTROOT%\migrate.log
	
)

