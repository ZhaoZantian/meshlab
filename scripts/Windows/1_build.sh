#!/bin/bash
# this is a script shell for compiling meshlab in a windows environment.
# Requires a VS >= 2017 and Qt environments which are set-up properly,
# and an accessible cmake binary.
#
# Without given arguments, MeshLab will be built in the meshlab/build
# directory, and installed in meshlab/install.
#
# You can give as argument the BUILD_PATH and the INSTALL_PATH in the
# following way:
# bash 1_build.sh --build_path=/path/to/build --install_path=/path/to/install
# -b and -i arguments are also supported.

#default paths wrt the script folder
SCRIPTS_PATH="$(dirname "$(realpath "$0")")"
SOURCE_PATH=$SCRIPTS_PATH/../../src
BUILD_PATH=$SOURCE_PATH/../build
INSTALL_PATH=$SOURCE_PATH/../install
DOUBLE_PRECISION_OPTION=""
NIGHTLY_OPTION=""

#check parameters
for i in "$@"
do
case $i in
    -b=*|--build_path=*)
        BUILD_PATH="${i#*=}"
        shift # past argument=value
        ;;
    -i=*|--install_path=*)
        INSTALL_PATH="${i#*=}"
        shift # past argument=value
        ;;
    -d|--double_precision)
        DOUBLE_PRECISION_OPTION="-DBUILD_WITH_DOUBLE_SCALAR=ON"
        shift # past argument=value
        ;;
    -n|--nightly)
        NIGHTLY_OPTION="-DMESHLAB_IS_NIGHTLY_VERSION=ON"
        shift # past argument=value
        ;;
    *)
        # unknown option
        ;;
esac
done

#create build path if necessary
if ! [ -d $BUILD_PATH ]
then
    mkdir -p $BUILD_PATH
fi

#create install path if necessary
if ! [ -d $INSTALL_PATH ]
then
    mkdir -p $INSTALL_PATH
fi

BUILD_PATH=$(realpath $BUILD_PATH)
INSTALL_PATH=$(realpath $INSTALL_PATH)

cd $BUILD_PATH
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH $DOUBLE_PRECISION_OPTION $NIGHTLY_OPTION $SOURCE_PATH
ninja
ninja install
