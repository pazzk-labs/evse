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

#include <stdlib.h>
#include <errno.h>

#include "libmcu/board.h"
#include "libmcu/metrics.h"
#include "libmcu/metricfs.h"
#include "libmcu/nvs_kvstore.h"
#include "libmcu/actor.h"
#include "libmcu/actor_timer.h"
#include "libmcu/wdt.h"
#include "libmcu/cleanup.h"
#include "libmcu/apptmr.h"
#include "libmcu/ratelim.h"
#include "libmcu/i2c.h"

#include "logger.h"
#include "exio.h"
#include "config.h"
#include "secret.h"
#include "buzzer.h"
#include "safety.h"
#include "usrinp.h"
#include "uptime.h"
#include "fs/kvstore.h"
#include "fs/logfs.h"
#include "net/netmgr.h"
#include "updater.h"
#include "app.h"

#define ACTOR_MEMSIZE_BYTES		256U
#define ACTOR_STACKSIZE_BYTES		8192U

#define METRIC_PERIOD_MS		(60U/*min*/ * 60/*sec*/ * 1000)
#define METRIC_STORAGE_MAXLEN		720 /* 30-day worth of data */
#define METRIC_PREFIX			"metrics"

#define RUNNER_WDT_TIMEOUT_MS		(1U/*sec*/ * 1000)
#define CLEANUP_TIMEOUT_MS		(2U/*sec*/ * 1000)

static uint8_t actor_mem[ACTOR_MEMSIZE_BYTES];
static uint8_t actor_timer_mem[ACTOR_MEMSIZE_BYTES];

static struct actor runner_actor;
static struct actor exio_actor;
static struct actor metric_actor;
static struct actor cleanup_actor;
static struct apptmr *cleanup_timer;
static struct ratelim cleanup_limit;
static struct wdt *runner_wdt;

static struct app app;

static void update_metrics(struct app *papp,
		const uint32_t t0, const uint32_t t1)
{
	size_t used_bytes;
	fs_usage(papp->fs, &used_bytes, NULL);

	metrics_set(HeartbeatInterval, METRICS_VALUE(t1 - t0));
	metrics_set(MonotonicTime, METRICS_VALUE(t1));
	metrics_set(SystemTime, METRICS_VALUE(time(NULL)));
	metrics_set(Uptime, METRICS_VALUE(uptime_get()));
	metrics_set(CPULoad, METRICS_VALUE(board_cpuload(0, 1)));
	metrics_set(CPULoad2, METRICS_VALUE(board_cpuload(1, 1)));
	metrics_set(HeapLowWatermark, METRICS_VALUE(board_get_heap_watermark()));
	metrics_set(AppTimerCreatedCount, METRICS_VALUE(apptmr_len()));
	metrics_set(FileSystemUsage, METRICS_VALUE(used_bytes));
	metrics_set(LogFsCount, METRICS_VALUE(logfs_count(papp->logfs)));
	metrics_set(LogFsSize, METRICS_VALUE(logfs_size(papp->logfs, 0)));
}

static bool save_metrics(struct app *self, const uint32_t t0, const uint32_t t1)
{
	const size_t metrics_bufsize = metrics_count() * 8;
	uint8_t *encoded;
	metricfs_id_t id;
	int err;

	if ((encoded = (uint8_t *)malloc(metrics_bufsize)) == NULL) {
		metrics_increase(OOM);
		error("Failed to allocate memory for encoding.");
		return false;
	}

	update_metrics(self, t0, t1);

	const size_t encoded_len = metrics_collect(encoded, metrics_bufsize);

	if (!(err = metricfs_write(self->mfs, encoded, encoded_len, &id))) {
		metrics_reset();

		info("\"%u\" save%s(%d): %u bytes",
				id, !err? "d" : " failed", err, encoded_len);
	}

	free(encoded);

	return !err;
}

static void runner_handler(struct actor *actor, struct actor_msg *msg)
{
	static uint32_t interval_ms;
	static uint32_t t0;

	const uint32_t t = board_get_time_since_boot_ms();

	wdt_feed(runner_wdt);

	if (t - t0 >= interval_ms) {
		t0 = t;
		app_process(&interval_ms);
		metrics_increase(RunnerDispatchCount);
		metrics_set(RunnerNextInterval, METRICS_VALUE(interval_ms));
	}

	actor_free(msg);
	actor_send_defer(actor, NULL, interval_ms);
}

static void exio_handler(struct actor *actor, struct actor_msg *msg)
{
	unused(actor);
	exio_intr_t intr = exio_get_intr_source();

	if (intr & EXIO_INTR_EMERGENCY) {
		usrinp_raise(USRINP_EMERGENCY_STOP);
	}
	if (intr & EXIO_INTR_USB_CONNECT) {
		usrinp_raise(USRINP_USB_CONNECT);
	}
	if (intr & EXIO_INTR_ACCELEROMETER) {
	}

	actor_free(msg);
	metrics_increase(ExioDispatchCount);
}

static void metric_handler(struct actor *actor, struct actor_msg *msg)
{
	static uint32_t t0;
	const uint32_t t = board_get_time_since_boot_ms();
	const uint32_t elapsed = t - t0;

	if (elapsed >= METRIC_PERIOD_MS) {
		metrics_increase(MetricDispatchCount);
		if (save_metrics(&app, t0, t)) {
			t0 = t;
		}
	}

	actor_free(msg);
	actor_send_defer(actor, NULL, METRIC_PERIOD_MS / 2);
}

static void cleanup_handler(struct actor *actor, struct actor_msg *msg)
{
	unused(actor);
	info("Cleaning up...");

	apptmr_stop(cleanup_timer);
	cleanup_execute();
	metrics_increase(CleanupDispatchCount);

	info("Rebooting...");
	metrics_increase(ResetRequestCount);
	board_reboot();

	actor_free(msg);
}

static void raise_cleanup(void)
{
	/* NOTE: In case of actor is stuck, a timeout is also registered for
	 * backup. cleanup_handler() might be called twice in this case. */
	apptmr_start(cleanup_timer, CLEANUP_TIMEOUT_MS);
	actor_send(&cleanup_actor, NULL);
}

static void on_cleanup_timeout(struct apptmr *timer, void *ctx)
{
	unused(timer);
	struct actor *actor = (struct actor *)ctx;
	metrics_increase(CleanupTimeoutCount);
	cleanup_handler(actor, NULL);
}

static void on_config_save(void *ctx)
{
	unused(ctx);
	info("Config saved in NVS.");
}

static void on_watchdog_timeout(struct wdt *wdt, void *ctx)
{
	unused(ctx);
	if (ratelim_request(&cleanup_limit)) {
		raise_cleanup();
		error("Watchdog timeout: %s", wdt_name(wdt));
	}
	metrics_increase(WDTCount);
}

static void on_watchdog_periodic(void *ctx)
{
	unused(ctx);
	metrics_set(WDTStackHighWatermark,
			METRICS_VALUE(board_get_current_stack_watermark()));
}

static void on_exio_intr(struct gpio *gpio, void *ctx)
{
	unused(gpio);
	unused(ctx);
	actor_send(&exio_actor, NULL);
}

static int get_exio_state(usrinp_t source)
{
	if (source == USRINP_EMERGENCY_STOP) {
		return exio_get_intr_level(EXIO_INTR_EMERGENCY);
	} else if (source == USRINP_USB_CONNECT) {
		return exio_get_intr_level(EXIO_INTR_USB_CONNECT);
	}
	return -EINVAL;
}

static void create_i2c_devices(struct pinmap_periph *p)
{
	i2c_enable(p->i2c0);
	i2c_reset(p->i2c0);

	p->io_expander = i2c_create_device(p->i2c0, 0x74, 400000);
	p->codec = i2c_create_device(p->i2c0, 0x18, 400000);
	p->temp = i2c_create_device(p->i2c0, 0x49, 400000);
	p->acc = i2c_create_device(p->i2c0, 0x19, 400000);
}

static bool update(void *ctx)
{
	unused(ctx);
	if (updater_process() == -EAGAIN) {
		return true;
	}
	return false;
}

static void run_network_updater(void *ctx)
{
	netmgr_register_task(update, ctx);
}

void reboot_gracefully(void)
{
	raise_cleanup();
}

int main(void)
{
	const board_reboot_reason_t reboot_reason = board_get_reboot_reason();
	struct pinmap_periph *periph = &app.periph;
	uint32_t network_healthchk_interval_ms = 0;

	board_init(); /* should be called very first. */

	metrics_init(0);
	metrics_increase(ResetCount);

	pinmap_init(periph);
	uptime_init();
	cleanup_init();
	wdt_init(on_watchdog_periodic, NULL);
	config_init(nvs_kvstore_new(), on_config_save, NULL);
	secret_init(nvs_kvstore_new());

	app.fs = fs_create(flash_create(0));
	fs_mount(app.fs);
	app.kvstore = fs_kvstore_create(app.fs);
	app.mfs = metricfs_create(app.kvstore,
			METRIC_PREFIX, METRIC_STORAGE_MAXLEN);
	app.logfs = logfs_create(app.fs, LOGGER_FS_BASE_PATH,
			LOGGER_FS_MAX_SIZE, LOGGER_FS_MAX_LOGS,
			LOGGER_FS_CACHE_SIZE);
	logger_init(app.logfs);

	info("%s(%s) v%s: Booting from %s.",
			board_name(),
			board_get_serial_number_string(),
			board_get_version_string(),
			board_get_reboot_reason_string(reboot_reason));

	create_i2c_devices(periph);
	exio_init(periph->io_expander, periph->io_expander_reset);
	exio_set_audio_power(false);

	actor_init(actor_mem, sizeof(actor_mem), ACTOR_STACKSIZE_BYTES);
	actor_timer_init(actor_timer_mem, sizeof(actor_timer_mem));
	actor_set(&runner_actor, runner_handler, 0);
	actor_set(&exio_actor, exio_handler, 0);
	actor_set(&metric_actor, metric_handler, 0);
	actor_set(&cleanup_actor, cleanup_handler, 0);

	safety_init(periph->input_power, periph->output_power);
	safety_enable();

	config_get("net.health", &network_healthchk_interval_ms,
			sizeof(network_healthchk_interval_ms));
	netmgr_init(network_healthchk_interval_ms);
	updater_init();
	updater_set_runner(run_network_updater, NULL);

	usrinp_init(periph->debug_button, get_exio_state);
	gpio_register_callback(periph->io_expander_int, on_exio_intr, NULL);
	gpio_enable(periph->io_expander_int);

	/* NOTE: around 3kHz and 9kHz will be the most high-pitched sound on our
	 * buzzer. which is G note in octave 7 and B note in octave 8. */
	buzzer_init(periph->pwm2_ch2_buzzer, 0, 0);
	buzzer_play(&melody_booting);

	runner_wdt = wdt_new("runner", RUNNER_WDT_TIMEOUT_MS, 0, 0);
	wdt_enable(runner_wdt);
	wdt_register_timeout_cb(on_watchdog_timeout, NULL);

	cleanup_timer = apptmr_create(false, on_cleanup_timeout, &cleanup_actor);
	ratelim_init(&cleanup_limit, RATELIM_UNIT_MINUTE, 1, 1);

	app_init(&app);

	actor_send(&runner_actor, NULL);
	actor_send(&metric_actor, NULL);

	metrics_set(ResetReason, METRICS_VALUE(reboot_reason));
	metrics_set(InitStackHighWatermark,
			METRICS_VALUE(board_get_current_stack_watermark()));
	metrics_set(BootingTime, METRICS_VALUE(board_get_time_since_boot_ms()));

	return 0;
}
