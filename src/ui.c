/*
 * السوار الوطني - واجهة السوار على لوحة التطوير Thingy:91 X
 *
 * الأزرار:
 *   الزر ١ = «أنا بأمان» (الأيمن الأخضر في السوار)
 *   الزر ٢ = «إغلاق التنبيه» (الأيسر)
 *   ضغط الزرين معًا ٣ ثوانٍ = إنذار تجريبي (وإن كان هناك إنذار: زوال خطر تجريبي)
 *
 * الإضاءة: نفترض LED1 أحمر و LED2 أخضر و LED3 أزرق في مكتبة DK.
 * إن اختلف الترتيب في اللوحة يُعدَّل هنا فقط.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

#include "ui.h"
#include "state.h"

LOG_MODULE_REGISTER(ui, LOG_LEVEL_INF);

#define LED_R DK_LED1
#define LED_G DK_LED2
#define LED_B DK_LED3

#define BTN_SAFE    DK_BTN1_MSK
#define BTN_DISMISS DK_BTN2_MSK
#define COMBO_HOLD  K_SECONDS(3)

static enum ui_light current_light = UI_LIGHT_OFF;

/* ---------------- الإضاءة ---------------- */

static void leds_apply(enum ui_light c)
{
	dk_set_led(LED_R, c == UI_LIGHT_AMBER);
	dk_set_led(LED_G, c == UI_LIGHT_AMBER || c == UI_LIGHT_GREEN);
	dk_set_led(LED_B, 0);
}

void ui_light(enum ui_light color)
{
	current_light = color;
	leds_apply(color);
}

/* ---------------- الاهتزاز ----------------
 * لا يوجد محرك اهتزاز في لوحة التطوير، فنحاكيه بوميض الضوء.
 * في السوار الحقيقي: أوامر DRV2605L بالقوة المحفوظة في الإعدادات.
 */
static bool vib_on;
static bool vib_phase;

static void vib_fn(struct k_work *w);
static K_WORK_DELAYABLE_DEFINE(vib_work, vib_fn);

static void vib_fn(struct k_work *w)
{
	if (!vib_on) {
		leds_apply(current_light);
		return;
	}
	vib_phase = !vib_phase;
	leds_apply(vib_phase ? current_light : UI_LIGHT_OFF);
	k_work_reschedule(&vib_work, K_MSEC(400));
}

void ui_vibrate_alert(void)
{
	LOG_INF("[vibration] alert pattern ON");
	vib_on = true;
	k_work_reschedule(&vib_work, K_NO_WAIT);
}

void ui_vibrate_short(void)
{
	LOG_INF("[vibration] short pulse");
}

void ui_vibrate_stop(void)
{
	if (vib_on) {
		LOG_INF("[vibration] OFF");
	}
	vib_on = false;
	k_work_reschedule(&vib_work, K_NO_WAIT);
}

/* ---------------- الصوت ---------------- */

void ui_sound_alert(void)
{
	/* TODO(السوار الحقيقي): نغمة الطنّان حسب الإعداد، أو كتم الصوت */
	LOG_INF("[sound] alert tone");
}

/* ---------------- الشاشة ----------------
 * لا توجد شاشة في لوحة التطوير، فنكتب المحتوى في السجل.
 */
void ui_screen_off(void)
{
	LOG_INF("[screen] off");
}

void ui_screen_status(void)
{
	LOG_INF("[screen] battery %d%% | connected", ui_battery_percent());
}

void ui_screen_text(const char *text)
{
	LOG_INF("[screen] %s", text);
}

void ui_family_safe(const char *name)
{
	LOG_INF("[screen] %s is safe", name);
}

int ui_battery_percent(void)
{
	/* TODO: قراءة مقياس البطارية من nPM1300 */
	return 85;
}

/* ---------------- الأزرار ---------------- */

static uint32_t held;
static bool combo_fired;

static void combo_fn(struct k_work *w)
{
	combo_fired = true;
	enum siwar_state s = state_get();

	if (s == ST_ALERT || s == ST_ALERT_MUTED || s == ST_SAFE) {
		LOG_WRN("TEST: simulated all-clear");
		state_post_simple(EV_ALL_CLEAR);
	} else {
		LOG_WRN("TEST: simulated national alert");
		state_post_simple(EV_ALERT);
	}
}
static K_WORK_DELAYABLE_DEFINE(combo_work, combo_fn);

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	uint32_t pressed = button_state & has_changed;
	uint32_t released = ~button_state & has_changed;

	held |= pressed;

	if ((button_state & (BTN_SAFE | BTN_DISMISS)) == (BTN_SAFE | BTN_DISMISS)) {
		combo_fired = false;
		k_work_reschedule(&combo_work, COMBO_HOLD);
		return;
	}

	if (released) {
		k_work_cancel_delayable(&combo_work);
		bool was_combo = (held & (BTN_SAFE | BTN_DISMISS)) == (BTN_SAFE | BTN_DISMISS);

		if (button_state == 0) {
			if (!was_combo && !combo_fired) {
				if (held & BTN_SAFE) {
					state_post_simple(EV_BTN_SAFE);
				} else if (held & BTN_DISMISS) {
					state_post_simple(EV_BTN_DISMISS);
				}
			}
			held = 0;
			combo_fired = false;
		}
	}
}

int ui_init(void)
{
	int err = dk_leds_init();

	if (err) {
		LOG_ERR("LEDs init failed (%d)", err);
		return err;
	}
	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Buttons init failed (%d)", err);
		return err;
	}
	leds_apply(UI_LIGHT_OFF);
	return 0;
}
