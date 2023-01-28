import os
import runpy
import sys
from pathlib import Path


if __name__ == '__main__':

    if len(sys.argv) > 2:

        sys.argv = sys.argv[1:]

        if 'pio' in sys.argv[0]:
            from platformio.__main__ import main
            sys.exit(main())
        else:
            try:
                runpy.run_path(sys.argv[0], run_name='__main__')
            except SystemExit as e:
                sys.exit(e.code)
            else:
                sys.exit(0)
    else:
        sys.exit(1)

