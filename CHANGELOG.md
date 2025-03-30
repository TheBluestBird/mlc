# Changelog

## MLC 1.3.4 (March 30, 2025)
- Build fixes
- Source and Destination paths now can contain spaces
- Regex to copy non music files can have spaces
- A feature to define a regex to exclude paths from rendering

## MLC 1.3.3 (October 13, 2023)
- Regex to specify non-music files to copy
- Encoding settings (VBR/CBR, encoding quality, output quality)

## MLC 1.3.2 (October 10, 2023)
- A release purely for CI

## MLC 1.3.1 (October 10, 2023)
- Release build with optimizations
- Removed unused files from build
- Suppressed warnings
- CI to release to AUR

## MLC 1.3.0 (October 09, 2023)
- Config file to control the process
- First help page
- Program modes concept to implement config print and help

## MLC 1.2.0 (August 11, 2023)
- Better way of handling tags using TagLib

## MLC 1.1.0 (July 23, 2023)
- New logging system
- Artist, Album artist, Album and Title are now utf16 encoded, should fix broken titles
- BPM tag is now rounded, as it supposed to be by spec
- Lyrics is not set now for USLT tag for unsynchronized lyrics is not supported in LAME
- Date bug fix, it was MMDD instead of standard DDMM

## MLC 1.0.1 (July 21, 2023)
- Added multithreaded encoding
- Only the first artist vorbis tag is left in the id3 tags

## MLC 1.0.0 (July 19, 2023)
- Initial release, only core functionality
