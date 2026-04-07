"""Build script for the nvdssr_ext C extension module."""

import subprocess

from setuptools import Extension, setup


def _pkg_config(lib, flag):
    """Run pkg-config and return a list of stripped tokens."""
    try:
        out = subprocess.check_output(
            ["pkg-config", flag, lib], text=True
        ).strip()
        return out.split() if out else []
    except (subprocess.CalledProcessError, FileNotFoundError):
        return []


def _strip_prefix(tokens, prefix):
    return [t[len(prefix):] for t in tokens if t.startswith(prefix)]


gst_cflags = _pkg_config("gstreamer-1.0", "--cflags")
gst_libs = _pkg_config("gstreamer-1.0", "--libs")

include_dirs = (
    _strip_prefix(gst_cflags, "-I")
    + ["/opt/nvidia/deepstream/deepstream/sources/includes"]
)
library_dirs = _strip_prefix(gst_libs, "-L")
libraries = _strip_prefix(gst_libs, "-l")

ext = Extension(
    "nvdssr_ext",
    sources=["nvdssr_ext.c"],
    include_dirs=include_dirs,
    library_dirs=library_dirs,
    libraries=libraries,
)

setup(
    name="nvdssr_ext",
    version="1.0",
    ext_modules=[ext],
)
