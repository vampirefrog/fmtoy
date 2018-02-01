#include <asoundlib.h>
#include "midi.h"

static char *midi_note_names[12] = {
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

const char *midi_note_name(int note) {
	return midi_note_names[note % 12];
}

int midi_note_octave(int note) {
	return note / 12;
}

const char *midi_cc_name(int cc) {
	switch(cc) {
		case MIDI_CTL_ALL_NOTES_OFF: return "All notes off";
		case MIDI_CTL_ALL_SOUNDS_OFF: return "All sounds off";
		case MIDI_CTL_DATA_DECREMENT: return "Data Decrement";
		case MIDI_CTL_DATA_INCREMENT: return "Data Increment";
		case MIDI_CTL_E1_REVERB_DEPTH: return "E1 Reverb Depth";
		case MIDI_CTL_E2_TREMOLO_DEPTH: return "E2 Tremolo Depth";
		case MIDI_CTL_E3_CHORUS_DEPTH: return "E3 Chorus Depth";
		case MIDI_CTL_E4_DETUNE_DEPTH: return "E4 Detune Depth";
		case MIDI_CTL_E5_PHASER_DEPTH: return "E5 Phaser Depth";
		case MIDI_CTL_GENERAL_PURPOSE5: return "General purpose 5";
		case MIDI_CTL_GENERAL_PURPOSE6: return "General purpose 6";
		case MIDI_CTL_GENERAL_PURPOSE7: return "General purpose 7";
		case MIDI_CTL_GENERAL_PURPOSE8: return "General purpose 8";
		case MIDI_CTL_HOLD2: return "Hold2";
		case MIDI_CTL_LEGATO_FOOTSWITCH: return "Legato foot switch";
		case MIDI_CTL_LOCAL_CONTROL_SWITCH: return "Local control switch";
		case MIDI_CTL_LSB_BALANCE: return "Balance";
		case MIDI_CTL_LSB_BANK: return "Bank selection";
		case MIDI_CTL_LSB_BREATH: return "Breath";
		case MIDI_CTL_LSB_DATA_ENTRY: return "Data entry";
		case MIDI_CTL_LSB_EFFECT1: return "Effect1";
		case MIDI_CTL_LSB_EFFECT2: return "Effect2";
		case MIDI_CTL_LSB_EXPRESSION: return "Expression";
		case MIDI_CTL_LSB_FOOT: return "Foot";
		case MIDI_CTL_LSB_GENERAL_PURPOSE1: return "General purpose 1";
		case MIDI_CTL_LSB_GENERAL_PURPOSE2: return "General purpose 2";
		case MIDI_CTL_LSB_GENERAL_PURPOSE3: return "General purpose 3";
		case MIDI_CTL_LSB_GENERAL_PURPOSE4: return "General purpose 4";
		case MIDI_CTL_LSB_MAIN_VOLUME: return "Main volume";
		case MIDI_CTL_LSB_MODWHEEL: return "Modulation";
		case MIDI_CTL_LSB_PAN: return "Panpot";
		case MIDI_CTL_LSB_PORTAMENTO_TIME: return "Portamento time";
		case MIDI_CTL_MONO1: return "Mono1";
		case MIDI_CTL_MONO2: return "Mono2";
		case MIDI_CTL_MSB_BALANCE: return "Balance";
		case MIDI_CTL_MSB_BANK: return "Bank selection";
		case MIDI_CTL_MSB_BREATH: return "Breath";
		case MIDI_CTL_MSB_DATA_ENTRY: return "Data entry";
		case MIDI_CTL_MSB_EFFECT1: return "Effect1";
		case MIDI_CTL_MSB_EFFECT2: return "Effect2";
		case MIDI_CTL_MSB_EXPRESSION: return "Expression";
		case MIDI_CTL_MSB_FOOT: return "Foot";
		case MIDI_CTL_MSB_GENERAL_PURPOSE1: return "General purpose 1";
		case MIDI_CTL_MSB_GENERAL_PURPOSE2: return "General purpose 2";
		case MIDI_CTL_MSB_GENERAL_PURPOSE3: return "General purpose 3";
		case MIDI_CTL_MSB_GENERAL_PURPOSE4: return "General purpose 4";
		case MIDI_CTL_MSB_MAIN_VOLUME: return "Main volume";
		case MIDI_CTL_MSB_MODWHEEL: return "Modulation";
		case MIDI_CTL_MSB_PAN: return "Panpot";
		case MIDI_CTL_MSB_PORTAMENTO_TIME: return "Portamento time";
		case MIDI_CTL_NONREG_PARM_NUM_LSB: return "Non-registered parameter number";
		case MIDI_CTL_NONREG_PARM_NUM_MSB: return "Non-registered parameter number";
		case MIDI_CTL_OMNI_OFF: return "Omni off";
		case MIDI_CTL_OMNI_ON: return "Omni on";
		case MIDI_CTL_PORTAMENTO: return "Portamento";
		case MIDI_CTL_PORTAMENTO_CONTROL: return "Portamento control";
		case MIDI_CTL_REGIST_PARM_NUM_LSB: return "Registered parameter number";
		case MIDI_CTL_REGIST_PARM_NUM_MSB: return "Registered parameter number";
		case MIDI_CTL_RESET_CONTROLLERS: return "Reset Controllers";
		case MIDI_CTL_SC10: return "SC10";
		case MIDI_CTL_SC1_SOUND_VARIATION: return "SC1 Sound Variation";
		case MIDI_CTL_SC2_TIMBRE: return "SC2 Timbre";
		case MIDI_CTL_SC3_RELEASE_TIME: return "SC3 Release Time";
		case MIDI_CTL_SC4_ATTACK_TIME: return "SC4 Attack Time";
		case MIDI_CTL_SC5_BRIGHTNESS: return "SC5 Brightness";
		case MIDI_CTL_SC6: return "SC6";
		case MIDI_CTL_SC7: return "SC7";
		case MIDI_CTL_SC8: return "SC8";
		case MIDI_CTL_SC9: return "SC9";
		case MIDI_CTL_SOFT_PEDAL: return "Soft pedal";
		case MIDI_CTL_SOSTENUTO: return "Sostenuto";
		case MIDI_CTL_SUSTAIN: return "Sustain pedal";
	}

	return "Unknown";
}
