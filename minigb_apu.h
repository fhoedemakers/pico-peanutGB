/**
 * minigb_apu is released under the terms listed within the LICENSE file.
 *
 * minigb_apu emulates the audio processing unit (APU) of the Game Boy. This
 * project is based on MiniGBS by Alex Baines: https://github.com/baines/MiniGBS
 */

#pragma once

#include <stdint.h>

#define AUDIO_SAMPLE_RATE	44100   // was 32768
//
#define DMG_CLOCK_FREQ		4194304.0

/* There is deliberately no AUDIO_SAMPLES here any more. It used to be
 * AUDIO_SAMPLE_RATE / (DMG_CLOCK_FREQ / SCREEN_REFRESH_CYCLES) = 738, the
 * number of samples in one *Game Boy* frame (59.7275 Hz). But the emulator is
 * paced by the display, not by the Game Boy, and the audio sinks are clocked
 * from the pixel clock -- so producing 738 per frame drifted against them and
 * caused periodic dropouts. How many samples a frame needs is a property of the
 * output side, so it is decided there: see emu_audio_frame_budget() in main.cpp,
 * which is passed to audio_callback() through its len argument. */

/**
 * Fill allocated buffer "data" with "len" BYTES of audio: len/4 stereo frames,
 * each a pair of native-endian int16_t (left then right). "len" is honoured, so
 * one video frame's audio may be rendered in several successive calls; all APU
 * state persists between them.
 */
void audio_callback(void *ptr, uint8_t *data, int len);

/**
 * Read audio register at given address "addr".
 */
uint8_t audio_read(const uint16_t addr);

/**
 * Write "val" to audio register at given address "addr".
 */
void audio_write(const uint16_t addr, const uint8_t val);

/**
 * Initialise audio driver.
 */
void audio_init(void);
