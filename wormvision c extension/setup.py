from setuptools import setup, Extension

setup(
    name="wormvision",
    version="1.0",
    description="",
    ext_modules=[
        Extension(
            "wormvision",
            sources=["wormvision.c",
                     "operators_basic.c",
                     "operators.c",
                     "operators_float.c",
                     "operators_int16.c",
                     "operators_rgb565.c",
                     "operators_rgb888.c"],
            py_limited_api=True)
    ]
)
# steps to fix operators_basic.c and operators.c:
# replace #warning by // #warning
#

# build (from outside venv) with "python setup.py build" in this folder
# install (from within venv) with "python setup.py install" in this folder
# visual studio build tools need to be installed and cl.exe added to the system path
# on raspberry pi: "sudo python3 setup.py install" in this folder
