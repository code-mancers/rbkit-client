rbkit-client
============
[![Build Status](https://travis-ci.org/code-mancers/rbkit-client.svg?branch=master)](https://travis-ci.org/code-mancers/rbkit-client)

![Rbkit](https://github.com/code-mancers/rbkit-client/raw/master/images/rbkit2.gif "RBkit use")

A build for OSX can be downloaded from:

https://github.com/code-mancers/rbkit-client/releases/tag/0.1.2

# Development

## Requirements

You need Qt on your system in order to build the project. It's recommended to
install Qt Creator which is an IDE for Qt projects.

## Compiling on Linux

Assuming you have Qt 5.3.x version installed somewhere on your Linux box, after
cloning the repository you should run:

```bash
qmake
make -j4
```

And that should give you `rbkit-app/RBKitClient` executable, which you can
go ahead and run.

## Qt build issue on OSX Mavericks

Qt binary distribution compiles with `-stdlib=libstdc++` for std lib,
where as Xcode 5 on Mavericks compiles with `-stdlib=libc++`. So when you
are using libraries compiled outside of Qt in the app, the linker will not
be able to resolve the symbols in the stdlib. Fix this by doing to following.
Go to `Projects` tab in Qt and under `Build Steps`, expand the qmake step and
look for `-spec` argument. You need to add a flag `QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9`
in the `qmake.conf` file of that spec.

For example:

My qmake command is

```bash
qmake /Users/emil/OpenSource/rbkit-client/RbkitClient.pro -r -spec macx-g++
CONFIG+=debug CONFIG+=x86_64 CONFIG+=declarative_debug CONFIG+=qml_debug
```

Since value of `-spec` is macx-g++, I'll need to add a line with
`QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9` in `~/Qt5.2.1/5.2.1/clang_64/mkspecs/macx-g++/qmake.conf`

*Note* - Above special syntax is usually not required on Linux systems.

Running a shadow build, i.e building from a separate directory is very
simple. Just create a build directory, say `~/rbkit-build`. Lets say
source code is cloned at `~/rbkit-client`

```bash
$ mkdir -p ~/rbkit-build
$ cd ~/rbkit-build

# in build directory, one can invoke qmake and specify cloned directory
# as source. This is pretty much shallow build.
$ qmake ~/rbkit-client -r -spec macx-g++ CONFIG+=debug CONFIG+=x86_64
CONFIG+=declarative_debug CONFIG+=qml_debug

# Makefile will be generated by qmake. Just run make
$ make
```

## Testing Framework integration
Right now, QTest framework is integrated. So, if you make a build, then
tests will be built under `$BUILD_DIR/tests` folder. Those executables
can be run from commandline directly.

NOTE: For running benchmarks, there should be a file called `hugedump`
under `tests/msgpack` folder. This file is not checked in, because in
my machine, it is occupying 67mb. So, its better to keep this file
out of source control. For running tests, simply copy the file
`tests/msgpack/objectdump` to `tests/msgpack/hugedump`.


```bash
$ mkdir -p ~/rbkit-build
$ cd ~/rbkit-build

# in build directory, one can invoke qmake and specify cloned directory
# as source. This is pretty much shallow build.
$ qmake ~/rbkit-client -r -spec macx-g++ CONFIG+=debug CONFIG+=x86_64
CONFIG+=declarative_debug CONFIG+=qml_debug

# Makefile will be generated by qmake. Just run make
$ make

# run tests by invoking rctests
$ ./tests/rctests
```

## Debugging Qt Webkit applications.

After adding following line in code,

```cpp
QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
```

You can debug js applications running inside Qt via simple "inspect element", like we do in Chrome or
other webkit browsers. Dead simple.

## Cleaning up build files
If you suspect that there are build files left over in msgpack and zeromq folders,
run this command to clean them up.

```sh
  $ git clean -dxf msgpack-c zeromq zeromq-4.0.4
```

# Releasing a new version

### Create a dmg for OSX platform
Update the VERSION value in RbkitClient.pro file to reflect the new version.
Run `macdeployqt RbkitClient.app -dmg` where `RbKitClient.app` is the OSX deployment
created by Qt Creator when you built the project locally. Doing this gives you
a `dmg` file.

### Tag and draft a release
Github allows you to draft releases and attach files along with the releases.
[Click here](https://github.com/code-mancers/rbkit-client/releases/new) to draft
a release. Releases always happen from the master branch. Tag the commit with
the version to be released. We follow [semantic versioning](http://semver.org/).
Upload the dmg file on the release draft and submit.
