#!/usr/bin/env bash
# Builds a static cairo (and its dependencies) with vcpkg, for linking into
# the pytriskel wheels. Usage: vcpkg-cairo.sh <triplet>
set -euo pipefail

triplet="$1"

if [ -d /opt/python ]; then
    # manylinux container: install the tools vcpkg and meson need, and put a
    # modern python on PATH (meson needs >= 3.7, the el8 system python is 3.6)
    # autotools are for vcpkg's gperf port (a fontconfig build dependency);
    # fontconfig/dejavu are not build dependencies: they give the container
    # the font configuration a normal Linux system has, so the wheel smoke
    # test exercises text measuring like an end-user machine would
    dnf install -y -q epel-release || true
    dnf install -y -q zip unzip tar perl pkgconf-pkg-config \
        autoconf autoconf-archive automake libtool \
        fontconfig dejavu-sans-fonts
    export PATH="/opt/python/cp312-cp312/bin:${PATH}"
    export VCPKG_DEFAULT_BINARY_CACHE="${VCPKG_DEFAULT_BINARY_CACHE:-/project/vcpkg-archives}"
    if [ "$(uname -m)" != x86_64 ]; then
        # There are no prebuilt vcpkg/cmake/ninja binaries for linux-arm64
        export VCPKG_FORCE_SYSTEM_BINARIES=1
        pip install -q cmake ninja
    fi
elif [ "$(uname)" = Darwin ]; then
    # autotools are for vcpkg's gperf port (a fontconfig build dependency)
    brew install autoconf autoconf-archive automake libtool
    if ! command -v pkg-config >/dev/null; then
        # CMake reads the static link information through pkg-config
        brew install pkgconf
    fi
fi

export VCPKG_DEFAULT_BINARY_CACHE="${VCPKG_DEFAULT_BINARY_CACHE:-${PWD}/vcpkg-archives}"
mkdir -p "${VCPKG_DEFAULT_BINARY_CACHE}"

root="${VCPKG_INSTALLATION_ROOT:-${HOME}/vcpkg}"
if [ ! -e "${root}/vcpkg" ] && [ ! -e "${root}/vcpkg.exe" ]; then
    git clone --depth 1 https://github.com/microsoft/vcpkg "${root}"
    "${root}/bootstrap-vcpkg.sh" -disableMetrics
fi

ports=("cairo:${triplet}")
if [[ "${triplet}" == *windows* ]]; then
    # pkgconf gives CMake a pkg-config executable on Windows
    ports+=("pkgconf:x64-windows")
fi
"${root}/vcpkg" install "${ports[@]}"

# Export the install locations for later workflow steps (host builds only)
if [ -n "${GITHUB_ENV:-}" ]; then
    if command -v cygpath >/dev/null; then
        root="$(cygpath -m "${root}")"
    fi
    echo "VCPKG_CAIRO_PREFIX=${root}/installed/${triplet}" >> "${GITHUB_ENV}"
    if [[ "${triplet}" == *windows* ]]; then
        echo "PKG_CONFIG_EXE=${root}/installed/x64-windows/tools/pkgconf/pkgconf.exe" >> "${GITHUB_ENV}"
    fi
fi
