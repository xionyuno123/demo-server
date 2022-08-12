#/bin/sh

# Maksure that you install microhttpd and jasson libraries first.
# The two libraries can be found on the following two websites.
# 1. microhttpd: https://www.gnu.org/software/libmicrohttpd
# 2. jansson: https://github.com/akheron/jansson

DPDK_VERSION=20.11.4
ROOT_DIR=$PWD

if [ ! -f "$ROOT_DIR/deps/dpdk-$DPDK_VERSION.tar.gz" ]; then
    echo "Downloading DPDK $DPDK_VERSION"
    wget http://fast.dpdk.org/rel/dpdk-$DPDK_VERSION.tar.gz -P $ROOT_DIR/deps
fi

if [ ! -d "$ROOT_DIR/deps/dpdk-source" ]; then
    mkdir $ROOT_DIR/deps/dpdk-source
    tar -xf $ROOT_DIR/deps/dpdk-$DPDK_VERSION.tar.gz --strip-components=1 -C $ROOT_DIR/deps/dpdk-source
fi

if [ ! -d "$ROOT_DIR/deps/dpdk-source/build" ]; then
    echo "Build and install DPDK"
    
    if [ -d "$ROOT_DIR/deps/dpdk-install" ]; then
        rm -rf $ROOT_DIR/deps/dpdk-install
    fi

    cd $ROOT_DIR/deps/dpdk-source
    meson -Dprefix=$ROOT_DIR/deps/dpdk-install build
    cd $ROOT_DIR/deps/dpdk-source/build 
    ninja
    ninja install
    echo "Installing dpdk library to $ROOT_DIR/deps/dpdk-install"
fi

if [ ! -d "$ROOT_DIR/build" ]; then
    mkdir $ROOT_DIR/build
    cd $ROOT_DIR/build
    PKG_CONFIG_PATH=$ROOT_DIR/deps/dpdk-install/lib/x86_64-linux-gnu/pkgconfig cmake ..
fi

echo "Build configuration completes. Please switch to build directory to build the test-pmd program."