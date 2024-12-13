FM Toy
========

![Project status](https://img.shields.io/badge/Project%20status-Beta-blue.svg) ![License](https://img.shields.io/github/license/vampirefrog/fmtoy?color=green)

[![Linux Build](https://github.com/vampirefrog/fmtoy/actions/workflows/linux-jack.yml/badge.svg)](https://github.com/vampirefrog/fmtoy/actions/workflows/linux-jack.yml)

Yamaha FM sound synthesis emulation based MIDI synthesizer.

[Demo](https://vampi.tech/demos/fmtoy/)

Also used in [this database](https://fm.vampi.tech/voices).

Currently only targetting jack audio and ALSA, and also WebAssembly based browser emulation (see demo above), it accepts MIDI input, and emulates one sound chip per MIDI channel. Currently supported are YM2151, YM2203, YM2608, YM2610, YM2610B, YM2612 and YM3812. It requires at least one command line argument, which is an `.OPM` file. You can specify multiple `.OPM` files and their voices will be concatenated up to 128 voices. You can select the voice with MIDI Program Change. The voice will change for all sound chips simultaneously, so the channel number is ignored in this case. The point of this tool is to listen to each chip side by side and compare their outputs. It is useful for testing voice conversion algorithms, such as OPM to OPN.

You can get a bunch of `.OPM` files from [KVR forum](https://www.kvraudio.com/forum/viewtopic.php?t=277864).

| MIDI Channel  | Chip      |  Name  | Polyphony |
| ------------- |-----------|:------:|----------:|
| 1             | YM2151    |  OPM   |         8 |
| 2             | YM2203    |  OPN   |         3 |
| 3             | YM2608    |  OPNA  |         6 |
| 4             | YM2610    |  OPNB  |         6 |
| 5             | YM2610b   | OPNB-B |         6 |
| 6             | YM2612    |  OPN2  |         6 |
| 7             | YM3812    |  OPL2  |         9 |

Please try `./fmtoy_jack --help` to see a list of command-line options.


Building
--------

```sh
make
```

Emscripten:

```sh
emmake make wasm
```


Licensing
---------

Please see the LICENSE file for more information.
