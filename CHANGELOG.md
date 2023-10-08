# Changelog

## MLC 1.3.0 (UNRELEASED)
- Config file to control the process
- Program modes concept to implement config print and help

## MLC 1.2.0 (August 11, 2023)
- Better way of handling tags using TagLib

## MLC 1.1.0 (July 23, 2023)
- New logging system
- Artist, Album artist, Album and Title are now utf16 encoded, should fix broten titles
- BPM tag is now rounded, as it supposed to be by spec
- Lyrics is not set now for USLT tag for unsychronized lyrics is not supported in LAME
- Date bug fix, it was MMDD instead of standard DDMM

## MLC 1.0.1 (July 21, 2023)
- Added multithreaded encoding
- Only the first artist vorbis tag is left in the id3 tags

## MLC 1.0.0 (July 19, 2023)
- Initial release, only core functionality
