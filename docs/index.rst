###########
TinyCSocket
###########

Tinycsocket is a thin cross-platform socket library written in C99. It focuses
on a minimal footprint, cross-platform and to also provide simple lowlevel utils
(for example tcs_simple_connect(...) which resolves and connects to a hostname).
The API is similar to BSD sockets with some differences. All functions return an
error-code. The advantage is that the error handling is simple to understand and
to handle for all plattforms. The disadvantage is that the functions can not be
easily integrated in expressions.

Currently support plattforms:

- Windows 2000 SP1 and newer (XP, Server 2003, Vista, Server 2008, 7, 8, 8.1 and 10)
- POSIX.1-2001 systems


*************************
Installation Instructions
*************************
Depending on if you use cmake or not there are two installation instructions.

Integrate with CMake
====================
If you are using a cmake project, it is recommended to include tinycsocket to
your build system. Add this repo as a submodule and add tinycsocket/src to your
CMakeLists.txt.

.. code:: bash

   git submodule add https://gitlab.com/dosshell/tinycsocket.git
   add_subdirectory(tinycsocket/src)
   target_link_libraries(your_target PRIVATE tinycsocket)


You can read more about how to use submodules here: https://git-scm.com/book/en/v2/Git-Tools-Submodules

Manually Linking
================
You can also build this project to get a lib directory and an include directoy.
Generate a build-system out of tinycsocket with cmake and build the install
target. Don't forget that if you are targeting Windows you also need to link to
wsock32.lib and ws2_32.lib.
The following commands will create these include- and lib folders in a folder named
install:

.. code:: bash

   git clone https://gitlab.com/dosshell/tinycsocket.git
   mkdir build
   cd build
   cmake -DCMAKE_INSTALL_PREFIX:Path=../install ../tinycsocket
   cmake --build . --target INSTALL --config Release

You can now remove the build directory and the tinycsocket directory if you
like.


***********************
Reference Documentation
***********************

This is the part you probably are looking for.

List of All Functions
=====================

List of all available functions in TinyCSocket:

.. csv-table::
    :widths: auto
    :file: _mybuild/function_index.csv
    :header-rows: 1


List of All Constants
=====================

List of all available constants in TinyCSocket:

.. csv-table::
    :widths: auto
    :file: _mybuild/variable_index.csv
    :header-rows: 1


Reference Details
=================

This section contains details of all functions.

.. include:: _mybuild/function_references.rst
