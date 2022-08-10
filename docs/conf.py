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

Config.set_library_file('/Library/Developer/CommandLineTools/usr/lib/libclang.dylib')
