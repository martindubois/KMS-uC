@echo off

rem Author    KMS - Martin Dubois, P. Eng.
rem Copyright (C) 2026 KMS
rem License   http://www.apache.org/licenses/LICENSE-2.0
rem Product   KMS-uC
rem File      Tests/MC56F/MC56F84565/CreateSAP.cmd

echo Executing  Tests/MC56F/MC56F84565/CreateSAP.cmd  ...

rem ===== Configuration =====================================================

set CYCLONE_FOLDER=C:\PEMicro\cyclone

rem ===== Initialisation ====================================================

set CFG_FILE="Cyclone.cfg"

set SAP_FILE="C:\_VC\KMS-uC\Tests\MC56F\MC56F84565\Test.sap"

set CSAP_EXE="%CYCLONE_FOLDER%\imageCreation\ImageCreationSupportFiles\csapdscz.exe"

rem ===== Verification ======================================================

if not exist %CYCLONE_FOLDER% (
    echo FATAL ERROR  %CYCLONE_FOLDER%  does not exist
    echo Install Cyclone software
    pause
    exit 10
)

if not exist %CSAP_EXE% (
    echo FATAL ERROR  %CSAP_EXE%  does not exist
    echo Repair Cyclone software
    pause
    exit 20
)

rem ===== Execution =========================================================

%CSAP_EXE% ? %CFG_FILE% /imagefile %SAP_FILE%
if ERRORLEVEL 1 (
    echo ERROR  %CSAP_EXE% ? %CFG_FILE% /imagefile %SAP_FILE%  failed - %ERRORLEVEL%
    pause
    exit /B 30
)

rem ===== End ===============================================================

echo OK
