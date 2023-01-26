import os
import runpy
import sys
from pathlib import Path

import updater


if __name__ == '__main__':

    update = '--update-src' in sys.argv
    old_argv = [x for x in sys.argv if '--update-src' not in x]

    sys.argv = sys.argv[1:]

    root_dir = Path(sys.executable).parents[1] / 'openplc-uploader'
    bundle_dir = Path(getattr(sys, '_MEIPASS', Path.cwd()))
    config_file = root_dir / 'platformio.ini'

    if not config_file.is_file():
        updater.main(cwd=root_dir, path=bundle_dir)
    elif update:
        updater.main(cwd=root_dir, path=bundle_dir, update=True)

    sys.argv = [str(root_dir / 'bin' / 'gui.py')]
    sys.argv.append('--root-dir')
    sys.argv.append(str(root_dir))

    try:
        runpy.run_path(sys.argv[0], run_name='__main__')
    except SystemExit as e:
        if e.code == 42:

            sys.argv = old_argv
            sys.argv.append('--update-src')

            os.execv(sys.argv[0], sys.argv)

        else:
            sys.exit(e.code)
    else:
        sys.exit(0)
