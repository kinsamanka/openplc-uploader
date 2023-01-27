#!/bin/bash -e

BUILD_DIR="$PWD/build"
CONDA_DIR="$PWD/conda"
PATH="$CONDA_DIR/bin:$PATH"
UNAME=`uname`

CONDA_URL=http://repo.continuum.io/miniconda/Miniconda3-latest-$UNAME-x86_64.sh

if [ ! -d "$CONDA_DIR" ]; then
    mkdir -p "$CONDA_DIR"
    cd "$CONDA_DIR"
    wget $CONDA_URL -O miniconda.sh
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

python "$BUILD_DIR/bin/gui.py"
