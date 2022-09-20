from os import makedirs
from pathlib import PureWindowsPath
from shutil import rmtree
from subprocess import check_output
import platform

def skip_from_build(node):
    n = node.get_path()
    if platform.system() == "Windows":
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

env.Append(CCFLAGS=[f"-DFLASH_SIZE={bc.get('upload.maximum_size')}",
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

print("Compiling plc_prog.st ...")

gdir = f"{env['PROJECT_SRC_DIR']}/generated"
rmtree(gdir, ignore_errors=True)
makedirs(gdir, exist_ok=True)

path = env.PioPlatform().get_package_dir("tool-matiec")
cmd = "iec2c.exe" if platform.system() == "Windows" else "iec2c"

check_output(f"{path}/bin/{cmd} -l -I lib/matiec/lib -T src/generated plc_prog.st", shell=True)
