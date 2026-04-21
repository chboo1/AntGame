from setuptools import setup, Extension

setup(
    name="AntGame",
    version="1.0",
    install_requires=[],
    ext_modules=[Extension(
        name="AntGame",
        include_dirs=["src/headers"],
        define_macros=[("PYTHON_COMP","b")],
        sources=["src/AntGamemodule.cpp", "src/lib/map.cpp", "src/lib/network.cpp", "src/lib/sockets.cpp"],
        extra_compile_args=["-Wno-sign-compare", "-Wno-unused-variable", "-Wno-unused-but-set-variable"]
    )],
)
