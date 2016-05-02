# Oryol Extension Samples

These are the Oryol Extension Samples, meaning samples which depend on
extension modules that are not a core part of Oryol.

Oryol is here: http://github.com/floooh/oryol

The Extension Samples webpage is here: http://floooh.github.com/oryol-samples

### Build Status:

|Platform|Build Status|
|--------|------|
|OSX + Linux (OpenGL)|[![Build Status](https://travis-ci.org/floooh/oryol-samples.svg?branch=master)](https://travis-ci.org/floooh/oryol-samples)|
|Windows (OpenGL + D3D11)|[![Build status](https://ci.appveyor.com/api/projects/status/t1l7s1hobnocpn6q/branch/master?svg=true)](https://ci.appveyor.com/project/floooh/oryol-samples/branch/master)|

### How to build

You need a recent cmake, python 2.7.x (3.x should work too), and a
working C/C++ development environment:

- Windows: a recent Visual Studio
- OSX: Xcode and with command line tools
- Linux: gcc, X Window, OpenGL and ALSA headers and libs


To build and run one of the samples:
```bash
> mkdir projects
> cd projects
> git clone https://github.com/floooh/oryol-samples
> cd oryol-samples

# build and run a sample
> ./fips build
> ./fips run Paclone

# open project in VStudio or XCode:
> ./fips open
```

See the [Oryol build instructions](https://github.com/floooh/oryol/blob/master/doc/BUILD.md)
and [fips build system documentation](http://floooh.github.io/fips/)
for more detailed build instructions.

