from os import makedirs, environ
from pathlib import PureWindowsPath
from shutil import rmtree
from subprocess import check_output
import platform

win = platform.system() == "Windows"


def skip_from_build(node):
    n = node.get_path()
    if win:
        n = PureWindowsPath(n).as_posix()

    # filter UIP src and matiec dir
    skip = ["lib/matiec"]
    if any(s in n for s in skip):
        return None

    # otherwise allow all
    return node


def fix_pous_c(node):
    if "POUS" not in node.name:
        return node

    return env.Object(
        node,
        CCFLAGS=env["CCFLAGS"] + ["-include", "generated/POUS.h"]
    )


Import("env")

bc = env.BoardConfig()

rom_size = f"{bc.get('upload.maximum_size') // 1024}K"
ram_size = f"{bc.get('upload.maximum_ram_size') // 1024}K"

env.Append(CCFLAGS=[f"-DMAX_FLASH_SIZE={bc.get('upload.maximum_size')}",
                    f"-DRAM_SIZE={bc.get('upload.maximum_ram_size')}",
                    f"-DRAM_{ram_size}"])

# mark these libs as system to ignore GCC warnings
env.Append(CCFLAGS=["-isystem", "lib/matiec/lib/C"])

env.Append(CFLAGS=["-Wimplicit-function-declaration",
                   "-Wmissing-prototypes",
                   "-Wstrict-prototypes"])

# Register callback
env.AddBuildMiddleware(skip_from_build, "*")

env.AddBuildMiddleware(fix_pous_c)

src = environ.get('OPENPLC_SRC')
if not src:
    src = "plc_prog.st"

print(f"Compiling {src} ...")

gdir = f"{env['PROJECT_SRC_DIR']}/generated"
rmtree(gdir, ignore_errors=True)
makedirs(gdir, exist_ok=True)

path = f"{env['PROJECT_DIR']}/lib/matiec"
if win:
    path = PureWindowsPath(path).as_posix()

c = f'"{path}/bin/iec2c.exe"' if win else f'{path}/bin/iec2c'

cmd = [c, '-l', '-I', 'lib/matiec/lib', '-T', 'src/generated', f'"{src}"']

print(f"    {' '.join(cmd)}")

check_output(' '.join(cmd), shell=True)
