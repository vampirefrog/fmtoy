#ifndef SYX_FB01_H_
#define SYX_FB01_H_

#include <stdint.h>
#include <stddef.h>

// YAMAHA FM SOUND GENERATOR
// FB-01
// SERVICE MANUAL
// Page 19
// Voice parameter list

struct fb01_bulk_voice_op {
	uint8_t tl;                  // TOTAL LEVEL                           0-127 (0=Max. level)
	unsigned ks_type:2;          // KEYBOARD SCALING (LEVEL) TYPE BIT0/1  0-3
	unsigned tl_sensitivity:3;   // VELOCITY SENSITIVITY FOR TL           0-7
	unsigned ks_level_depth:4;   // KEYBOARD SCALING (LEVEL) DEPTH        0-15
	unsigned tl_adjust:4;        // ADJUST FOR TL                         0-15
	unsigned detune:3;           // DETUNE                                0-7
	unsigned freq:4;             // FREQUENCY                             0-15
	unsigned ks_rate_depth:2;    // KEYBOARD SCALING (RATE) DEPTH         0-3
	unsigned ar:5;               // ATTACK RATE                           0-31
	unsigned carrier:1;          // MODULATOR/CARRIER FLAG                0,1 (1=carrier)
	unsigned ar_velocity_sens:2; // VELOCITY SENSITIVITY FOR AR           0-3
	unsigned d1r:5;              // DECAY 1 RATE                          0-31
	unsigned inharmonic_freq:2;  // INHARMONIC FREQUENCY                  0-3
	unsigned d2r:5;              // DECAY 2 RATE                          0-31
	unsigned sl:4;               // SUSTAIN LEVEL                         0-15
	unsigned rr:4;               // RELEASE RATE                          0-15 (0=Max. level)
};

struct fb01_bulk_voice {
	char name[7];                // VOICE NAME                            ASCII
	uint8_t user_code;           // USER'S CODE                           0-255
	uint8_t lfo_speed;           // LFO SPEED                             0-255
	unsigned lfo_enable:1;       // ENABLE TO LOAD LFO DATA               0,1 (1=ENABLE)
	unsigned am_depth:7;         // AMPLITUDE MODULATION DEPTH            0-127
	unsigned lfo_sync:1;         // LFO SYNC                              0-1(1=SYNC ON)
	unsigned pm_depth:7;         // PITCH MODULATION DEPTH                0-127
	unsigned op_mask:4;          // ENABLE OPERATOR                       0,1 (1=ON) BIT MAP OP4 => OP1
	unsigned l_enable:1;         // LEFT OUTPUT ENABLE                    0,1 (1=ON) FB-01 Ignored
	unsigned r_enable:1;         // RIGHT OUTPUT ENABLE                   0,1 (1=ON) FB-01 Ignored
	unsigned fb_level:3;         // FEED BACK LEVEL                       0-6
	unsigned algorithm:3;        // ALGORITHM                             0-7
	unsigned pm_sens:3;          // PITCH MODULATION SENSITIVITY          0-7
	unsigned am_sens:2;          // AMPLITUDE MODULATION SENSITIVITY      0-3
	unsigned lfo_waveform:2;     // LFO WAVE FORM                         0-3
	int8_t transpose;            // TRANSPOSE                             0-255 (2's complement)
	struct fb01_bulk_voice_op op[4]; // OP1 - OP4
	unsigned mono:1;             // POLY'MONO                             0,1 (1=MONO MODE)
	unsigned portamento_speed:7; // PORTAMENTO SPEED                      0-127
	unsigned pmd_controller:3;   // INPUT CONTROLLER (PMD)                0-4 (0=OFF, 1=TOUCH, 2=WHEEL, 3=BREATH, 4=FOOT)
	unsigned pitch_bend_range:4; // PITCHBENDER RANGE                     0-12
};

struct fb01_bulk_voice_bank {
	uint8_t channel;
	uint8_t bank;
	char name[8];
	uint8_t num_voices;
	struct fb01_bulk_voice voices[48];
};

#define FB01_ALL_STATUSES \
	FB01_STATUS(SUCCESS, "Success") \
	FB01_STATUS(IN_PROGRESS, "In progress") \
	FB01_STATUS(FILE_TOO_SMALL, "File too small") \
	FB01_STATUS(NOT_SYSEX, "Not sysex") \
	FB01_STATUS(BAD_ID_NO, "Bad id no") \
	FB01_STATUS(BAD_SUBSTATUS, "Bad substatus") \
	FB01_STATUS(BAD_CHANNEL, "Bad channel") \
	FB01_STATUS(BAD_MESSAGE_NO, "Bad message no") \
	FB01_STATUS(BAD_SYSTEM_NO, "Bad system no") \
	FB01_STATUS(BAD_OPERATION_NO, "Bad operation no") \
	FB01_STATUS(BAD_FORMAT_NO, "Bad format no") \
	FB01_STATUS(BAD_BANK_NO, "Bad bank no") \
	FB01_STATUS(TOO_SMALL, "Too small") \
	FB01_STATUS(TOO_BIG, "Too big") \
	FB01_STATUS(BAD_DATA_LOW, "Bad data low bits") \
	FB01_STATUS(BAD_DATA_HIGH, "Bad data high bits") \
	FB01_STATUS(BAD_BYTE_COUNT_LSB, "Bad byte count LSB") \
	FB01_STATUS(BAD_BYTE_COUNT_MSB, "Bad byte count MSB") \
	FB01_STATUS(BAD_CHECKSUM, "Bad checksum") \
	FB01_STATUS(AFTER_EOX, "After EOX") \
	FB01_STATUS(EARLY_EOX, "Early EOX") \
	FB01_STATUS(SHORT_WRITE, "Short write") \
	FB01_STATUS(TOO_MANY_VOICES, "Too many voices")

enum fb01_error {
#define FB01_STATUS(s, d) FB01_##s,
	FB01_ALL_STATUSES
#undef FB01_STATUS
	FB01_MAX_ERR
};

enum fb01_midi_receiver_state {
	FB01_RX_SYSEX_START,
	FB01_RX_ID_NO,
	FB01_RX_SUB_STATUS,
	FB01_RX_FORMAT_NO,
	FB01_RX_SYSTEM_NO,
	FB01_RX_MESSAGE_NO,
	FB01_RX_OP_NO,
	FB01_RX_BANK_NO,
	FB01_RX_BYTE_COUNT_MSB_OR_EOX, // Either EOX or more data
	FB01_RX_BYTE_COUNT_LSB,
	FB01_RX_DATA_LOW,
	FB01_RX_DATA_HIGH,
	FB01_RX_CHECKSUM,
	FB01_RX_EOX,
};

enum fb01_midi_receiver_voice_state {
	FB01_RX_V_IN_NAME,
	FB01_RX_V_IN_PADDING,
	FB01_RX_V_IN_VOICE,
	FB01_RX_V_AFTER_VOICES,
};

// state machine to parse MIDI data byte by byte
struct fb01_midi_receiver {
	enum fb01_midi_receiver_state state;
	int substatus, system_no, bank_no, byte_count, data_pos;
	uint8_t data_byte, accumulator;

	enum fb01_midi_receiver_voice_state voicestate;
	int voicenum, voicepos;
	char namebuf[8];
	struct fb01_bulk_voice voice;

	void *data_ptr;
	void (*voice_cb)(struct fb01_bulk_voice *voice, int voicenum, void *data_ptr);
	void (*bulk_voice_data_start)(uint8_t channel, uint8_t bank, int voicenum, char *name, void *data_ptr);
	void (*bulk_voice_data_end)(int num_voices, void *data_ptr);
};

const char *fb01_error_string(enum fb01_error err);
const char *fb01_input_controller_name(int c);
const char *fb01_lfo_waveform_name(int w);

int fb01_midi_receive_voice(struct fb01_midi_receiver *rx, uint8_t byte);
void fb01_midi_receiver_init(struct fb01_midi_receiver *rx);
int fb01_midi_receive(struct fb01_midi_receiver *rx, uint8_t byte);

int fb01_bulk_voice_bank_from_buffer(struct fb01_bulk_voice_bank *bank, uint8_t *buf, size_t filesize);
int fb01_bulk_voice_bank_send(struct fb01_bulk_voice_bank *bank, size_t (*write)(void *, size_t, void *), void *data_ptr);
#ifndef __EMSCRIPTEN__
void fb01_bulk_voice_dump(struct fb01_bulk_voice *voice, int voicenum);
void fb01_bulk_voice_bank_dump(struct fb01_bulk_voice_bank *bank);
#endif /* __EMSCRIPTEN */

#endif /* SYX_FB01_H_ */
