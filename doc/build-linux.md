Compiling DynaMoHack on Linux
===========================

These steps will build a version of DynaMoHack from source that can be played in Linux.


1. Install dependencies
-----------------------

These are the package names for Ubuntu; other distros may use different names.

 * gcc or clang
 * flex
 * bison
 * libncursesw5-dev
 * zlib1g-dev
 * cmake


2. Configure the build
----------------------

After getting and extracting the game's source code (e.g. to ~/dynamohack), create a build directory and run CMake:

    cd ~/dynamohack
    mkdir build
    cd ~/dynamohack/build
    cmake .. -DINSTALL_BASE=~/dynamohack/install

Other useful flags for cmake:

    -DCMAKE_BUILD_TYPE=Debug

... to include debugging symbols; useful for stack traces.

    -DALL_STATIC:BOOL=TRUE

... to statically link all DynaMoHack-related libraries into a single `dynamohack` binary.  BEWARE: If this option is changed in a non-clean build directory the results may be unexpected; to avoid this, use `make clean`, or separate build directories for static and dynamic builds.

If you want to compile with clang and it isn't already set as your default C compiler for your system, put "CC=/usr/bin/clang" in front of the cmake command, like so:

    CC=/usr/bin/clang cmake ... (etc.)


3. Compile the game
-------------------

Enter the following commands to compile the game:

    cd ~/dynamohack/build
    make install

To play the game, run its launch script:

    ~/dynamohack/install/dynamohack/dynamohack.sh

Most options can be set and saved in-game, but if you want to customize characters used on the map, see save files or view dump logs of finished games you can find them all in ~/.config/DynaMoHack.
