# Font toy experiment

This repository contains an experiment to create a font generator tool
that is based on constraint optimization. The approach is similar to
METAFONT.

There is nothing specific that makes this approach specific to fonts.
It could be used for arbitrary graphics generation, just like
METAPOST. This is not a priority at the moment, but may be implemented
in the future.

## Using it

The simplest way to try the program is to use the [Webassembly
version](https://jpakkane.github.io/fonttoyweb/) which can be run
directly in the web browser.

The code can be run natively as well. This results in an executable
called `fonttoy` that you can run:

    ./fonttoy path/to/file.fdef

The build depends on `liblbfgs` and `tinyxml2`. The code builds with
Meson and will download the dependencies automatically from
[WrapDB](https://wrapdb.mesonbuild.com/) automatically if they are not
available on the system. On Debian derived distros the dependencies
can be installed with:

    sudo apt install liblbfgs-dev libtinyxml2-dev

The implementation is in C++17. Compiling the Webassembly version
requires a version of Meson that is not yet in master, but there is an
outstanding pull request.
