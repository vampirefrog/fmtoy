FM Toy
========

Currently only targetting jack audio and ALSA, it accepts MIDI input, and emulates one sound chip per MIDI channel. Currently supported are YM2151, YM2203, YM2608, YM2610, YM2610B and YM2612. It requires at least one command line argument, which is an `.OPM` file. You can specify multiple `.OPM` files and their voices will be concatenated up to 128 voices. You can select the voice with MIDI Program Change. The voice will change for all sound chips simultaneously, so the channel number is ignored in this case. The point of this tool is to listen to each chip side by side and compare their outputs. It is useful for testing voice conversion algorithms, such as OPM to OPN.

You can get a bunch of `.OPM` files from [KVR forum](https://www.kvraudio.com/forum/viewtopic.php?t=277864).

| MIDI Channel  | Chip      |
| ------------- |-----------|
| 1             | YM2151    |
| 2             | YM2203    |
| 3             | YM2608    |
| 4             | YM2610    |
| 5             | YM2610b   |
| 6             | YM2612    |

vgm2opm
-------
Similar to Shiru's vgm2opm, it extracts 4-op FM voices from vgm files. Unlike Shiru's vgm2opm, it supports more chips, such as the OPN\* series and OPM, and limited support for OPL\*. It uses roughly the same algorithm to compact duplicate voices.

Please try `./fmtoy_jack --help` to see a list of command-line options.

Dumping tools
-------------
`opmdump`, `insdump`, `dmpdump`, `tfidump`, `y12dump` dump voice files in csv format. These are meant for importing the data into a database.
