# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import os
import sys
sys.path.insert(0, os.path.abspath('..'))

project = 'NetworkGym'
copyright = '2023, Intel Labs Multi-Access Intelligence'
author = 'Menglei Zhang'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    "myst_parser",
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'sphinx.ext.napoleon',
    "sphinx.ext.doctest",
    'sphinxmermaid',
    "sphinx_design",
]

myst_enable_extensions = ["colon_fence"]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# Napoleon settings
napoleon_use_ivar = True
napoleon_use_admonition_for_references = True
# See https://github.com/sphinx-doc/sphinx/issues/9119
napoleon_custom_sections = [("Returns", "params_style")]

# Autodoc
autoclass_content = "both"
autodoc_preserve_defaults = True


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_book_theme"
html_title = "NetworkGym Documentation"
html_logo = "network_gym_logo.png"
html_static_path = ['_static']
