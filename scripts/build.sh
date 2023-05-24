#!/bin/bash -e

BUILD_DIR="$PWD/build"
CONDA_DIR="$PWD/conda"
PATH="$CONDA_DIR/bin:$PATH"
UNAME=`uname`

CONDA_URL=http://repo.continuum.io/miniconda/Miniconda3-latest-$UNAME-x86_64.sh

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
else
    cd "$BUILD_DIR"
    git reset --hard
    git pull
    git submodule update --recursive
    cd -
fi

pyinstaller "$BUILD_DIR/assets/uploader.spec" -y

(cd dist; ln -sf bin/uploader uploader)

tar czf uploader_${UNAME,,}-64.tar.gz --transform 's/^dist/uploader/' dist

