# -- Path setup --------------------------------------------------------------
import os
import sys
sys.path.insert(0, os.path.abspath('.'))
from subprocess import call
import mygenerator


# -- Project information -----------------------------------------------------
project = u'TinyCSocket'
copyright = u'2020, Markus Lindelöw'
author = u'Markus Lindelöw'


# -- General configuration ---------------------------------------------------
master_doc = 'index'
extensions = []
templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']



# -- Options for HTML output -------------------------------------------------
html_theme = 'classic'
html_logo = 'static/logo.png'
html_favicon = 'static/favicon.ico'
html_show_sphinx = False
html_show_copyright = False
html_title = 'Docs'
html_static_path = ['static']
html_sidebars = {
    '**': ['localtoc.html', 'searchbox.html']
}
html_use_index = False

# -- Generate doxygen xml ------
call('doxygen')
mygenerator.run()
