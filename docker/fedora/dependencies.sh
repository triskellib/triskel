#!/usr/bin/sh
set -e

dnf update -y

dnf install -y \
	clang \
	clang-tools-extra \
	cmake \
	fmt-devel \
	git \
	llvm-devel \
	ninja-build \
	pkg-config \
	wget

# Cairo
dnf install -y cairo-devel

# ImGui
dnf install -y \
	glew-devel \
	glfw-devel \
	SDL2-devel \
	SDL2-static

# Imgui doesn't provide nice build steps
# so we need to revert to this :(
git clone https://github.com/ocornut/imgui.git imgui

cd imgui
cp /cxx-common/docker/fedora/imgui.CMakeLists.txt ./CMakeLists.txt
cmake -B build -G Ninja -DIMGUI_DIR=$(pwd)
cmake --build build --config Release --target install
cd ..

# STBIMAGE
wget "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
mv stb_image.h /usr/include/stb_image.h


# Gflags
dnf install -y gflags-devel

# LLVM
dnf install -y llvm-devel

# Binary analysis
dnf install -y capstone-devel
