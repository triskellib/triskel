#!/bin/bash
set -e -u -x

# Clear the previous wheels
rm dist/* || true

# Creates a build directory
mkdir /build

yum install -y cairo cairo-devel

export PLAT="manylinux_2_34_x86_64"

function repair_wheel {
    export LD_LIBRARY_PATH=/lib:$LD_LIBRARY_PATH
    export LD_LIBRARY_PATH=/lib64:$LD_LIBRARY_PATH

    wheel="$1"
    if ! auditwheel show "$wheel"; then
        echo "Skipping non-platform wheel $wheel"
    else
        auditwheel repair "$wheel" --plat "$PLAT" -w dist
    fi
}

# Compile wheels
for PYBIN in /opt/python/*/bin; do
    export SKBUILD_DIR="/build/${PYBIN}"
    mkdir --parents "${SKBUILD_DIR}"
    Python_ROOT_DIR=${PYBIN} "${PYBIN}/pip" wheel -w /build/wheelhouse/ . || true
done

# Bundle external shared libraries into the wheels
for whl in /build/wheelhouse/*.whl; do
    repair_wheel "$whl"
done