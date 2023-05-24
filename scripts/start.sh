#!/bin/bash -e

BUILD_DIR="$PWD/build"
CONDA_DIR="$PWD/conda"
PATH="$CONDA_DIR/bin:$PATH"
UNAME=`uname`
ARCH=`uname -m`
PYTHON="python"

if [ "$UNAME" = "Darwin" ]; then
    UNAME="MacOSX"
    PYTHON="pythonw"
fi

CONDA_URL=http://repo.continuum.io/miniconda/Miniconda3-latest-$UNAME-$ARCH.sh

if [ ! -d "$CONDA_DIR" ]; then
    mkdir -p "$CONDA_DIR"
    cd "$CONDA_DIR"
    curl -L $CONDA_URL -o miniconda.sh
    bash miniconda.sh -f -b -p "$CONDA_DIR"
    conda update --all -y
    conda config --set auto_update_conda False
    rm miniconda.sh

    conda install -y -c conda-forge platformio=6.1.6 wxpython=4.1.1 \
                                    pyinstaller=5.6.2 git pypubsub curl tar

    cd -
fi

if [ ! -d "$BUILD_DIR" ]; then
    git clone --recursive --depth=1 \
            https://github.com/kinsamanka/openplc-uploader.git "$BUILD_DIR"
fi

$PYTHON "$BUILD_DIR/bin/gui.py"
