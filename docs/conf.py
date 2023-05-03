import os
import sys
from subprocess import call

import pypandoc

sys.path.insert(0, os.path.abspath("."))
import reference_generator


# Project information
project = "TinyCSocket"
copyright = "2020, Markus Lindelöw"
author = "Markus Lindelöw"

# General configuration
master_doc = "index"
extensions = []
templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

# html options
html_theme = "classic"
html_logo = "static/logo.png"
html_favicon = "static/favicon.ico"
html_show_sphinx = False
html_show_copyright = False
html_title = "Docs"
html_static_path = ["static"]
html_sidebars = {"**": ["localtoc.html", "searchbox.html"]}
html_use_index = False

html_css_files = ["custom.css"]

# Run generators
os.makedirs("_mybuild", exist_ok=True)
pypandoc.convert_file(
    "../README.md",
    "rst",
    outputfile="_mybuild/README.rst",
    extra_args=["--wrap=none", "--columns=1024"],
)
ret = call("doxygen")
reference_generator.generate_references()
if ret != 0:
    sys.exit(ret)
