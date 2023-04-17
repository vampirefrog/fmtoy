#ifndef SYX_DX21_H_
#define SYX_DX21_H_

#include <stdint.h>
#include <stddef.h>

// DIGITAL PROGRAMMABLE ALGORITHM SYNTHESIZER
// DX21
// SERVICE MANUAL
// Page 15
// 5-2. VOICE PARAMETERS (VCED format)

// FM TONE GENERATOR
// TX81Z
// SERVICE MANUAL
// Page 20
// Voice Edit Additional Parameters (ACED)

struct dx21_vced_voice_op {
	uint8_t ar;               // ATTACK RATE                                  0 - 31
	uint8_t d1r;              // DECAY 1 RATE                                 0 - 31
	uint8_t d2r;              // DECAY 2 RATE                                 0 - 31
	uint8_t rr;               // RELEASE RATE                                 0 - 15
	uint8_t d1l;              // DECAY 1 LEVEL                                0 - 15
	uint8_t ksl;              // KEYBOARD SCALING LEVEL                       0 - 99
	uint8_t ksr;              // KEYBOARD SCALING RATE                        0 - 3
	uint8_t eg_bias_sens;     // EG BIAS SENSITIVITY                          0 - 7
	uint8_t ame;              // AMPLITUDE MODULATION ENABLE                  0 - 1
	uint8_t kv;               // KEY VELOCITY                                 0 - 7
	uint8_t ol;               // OUTPUT LEVEL                                 0 - 99
	uint8_t freq;             // OSCILLATOR FREQUENCY                         0 - 63
	uint8_t detune;           // DETUNE 1                                     0 - 7

	// TX81Z only
	uint8_t fix;              // Fixed Frequency                              0 - 1
	uint8_t fix_range;        // Fixed Frequency Range                        0 - 7 0(250Hz) - 7 (32kHz)
	uint8_t fin_ratio;        // Frequency Range Fine                         0 - 15
	uint8_t opw;              // Operator Waveform                            0 - 7
	uint8_t shft;             // EG Shift                                     0 - 3 0(96dB), 1(48dB), 2(24dB), 3(12dB)
};

struct dx21_vced_voice {
	struct dx21_vced_voice_op op[4];
	uint8_t algorithm;        // ALGORITHM                                    0 - 7
	uint8_t feedback;         // FEEDBACK LEVEL                               0 - 7
	uint8_t lfo_speed;        // LFO SPEED                                    0 - 99
	uint8_t lfo_delay;        // LFO DELAY                                    0 - 99
	uint8_t lfo_pmd;          // PITCH MODULATION DEPTH                       0 - 99
	uint8_t lfo_amd;          // AMPLITUDE MODULATION DEPTH                   0 - 99
	uint8_t lfo_sync;         // LFO SYNC                                     0 - 1
	uint8_t lfo_wave;         // LFO WAVE                                     0 - 3
	uint8_t pm_sens;          // PITCH MODULATION SENSITIVITY                 0 - 7
	uint8_t am_sens;          // AMPLITUDE MODULATION SENSITIVITY             0 - 7
	uint8_t middle_c;         // TRANSPOSE                                    0 - 48
	uint8_t mono_mode;        // PLAY MODE POLY/MONO                          0 - 1
	uint8_t pitch_bend_range; // PITCH BEND RANGE                             0 - 12
	uint8_t fingered_porta;   // PORTAMENTO MODE                              0 - 1
	uint8_t porta_time;       // PORTAMENTO TIME                              0 - 99
	uint8_t fc_volume;        // FOOT VOLUME                                  0 - 99
	uint8_t fc_sustain;       // SUSTAIN FOOT SWITCH                          0 - 1
	uint8_t fc_porta;         // PORTAMENTO FOOT SWITCH                       0 - 1
	uint8_t chorus;           // CHORUS SWITCH                                0 - 1
	uint8_t mw_pitch;         // MODULATION WHEEL PITCH MODULATION RANGE      0 - 99
	uint8_t mw_amplitude;     // MODULATION WHEEL AMPLITUDE MODULATION RANGE  0 - 99
	uint8_t bc_pitch;         // BREATH CONTROL PITCH MODULATION RANGE        0 - 99
	uint8_t bc_amplitude;     // BREATH CONTROL AMPLITUDE MODULATION RANGE    0 - 99
	uint8_t bc_pitch_bias;    // BREATH CONTROL PITCH BIAS RANGE              0 - 99
	uint8_t bc_eg_bias;       // BREATH CONTROL EG BIAS RANGE                 0 - 99
	char name[10];            // VOICE NAME

	// DX21 only
	uint8_t peg_rate_1;       // PITCH EG RATE 1                              0 - 99
	uint8_t peg_rate_2;       // PITCH EG RATE 2                              0 - 99
	uint8_t peg_rate_3;       // PITCH EG RATE 3                              0 - 99
	uint8_t level_1;          // PITCH EG LEVEL 1                             0 - 99
	uint8_t level_2;          // PITCH EG LEVEL 2                             0 - 99
	uint8_t level_3;          // PITCH EG LEVEL 3                             0 - 99

	// TX81Z only
	uint8_t reverb_rate;      // Reverb Rate                                  0 - 7 0(off), 7(fast)
	uint8_t fc_pitch;         // Foot Controller Pitch                        0 - 99
	uint8_t fc_amplitude;     // Foot Controller Amplitude                    0 - 99
};

struct dx21_vced_voice_bank {
	uint8_t channel;
	uint8_t num_voices;
	struct dx21_vced_voice voices[48];
};

#define DX21_ALL_STATUSES \
	DX21_STATUS(SUCCESS, "Success") \
	DX21_STATUS(IN_PROGRESS, "In progress") \
	DX21_STATUS(FILE_TOO_SMALL, "File too small") \
	DX21_STATUS(NOT_SYSEX, "Not sysex") \
	DX21_STATUS(BAD_ID_NO, "Bad id no") \
	DX21_STATUS(BAD_SUBSTATUS, "Bad substatus") \
	DX21_STATUS(BAD_CHANNEL, "Bad channel") \
	DX21_STATUS(BAD_MESSAGE_NO, "Bad message no") \
	DX21_STATUS(BAD_SYSTEM_NO, "Bad system no") \
	DX21_STATUS(BAD_OPERATION_NO, "Bad operation no") \
	DX21_STATUS(BAD_FORMAT_NO, "Bad format no") \
	DX21_STATUS(BAD_BANK_NO, "Bad bank no") \
	DX21_STATUS(TOO_SMALL, "Too small") \
	DX21_STATUS(TOO_BIG, "Too big") \
	DX21_STATUS(BAD_BYTE_COUNT_LSB, "Bad byte count LSB") \
	DX21_STATUS(BAD_BYTE_COUNT_MSB, "Bad byte count MSB") \
	DX21_STATUS(BAD_CHECKSUM, "Bad checksum") \
	DX21_STATUS(AFTER_EOX, "After EOX") \
	DX21_STATUS(EARLY_EOX, "Early EOX") \
	DX21_STATUS(SHORT_WRITE, "Short write") \
	DX21_STATUS(TOO_MANY_VOICES, "Too many voices") \
	DX21_STATUS(BAD_VCED_AR, "BAD_VCED_AR") \
	DX21_STATUS(BAD_VCED_D1R, "BAD_VCED_D1R") \
	DX21_STATUS(BAD_VCED_D2R, "BAD_VCED_D2R") \
	DX21_STATUS(BAD_VCED_RR, "BAD_VCED_RR") \
	DX21_STATUS(BAD_VCED_D1L, "BAD_VCED_D1L") \
	DX21_STATUS(BAD_VCED_KSL, "BAD_VCED_KSL") \
	DX21_STATUS(BAD_VCED_KSR, "BAD_VCED_KSR") \
	DX21_STATUS(BAD_VCED_EG_BIAS_SENS, "BAD_VCED_EG_BIAS_SENS") \
	DX21_STATUS(BAD_VCED_AME, "BAD_VCED_AME") \
	DX21_STATUS(BAD_VCED_KV, "BAD_VCED_KV") \
	DX21_STATUS(BAD_VCED_OL, "BAD_VCED_OL") \
	DX21_STATUS(BAD_VCED_FREQ, "BAD_VCED_FREQ") \
	DX21_STATUS(BAD_VCED_DETUNE, "BAD_VCED_DETUNE") \
	DX21_STATUS(BAD_VCED_ALGORITHM, "BAD_VCED_ALGORITHM") \
	DX21_STATUS(BAD_FEEDBACK, "BAD_FEEDBACK") \
	DX21_STATUS(BAD_LFO_SPEED, "BAD_LFO_SPEED") \
	DX21_STATUS(BAD_LFO_DELAY, "BAD_LFO_DELAY") \
	DX21_STATUS(BAD_LFO_PMD, "BAD_LFO_PMD") \
	DX21_STATUS(BAD_LFO_AMD, "BAD_LFO_AMD") \
	DX21_STATUS(BAD_LFO_SYNC, "BAD_LFO_SYNC") \
	DX21_STATUS(BAD_LFO_WAVE, "BAD_LFO_WAVE") \
	DX21_STATUS(BAD_PM_SENS, "BAD_PM_SENS") \
	DX21_STATUS(BAD_AM_SENS, "BAD_AM_SENS") \
	DX21_STATUS(BAD_MIDDLE_C, "BAD_MIDDLE_C") \
	DX21_STATUS(BAD_MONO_MODE, "BAD_MONO_MODE") \
	DX21_STATUS(BAD_PITCH_BEND_RANGE, "BAD_PITCH_BEND_RANGE") \
	DX21_STATUS(BAD_FINGeRED_pORTA, "BAD_FINGeRED_pORTA") \
	DX21_STATUS(BAD_PORTA_TIME, "BAD_PORTA_TIME") \
	DX21_STATUS(BAD_FC_VOLUME, "BAD_FC_VOLUME") \
	DX21_STATUS(BAD_FC_SUSTAIN, "BAD_FC_SUSTAIN") \
	DX21_STATUS(BAD_FC_PORTA, "BAD_FC_PORTA") \
	DX21_STATUS(BAD_CHORUS, "BAD_CHORUS") \
	DX21_STATUS(BAD_MW_PITCH, "BAD_MW_PITCH") \
	DX21_STATUS(BAD_MW_AMPLITUDE, "BAD_MW_AMPLITUDE") \
	DX21_STATUS(BAD_BC_PITCH, "BAD_BC_PITCH") \
	DX21_STATUS(BAD_BC_AMPLITUDE, "BAD_BC_AMPLITUDE") \
	DX21_STATUS(BAD_BC_PITCH_BIAS, "BAD_BC_PITCH_BIAS") \
	DX21_STATUS(BAD_BC_EG_BIAS, "BAD_BC_EG_BIAS") \
	DX21_STATUS(BAD_PEG_RATE_1, "BAD_PEG_RATE_1") \
	DX21_STATUS(BAD_PEG_RATE_2, "BAD_PEG_RATE_2") \
	DX21_STATUS(BAD_PEG_RATE_3, "BAD_PEG_RATE_2") \
	DX21_STATUS(BAD_LEVEL_1, "BAD_LEVEL_1") \
	DX21_STATUS(BAD_LEVEL_2, "BAD_LEVEL_2") \
	DX21_STATUS(BAD_LEVEL_3, "BAD_LEVEL_3") \
	DX21_STATUS(BAD_VMEM_AR, "BAD_VMEM_AR") \
	DX21_STATUS(BAD_VMEM_D1R, "BAD_VMEM_D1R") \
	DX21_STATUS(BAD_VMEM_D2R, "BAD_VMEM_D2R") \
	DX21_STATUS(BAD_VMEM_RR, "BAD_VMEM_RR") \
	DX21_STATUS(BAD_VMEM_D1L, "BAD_VMEM_D1L") \
	DX21_STATUS(BAD_VMEM_KSL, "BAD_VMEM_KSL") \
	DX21_STATUS(BAD_VMEM_AME_EBS_KVS, "BAD_VMEM_AME_EBS_KVS") \
	DX21_STATUS(BAD_VMEM_OL, "BAD_VMEM_OL") \
	DX21_STATUS(BAD_VMEM_FREQ, "BAD_VMEM_FREQ") \
	DX21_STATUS(BAD_VMEM_RS_DET, "BAD_VMEM_RS_DET") \
	DX21_STATUS(BAD_VMEM_DET, "BAD_VMEM_DET") \
	DX21_STATUS(BAD_CH_MO_SU_PO_PM, "BAD_CH_MO_SU_PO_PM") \
	DX21_STATUS(BAD_SHFT_FIX_FIXRG, "BAD_SHFT_FIX_FIXRG") \
	DX21_STATUS(BAD_OPW_FINE, "BAD_OPW_FINE") \
	DX21_STATUS(BAD_REVERB_RATE, "BAD_REVERB_RATE") \
	DX21_STATUS(BAD_FC_PITCH, "BAD_FC_PITCH") \
	DX21_STATUS(BAD_FC_AMPLITUDE, "BAD_FC_AMPLITUDE")

enum dx21_error {
#define DX21_STATUS(s, d) DX21_##s,
	DX21_ALL_STATUSES
#undef DX21_STATUS
	DX21_MAX_ERR
};

enum dx21_midi_receiver_state {
	DX21_RX_SYSEX_START,
	DX21_RX_ID_NO,
	DX21_RX_SUB_STATUS,
	DX21_RX_FORMAT_NO,
	DX21_RX_BYTE_COUNT_MSB_OR_EOX, // Either EOX or more data
	DX21_RX_BYTE_COUNT_LSB,
	DX21_RX_DATA,
	DX21_RX_CHECKSUM,
	DX21_RX_EOX,
};

// state machine to parse MIDI data byte by byte
struct dx21_midi_receiver {
	enum dx21_midi_receiver_state state;
	int channel, format, byte_count, data_pos;
	uint8_t accumulator;

	int voicenum, voicepos;
	char namebuf[8];
	struct dx21_vced_voice voice;

	void *data_ptr;
	void (*voice_cb)(struct dx21_vced_voice *voice, int voicenum, void *data_ptr);
	void (*bulk_voice_data_start)(uint8_t channel, uint8_t bank, int voicenum, char *name, void *data_ptr);
	void (*bulk_voice_data_end)(int num_voices, void *data_ptr);
};

const char *dx21_error_string(enum dx21_error err);
const char *dx21_input_controller_name(int c);
const char *dx21_lfo_waveform_name(int w);

int dx21_midi_receive_voice(struct dx21_midi_receiver *rx, uint8_t byte);
void dx21_midi_receiver_init(struct dx21_midi_receiver *rx);
int dx21_midi_receive(struct dx21_midi_receiver *rx, uint8_t byte);

int dx21_vced_voice_bank_from_buffer(struct dx21_vced_voice_bank *bank, uint8_t *buf, size_t filesize);
int dx21_vced_voice_bank_send(struct dx21_vced_voice_bank *bank, size_t (*write)(void *, size_t, void *), void *data_ptr);
#ifndef __EMSCRIPTEN__
void dx21_vced_voice_dump(struct dx21_vced_voice *voice, int voicenum);
void dx21_vced_voice_bank_dump(struct dx21_vced_voice_bank *bank);
#endif /* __EMSCRIPTEN */

#endif /* SYX_DX21_H_ */
