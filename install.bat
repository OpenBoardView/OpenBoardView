
echo "Installing OpenFlex Board View"

IF exist openboardview.exe (
IF exist sample-obv.conf (



;; Create the configuration and history file paths
;;
set OD=%APPDATA%\openboardview
IF exist %OD% (
		echo "Config install path already exists"
) else (
		echo "Creating config installation path %OD%..."
		mkdir %OD%
)
IF exist %OD%\obv.conf (
	echo "Configuration file already exists, not overwriting"
) else (
	echo "Copying configuration file..."
	copy sample-obv.conf %OD%\obv.conf
)



;; Create the binary install path
;;
set PD=%PROGRAMFILES%\openboardview
IF exist %PD% (
		echo "Binary install path already exists"
) else (
		echo "Creating binary installation path %PD%..."
		mkdir %PD%
)
echo "Copying binary...
copy openboardview.exe %PD%



;; Attempt to create a desktop link
;;
set LNK="%HOMEDRIVE%%HOMEPATH%\Desktop\openboardview.lnk"
if exist %LNK% (
		echo "Desktop link already exists"
		) else (
			if exist %PD% (
@echo off
echo Set oWS = WScript.CreateObject("WScript.Shell") > CreateShortcut.vbs
echo sLinkFile = "%HOMEDRIVE%%HOMEPATH%\Desktop\openboardview.lnk" >> CreateShortcut.vbs
echo Set oLink = oWS.CreateShortcut(sLinkFile) >> CreateShortcut.vbs
echo oLink.TargetPath = "%PD%" >> CreateShortcut.vbs
echo oLink.Save >> CreateShortcut.vbs
@echo on
		echo "Creating desktop link..."
cscript CreateShortcut.vbs
del CreateShortcut.vbs
)
)


echo "Install finished."
echo "You can now run the binary at %OD%\openboardview.exe"
) else (
	echo "Cannot find sample-obv.conf to install."
	)
) else (
	echo "Cannot find openboardview.exe to install."
	)

echo

