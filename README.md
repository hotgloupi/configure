[![Build Status](https://travis-ci.org/hotgloupi/configure.svg?branch=master)](https://travis-ci.org/hotgloupi/configure)
[![Scan Status](https://scan.coverity.com/projects/4064/badge.svg)](https://scan.coverity.com/projects/4064)
[![Coverage Status](https://coveralls.io/repos/hotgloupi/configure/badge.svg?branch=master)](https://coveralls.io/r/hotgloupi/configure?branch=master)
[![Build status](https://ci.appveyor.com/api/projects/status/9m3dpenqbakd6nf0/branch/master?svg=true)](https://ci.appveyor.com/project/hotgloupi/configure/branch/master)

# configure

 *Configure your projects' builds*

`configure` reads one or more file that describe your project in Lua and generates
the files needed to build it (Makefiles, Xcode, VisualStudio projects, ...).

Please see the [documentation](http://hotgloupi.github.io/configure) to get started with `configure`.

# How to install

You can download the binaries from the [latest release](https://github.com/hotgloupi/configure/releases/latest)
and extract them in a prefix of your choice.

Here are one liners to do that:

on OS X:

    curl -L https://github.com/hotgloupi/configure/releases/download/v0.1/configure-osx.tgz -o - | sudo tar -C /usr/local -xjf -

on Linux:

    curl -L https://github.com/hotgloupi/configure/releases/download/v0.1/configure-linux.tgz -o - | tar -C /usr/local -xjf -

You might want to change the tag version `v0.1` to a more recent one, and the install prefix `/usr/local` to something else (like `~/local`).

# How to build

You'll need a compiler that supports c++11 and boost libraries.

If you have an older version of `configure` already installed somewhere, you can try to use
it to configure a build of the latest version. If it's not working, you need to bootstrap it
using one of the script in the `bootstrap/` directory.

The bootstrap scripts will generate a binary that will let you configure a real build later on.
The scripts support few environment variable that let you specify the compiler and where boost
libraries are located.

Please have a look in those script files, they are really straightforward. They roughly consist 
of calling the compiler with all the source files (`g++ ./src/configure/**/*.cpp`).

Once the temporary executable is built, you can configure a build:

    CONFIGURE_LIBRARY_DIR=src/lib /tmp/configure build
    make -C build

And then, if you're brave, you can re-configure the build with the built executable:

    ./build/bin/configure build

The steps are the same for windows, except the temporary executable `configure.exe` is generated in 
the current directory.
