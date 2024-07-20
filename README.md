# luigi

A barebones GUI library for Win32 and X11.

This is a fork of [nakst's luigi](https://github.com/nakst/luigi) library,
but with changes that may not be acceptable to that library. The major changes
are:

* No longer single header.
* Built using CMake.
* Removal of Essence port (CMake does not support Essence).

There are also slight changes that make this no longer compatible with nakst's
version. The biggest change is making the `ui` context variable opaque.
Accessing properties of this structure are done via some new functions. Please
see example apps.
