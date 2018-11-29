from setuptools import setup, Extension

setup(
    name="wormvision",
    version="1.0",
    description="",
    ext_modules=[
        Extension(
            "wormvision",
            sources=["wormvision.c"],
            py_limited_api=True)
    ]
)

# build (from outside venv) with python setup.py build
# install (from within venv) with python setup.py install
# visual studio build tools need to be installed and cl.exe added to the system path
