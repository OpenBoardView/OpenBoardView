@echo off
echo Installing OpenFlex Board View


echo Configuration setup
set OD=%APPDATA%\openboardview
IF exist "%OD%" (
		echo Config install path already exists
) else (
		echo Creating config installation path %OD%...
		mkdir "%OD%"
)
set ODC=%OD%\obv.conf
IF exist "%ODC%" (
	echo Configuration file already exists, not overwriting
) else (
	echo Copying configuration file...
	copy sample-obv.conf "%ODC%"
)



set PD=%HOMEDRIVE%%HOMEPATH%\openboardview
IF exist "%PD%" (
		echo Binary install path already exists
) else (
		echo Creating binary installation path %PD%...
		mkdir "%PD%"
)
echo Copying binary...
copy openboardview.exe "%PD%"


set LNK=%HOMEDRIVE%%HOMEPATH%\Desktop\openboardview.lnk

echo Set oWS = WScript.CreateObject("WScript.Shell") > CreateShortcut.vbs
echo sLinkFile = "%LNK%" >> CreateShortcut.vbs
echo Set oLink = oWS.CreateShortcut(sLinkFile) >> CreateShortcut.vbs
echo oLink.TargetPath = "%PD%\openboardview.exe" >> CreateShortcut.vbs
echo oLink.Save >> CreateShortcut.vbs

if exist "%LNK%" (
	echo Desktop link already exists
) else (
	echo Creating desktop link...
	cscript CreateShortcut.vbs
)

echo .
echo .
echo Install finished.
echo You can now run the binary at %PD%\openboardview.exe
echo Check for a link on the desktop too.
echo .
echo .
@echo on

