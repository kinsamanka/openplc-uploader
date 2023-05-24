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


Import("env")

bc = env.BoardConfig()

env.Append(CCFLAGS=[f"-DMAX_FLASH_SIZE={bc.get('upload.maximum_size')}",
                    f"-DRAM_SIZE={bc.get('upload.maximum_ram_size')}",
                    f"-DRAM_{bc.get('upload.maximum_ram_size') // 1024}K"])

if 'stm32' in env['PIOPLATFORM'] and "boot" not in env["PIOENV"]:
    if (env['UPLOAD_PROTOCOL'] == 'serial'):
        env.Append(CCFLAGS=[f"-DSTM32_BAUD_RATE={env['UPLOAD_SPEED']}"])

# mark these libs as system to ignore GCC warnings
env.Append(CCFLAGS=["-isystem", "lib/matiec/lib/C"])

env.Append(CFLAGS=["-Wno-unused-but-set-variable", "-Wno-unused-variable",
                   "-Werror=implicit-function-declaration"])

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
