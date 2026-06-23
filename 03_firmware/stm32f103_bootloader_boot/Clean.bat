@echo off
chcp 65001 
cls

@echo.
@echo Keil project cleanup script
@echo Please place this file in the root directory of the project to be cleaned
@echo www.100ask.net
@echo.        
@echo off

echo Clean up? Y. Confirm
echo            N. Exit
Echo.
Set /p var=Please select:
If /i %var%==N (Exit) 

@echo.
echo Cleaning...

:: Project/
::del>nul 2>nul *.uvoptx   /s /q 
del>nul 2>nul *.uvguix.* /s /q  
del>nul 2>nul *.scvd     /s /q 

:: Project/DebugConfig
del>nul 2>nul *.dbgconf /s /q  

:: Project/Listings
del>nul 2>nul *.map /s /q  
del>nul 2>nul *.lst /s /q  

:: Project/Objects
del>nul 2>nul *.axf /s /q 
del>nul 2>nul *.htm /s /q 
del>nul 2>nul *.crf /s /q 
del>nul 2>nul *.dep /s /q 
del>nul 2>nul *.lnp /s /q 
del>nul 2>nul *.sct /s /q 
del>nul 2>nul *.hex /s /q 
del>nul 2>nul *.d   /s /q 
del>nul 2>nul *.o   /s /q 


@echo.
Echo Cleanup complete!
@echo. 
timeout /T 3 /NOBREAK

exit
