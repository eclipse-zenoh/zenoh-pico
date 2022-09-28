#
# Copyright (c) 2022 ZettaScale Technology
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
# which is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
#
# Contributors:
#   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
#

# Configuration file for the Sphinx documentation builder.
from clang.cindex import Config
from sys import platform
from pathlib import Path

# -- Project information -----------------------------------------------------
project = 'zenoh-pico'
copyright = '2017, 2022 ZettaScale Technology Inc'
author = 'ZettaScale Zenoh team'
release = '0.6.0'

# -- General configuration ---------------------------------------------------
master_doc = 'index'
extensions = ['sphinx_c_autodoc', 'sphinx_c_autodoc.napoleon']
language = 'c'
c_autodoc_roots = ['../include/zenoh-pico/api/']

# -- Options for HTML output -------------------------------------------------
html_theme = 'sphinx_rtd_theme'

breathe_debug_trace_directives = True

if platform == "darwin":
    libclang_file = Path("/Library/Developer/CommandLineTools/usr/lib/libclang.dylib")
    libclang_cellar = Path("/usr/local/Cellar/llvm/14.0.6/lib/libclang.dylib")
    if libclang_file.is_file():
        Config.set_library_file(libclang_file)
    elif libclang_cellar.is_file():
        Config.set_library_file(libclang_cellar)
    else:
        raise ValueError(f"libclang not found. \nTried: \n {libclang_file}\n {libclang_cellar}")

elif platform == "win32":
    raise ValueError("Windows not supported yet for building docs.")

else:
    Config.set_library_file('/usr/lib/llvm-6.0/lib/libclang.so.1') # Required for readthedocs
