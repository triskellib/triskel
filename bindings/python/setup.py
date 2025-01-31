import os
from skbuild import setup
from pathlib import Path

CMAKE_ARGS = ["-DCMAKE_BUILD_TYPE=Release","-DENABLE_IMGUI=OFF","-DENABLE_LLVM=OFF"]



python_root = os.environ.get("Python_ROOT_DIR", "")
if python_root != "":
    CMAKE_ARGS.append(f"-DPython_ROOT_DIR={python_root}")

build_dir = os.environ.get("SKBUILD_DIR", ".")

this_directory = Path(__file__).parent
long_description = (this_directory / "README.md").read_text()

setup(
    name="pytriskel",
    version="0.2.0",
    author="Jack Royer",
    author_email="jack.royer@inria.fr",
    license="MPL2.0",
    url="https://github.com/triskellib/triskel",
    description="CFG visualization library",
    long_description=long_description,
    long_description_content_type='text/markdown',
    packages=["pytriskel"],
    keywords=["cfg", "visualization", "reverse-engineering"],
    cmake_install_dir="pytriskel",
    cmake_source_dir="../..",
    cmake_args=CMAKE_ARGS,
    zip_safe=False,
    options={"egg_info": {"egg_base": build_dir}}
)