/*
 * This file is part of the Pazzk project <https://pazzk.net/>.
 * Copyright (c) 2024 Pazzk <team@pazzk.net>.
 *
 * Community Version License (GPLv3):
 * This software is open-source and licensed under the GNU General Public
 * License v3.0 (GPLv3). You are free to use, modify, and distribute this code
 * under the terms of the GPLv3. For more details, see
 * <https://www.gnu.org/licenses/gpl-3.0.en.html>.
 * Note: If you modify and distribute this software, you must make your
 * modifications publicly available under the same license (GPLv3), including
 * the source code.
 *
 * Commercial Version License:
 * For commercial use, including redistribution or integration into proprietary
 * systems, you must obtain a commercial license. This license includes
 * additional benefits such as dedicated support and feature customization.
 * Contact us for more details.
 *
 * Contact Information:
 * Maintainer: 권경환 Kyunghwan Kwon (on behalf of the Pazzk Team)
 * Email: k@pazzk.net
 * Website: <https://pazzk.net/>
 *
 * Disclaimer:
 * This software is provided "as-is", without any express or implied warranty,
 * including, but not limited to, the implied warranties of merchantability or
 * fitness for a particular purpose. In no event shall the authors or
 * maintainers be held liable for any damages, whether direct, indirect,
 * incidental, special, or consequential, arising from the use of this software.
 */

#include "buzzer.h"

#define UNIT_INTERVAL		150
#define REST_INTERVAL		50

static const struct tone beep[] = {
	{ .note = NOTE_G,       .octave = OCTAVE_7, .len_ms = 50 },
};

static const struct tone booting[] = {
	{ .note = NOTE_C,       .octave = OCTAVE_7, .len_ms = 100 },
	{ .note = NOTE_D,       .octave = OCTAVE_7, .len_ms = 100 },
	{ .note = NOTE_E,       .octave = OCTAVE_7, .len_ms = 150 },
};

static const struct tone plugged[] = {
	{ .note = NOTE_G,       .octave = OCTAVE_7, .len_ms = 150 },
	{ .note = NOTE_A,       .octave = OCTAVE_8, .len_ms = 150 },
};

static const struct tone unplugged[] = {
	{ .note = NOTE_A,       .octave = OCTAVE_8, .len_ms = 150 },
	{ .note = NOTE_G,       .octave = OCTAVE_7, .len_ms = 150 },
};

static const struct tone sleep[] = {
	{ .note = NOTE_G,       .octave = OCTAVE_5, .len_ms = 150 },
	{ .note = NOTE_F,       .octave = OCTAVE_4, .len_ms = 150 },
	{ .note = NOTE_E,       .octave = OCTAVE_4, .len_ms = 150 },
};

struct melody melody_beep = {
	.tones = beep,
	.nr_tones = sizeof(beep) / sizeof(beep[0]),
};

struct melody melody_booting = {
	.tones = booting,
	.nr_tones = sizeof(booting) / sizeof(booting[0]),
};

struct melody melody_plugged = {
	.tones = plugged,
	.nr_tones = sizeof(plugged) / sizeof(plugged[0]),
};

struct melody melody_unplugged = {
	.tones = unplugged,
	.nr_tones = sizeof(unplugged) / sizeof(unplugged[0]),
};

struct melody melody_sleep = {
	.tones = sleep,
	.nr_tones = sizeof(sleep) / sizeof(sleep[0]),
};
