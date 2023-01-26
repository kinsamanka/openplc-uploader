# -*- mode: python ; coding: utf-8 -*-
from PyInstaller.utils.hooks import collect_all
from distutils.sysconfig import get_python_lib
import json
from pathlib import Path
import platform
import shutil
from subprocess import check_output


block_cipher = None

datas, binaries, hiddenimports = collect_all('platformio')

if platform.system() == 'Windows':
    root = Path(get_python_lib()).parents[1] / 'pkgs'
    cmds = ('ca-certificates', 'curl', 'libcurl', 'libssh2', 'openssl',
            'zlib', '7zip')
    post = 'Library'
else:
    root = Path(get_python_lib()).parents[2] / 'pkgs'
    cmds = ('ca-certificates', 'curl', 'libcurl', 'tar')
    post = ''

for c in cmds:
    out = check_output(['conda', 'list', '--json', '-f', c]).decode()
    dir = str(root / json.loads(out)[0]['dist_name'] / post)
    datas += [(dir, '.')]

a = Analysis(
    ['bin/start_pio.py'],
    binaries=binaries,
    datas=datas,
    hiddenimports=hiddenimports,
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

b = Analysis(
    ['bin/start_gui.py'],
    pathex=['bin'],
    hiddenimports=['gui'],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

pio_pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)
gui_pyz = PYZ(b.pure, b.zipped_data, cipher=block_cipher)

pio_exe = EXE(
    pio_pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name='pio',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
)

gui_exe = EXE(
    gui_pyz,
    b.scripts,
    [],
    exclude_binaries=True,
    name='uploader',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
)

coll = COLLECT(
    pio_exe,
    gui_exe,
    a.binaries,
    b.binaries,
    a.zipfiles,
    b.zipfiles,
    a.datas,
    b.datas,
    strip=False,
    upx=True,
    name='bin',
)
