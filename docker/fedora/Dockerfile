FROM fedora:41
WORKDIR /cxx-common
COPY ../.. .
RUN ./docker/fedora/dependencies.sh
RUN cmake -B build . \
    -G Ninja \
    -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++ \
    -D ENABLE_LINTING=OFF \
    -D ENABLE_TESTING=OFF \
    -DENABLE_LLVM=ON \
    -DENABLE_IMGUI=ON \
    -DENABLE_CAIRO=ON \
    -DBUILD_BENCH=ON \
    -DBUILD_GUI=ON \
    -DBUILD_IMG=ON
RUN cmake --build build