@echo off

:: install on the directory where this batch file resides
set PWD=%~dp0
pushd "%PWD%"

set BUILDER_DIR=%PWD%builder
set CONDA_DIR=%PWD%conda
set CONDA_FNAME=Miniconda3-latest-Windows-x86_64.exe
set CONDA_URL=https://repo.anaconda.com/miniconda/%CONDA_FNAME%

goto :main

::
:: start of functions
::

:clone_repo
echo ## cloning git repo
setlocal
call "%CONDA_DIR%\condabin\conda" activate base 
git clone --recursive --depth=1 https://github.com/kinsamanka/openplc-uploader.git "%BUILDER_DIR%"
endlocal
exit /b 0

:update_repo
echo ## updating git repo
setlocal
call "%CONDA_DIR%\condabin\conda" activate base 
pushd "%BUILDER_DIR%"
git reset --hard
git pull
git submodule update --recursive
popd
endlocal
exit /b 0

:download_installer
echo ## Downloading Miniconda installer ...
bitsadmin /transfer getminiconda /download /priority normal %CONDA_URL% "%PWD%%CONDA_FNAME%"
exit /b 0

:install_conda
setlocal
echo ## Installing miniconda environment to %CONDA_DIR%, please wait...
%CONDA_FNAME% /InstallationType=JustMe /AddToPath=0 /RegisterPython=0 /S /D=%CONDA_DIR%
del %CONDA_FNAME%
echo ## basic miniconda installed
call "%CONDA_DIR%\condabin\conda" update -n base -yc defaults --all
call "%CONDA_DIR%\condabin\conda" activate base 
echo ## installing required packages
call conda install -y -c conda-forge platformio=6.1.6 wxpython=4.1.1 ^
    pyinstaller=5.6.2 git pypubsub curl 7zip
conda deactivate
endlocal
exit /b 0

:package
setlocal
call "%CONDA_DIR%\condabin\conda" activate base 
pyinstaller "%BUILDER_DIR%\assets\uploader.spec" -y
copy "%BUILDER_DIR%\assets\uploader.lnk" dist
pushd dist
7z a ..\uploader_win-64.zip bin uploader.lnk
popd
endlocal
exit /b 0

:main

if exist "%CONDA_DIR%" (
  if exist "%BUILDER_DIR%" (
    call :update_repo
    call :package
  ) else (
    :: continue aborted install
    call :clone_repo
    call :package
  )
  exit /b 0
)

if not exist "%CONDA_FNAME%" call :download_installer

call :install_conda
call :clone_repo
call :package
