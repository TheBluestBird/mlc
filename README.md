# MLC - Music Library Compiler

[![AUR version](https://img.shields.io/aur/version/mlc?style=flat-square)](https://aur.archlinux.org/packages/mlc/)

This is a program to compile your local lossless music library to lossy formats

### Prerequisites

- flac
- lame
- jpeg
- taglib

### Building

```sh
$ git clone https://git.macaw.me/blue/mlc
$ cd mlc
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

### Usage

Just to compile lossless library to lossy you can use this command
assuming you are in the same directory you have just built `MLC`:

```
./mlc path/to/lossless/library path/to/store/lossy/library
```

There are more ways to use `MLC`, please refer to help for more options:

```sh
$ ./mlc help
# or
$ ./mlc --help
#
# or
$ ./mlc -h
```

`MLC` has a way to configure conversion process, it will generate global for you user config on the first launch.
It will be located in `~/.config/mlc.conf` and `MLC` will notify you when it's generated.

You can also make local configs for each directory you launch mlc from.

To output the default config run the following, assuming you are in the same directory you have just built `MLC`:

```sh
$ ./mlc config
```

To use non default config run the following, assuming you are in the same directory you have just built `MLC`:

```sh
$ ./mlc path/to/lossless/library path/to/store/lossy/library -c path/to/config/file
```

### About

For now the program is very primitive, it only works to convert `.flac` files (it trusts the suffix) to `.mp3`.
I'm planing to add more lossy formats first then, may be, more lossless.

`MLC` keeps file structure, copies all the non `.flac` files, tries to adapt some `.flac` (vorbis) tags to id3v1 and id3v2 of destination `.mp3` file.
You can set up output format and quality and which kind of files should it skip in the config.

By default `MLC` runs on as many threads as your system can provide to speed up conversion process. 
You can set up correct amount of threads in the config.
