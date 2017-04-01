Compiling DynaMoHack on Windows for Cygwin
========================================

The following steps will build a version of DynaMoHack that runs in Cygwin, a Linux-like compatibility layer for Windows.

This procedure is mostly meant for people that already have Cygwin installed and prefer its terminal to the Windows console.  Those who just want to play the game should consider using a MinGW version instead (pre-compiled or built from source).


1. Install dependecies with the Cygwin installer
------------------------------------------------

Run Cygwin's setup.exe and choose the following packages:

 * gcc
 * make
 * cmake
 * flex
 * bison
 * libncursesw-devel
 * git
 * zlib-devel


2. Clone the game's git repository
----------------------------------

In the Cygwin terminal:

    cd ~
    git clone -b unnethack https://github.com/DynaMoHack/DynaMoHack.git dynamohack

At this point compiling the game for Cygwin is similar to compiling it for Linux.  All of the remaining commands should be entered in the Cygwin terminal.


3. Configure the build
----------------------

Make the build directory, run CMake:

    cd ~/dynamohack
    mkdir build
    cd ~/dynamohack/build
    cmake ..

Use the CMake GUI to set the install path:

    cd ~/dynamohack/build
    ccmake .

Set SHELLDIR and CMAKE_INSTALL_PREFIX to /home/username/dynamohack/install.

Set BINDIR, DATADIR and LIBDIR to /home/username/dynamohack/install/dynamohack-data.

Press 'c' to configure, then 'g' to generate the build files and exit.


4. Compile the game
-------------------

Enter the following commands to compile the game:

    cd ~/dynamohack/build
    make install

To play the game, run its launch script in the Cygwin terminal:

    ~/dynamohack/install/dynamohack.sh

Most options can be set and saved in-game, but if you want to customize characters used on the map, see save files or view dump logs of finished games you can find them all in ~/.config/DynaMoHack.

Like on Linux, programs in Cygwin use baked-in paths at compile time, so it's best to compile the game from source rather than try to get a pre-compiled Cygwin build to work.  A pre-compiled native version (e.g. using MinGW) has no such caveats.
