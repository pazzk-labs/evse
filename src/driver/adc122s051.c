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

#include "adc122s051.h"
#include <stdlib.h>
#include <errno.h>

#if !defined(ADC122S051_REF_MILLIVOLT)
#define ADC122S051_REF_MILLIVOLT	3300
#endif
#if !defined(ADC122S051_RESOLUTION)
#define ADC122S051_RESOLUTION		4096
#endif

struct adc122s051 {
	struct spi_device *spi;

	uint16_t *buf;
	uint16_t bufsize;
};

static uint16_t convert_raw_to_millivolt(const uint16_t raw)
{
	const uint32_t millivolt = (uint32_t)raw * 1000 / ADC122S051_RESOLUTION
		* ADC122S051_REF_MILLIVOLT / 1000;
	return (uint16_t)millivolt;
}

uint16_t adc122s051_convert_adc_to_millivolt(const uint16_t adc_raw)
{
	return convert_raw_to_millivolt(adc_raw);
}

int adc122s051_measure(struct adc122s051 *self,
		uint16_t adc_samples[], const uint16_t nr_samples,
		struct adc122s051_callback *cb)
{
	size_t samples_size = sizeof(*adc_samples) * nr_samples;
	uint8_t *tmp = (uint8_t *)adc_samples; /* for endian-agnotic */

	if (samples_size > self->bufsize) {
		return -ENOSPC;
	}

	int err = spi_writeread(self->spi, self->buf, samples_size,
			adc_samples, samples_size);

	if (!err) {
		for (uint16_t i = 0; i < nr_samples; i++) {
			uint16_t adc = (uint16_t)
				((tmp[i * 2] << 8) | tmp[i * 2 + 1]);

			if (cb) {
				adc = (*cb->on_sample)(cb->on_sample_ctx, adc);
			}

			adc_samples[i] = adc;
		}
	}

	return err;
}

struct adc122s051 *adc122s051_create(struct spi_device *spi,
		uint16_t *buf, const uint16_t bufsize)
{
	struct adc122s051 *p = malloc(sizeof(struct adc122s051));

	if (p) {
		p->spi = spi;
		p->buf = buf;
		p->bufsize = bufsize;
	}

	return p;
}

void adc122s051_destroy(struct adc122s051 *self)
{
	free(self);
}
