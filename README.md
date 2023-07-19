# MLC - Music Library Compiler

This is a program for compilation of your loseless music library to lossy formats

### Prerequisites

- flac
- lame
- jpeg

### Building

```
$ git clone https://git.macaw.me/blue/mlc
$ cd mlc
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

### Usage

```
./mlc path/to/loseless/library path/to/store/lossy/library
```

For now the program is very primitive, it only works to convert `.flac` files (it trusts the suffix) to `.mp3` VBR best possible quality slowest possible conversion algorithm.

MLC keeps file structure, copies all the non `.flac` files, tries to adapt some `.flac` (vorbis) tags to id3v1 and id3v2 of destination `.mp3` file.

For now it's siglethread, no interrupt controll, not even sure converted files are valid, no exotic cases handled, no conversion options can be passed. May be will improve in the futer.
