#pragma once

#include <stdint.h>

const char *midi_note_name(int note);
int midi_note_octave(int note);
const char *midi_cc_name(int cc);
float midi_note_freq(uint8_t note);
