set target_dir=%1%
echo target directory is %target_dir%
md %target_dir%\sdk_client
md %target_dir%\app
xcopy /y /s ".\sdk_client" %target_dir%\sdk_client
xcopy /y /s ".\app" %target_dir%\app
xcopy /y /s ".\.cproject" %target_dir%
xcopy /y /s ".\project.ioc" %target_dir%
for %%f in (%target_dir%) do set projectname=%%~nxf
echo %projectname%
del "%target_dir%\%projectname%.ioc"
rename "%target_dir%\project.ioc" %projectname%.ioc

