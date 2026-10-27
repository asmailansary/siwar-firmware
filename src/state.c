/*
 * السوار الوطني - آلة الحالات
 *
 *  IDLE --زر--> AWAKE --١٠ث--> IDLE
 *  أي حالة --إنذار--> ALERT
 *  ALERT --إغلاق--> ALERT_MUTED
 *  ALERT / ALERT_MUTED --أنا بأمان--> SAFE  (يرسل لأساور الأهل)
 *  ALERT / ALERT_MUTED / SAFE --زوال الخطر--> CLEAR --٣د--> IDLE
 *
 * ملاحظة مهمة: خارج الإنذار، الأزرار تضيء الشاشة فقط ولا ترسل شيئًا،
 * حتى لا تُرسل «أنا بأمان» بالخطأ.
 */
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "state.h"
#include "ui.h"
#include "family.h"

LOG_MODULE_REGISTER(state, LOG_LEVEL_INF);

K_MSGQ_DEFINE(ev_q, sizeof(struct siwar_event), 16, 4);

static enum siwar_state cur = ST_IDLE;

static void screen_timeout_fn(struct k_work *w) { state_post_simple(EV_SCREEN_TIMEOUT); }
static void clear_timeout_fn(struct k_work *w) { state_post_simple(EV_CLEAR_TIMEOUT); }
static K_WORK_DELAYABLE_DEFINE(screen_timer, screen_timeout_fn);
static K_WORK_DELAYABLE_DEFINE(clear_timer, clear_timeout_fn);

static const char *state_name(enum siwar_state s)
{
	switch (s) {
	case ST_IDLE: return "IDLE";
	case ST_AWAKE: return "AWAKE";
	case ST_ALERT: return "ALERT";
	case ST_ALERT_MUTED: return "ALERT_MUTED";
	case ST_SAFE: return "SAFE";
	case ST_CLEAR: return "CLEAR";
	}
	return "?";
}

static void enter(enum siwar_state next)
{
	LOG_INF("%s -> %s", state_name(cur), state_name(next));
	cur = next;

	switch (next) {
	case ST_IDLE:
		ui_light(UI_LIGHT_OFF);
		ui_vibrate_stop();
		ui_screen_off();
		break;
	case ST_AWAKE:
		ui_light(UI_LIGHT_OFF);
		ui_screen_status();
		k_work_reschedule(&screen_timer, K_SECONDS(CONFIG_SIWAR_SCREEN_TIMEOUT_S));
		break;
	case ST_ALERT:
		k_work_cancel_delayable(&screen_timer);
		k_work_cancel_delayable(&clear_timer);
		ui_light(UI_LIGHT_AMBER);
		ui_screen_text("Alert: go to a safe place"); /* «توجّه لمكان آمن» */
		ui_vibrate_alert();
		ui_sound_alert();
		break;
	case ST_ALERT_MUTED:
		ui_vibrate_stop();
		break;
	case ST_SAFE:
		ui_vibrate_stop();
		ui_light(UI_LIGHT_GREEN);
		ui_screen_text("Sent to your family"); /* «تم الإرسال لأهلك» */
		family_broadcast_safe();
		break;
	case ST_CLEAR:
		ui_vibrate_stop();
		ui_light(UI_LIGHT_GREEN);
		ui_screen_text("All clear"); /* «زال الخطر» */
		ui_vibrate_short();
		k_work_reschedule(&clear_timer, K_SECONDS(CONFIG_SIWAR_CLEAR_HOLD_S));
		break;
	}
}

static void handle(const struct siwar_event *ev)
{
	/* أحداث تعمل في كل الحالات */
	if (ev->type == EV_ALERT) {
		enter(ST_ALERT);
		return;
	}
	if (ev->type == EV_FAMILY_SAFE) {
		LOG_INF("Family member safe: %s", ev->name);
		ui_family_safe(ev->name);
		ui_vibrate_short();
		if (cur == ST_IDLE) {
			enter(ST_AWAKE);
		}
		return;
	}

	switch (cur) {
	case ST_IDLE:
	case ST_AWAKE:
		if (ev->type == EV_BTN_SAFE || ev->type == EV_BTN_DISMISS) {
			enter(ST_AWAKE); /* يعيد مؤقت الـ١٠ ثوانٍ */
		} else if (ev->type == EV_SCREEN_TIMEOUT && cur == ST_AWAKE) {
			enter(ST_IDLE);
		}
		break;

	case ST_ALERT:
	case ST_ALERT_MUTED:
		if (ev->type == EV_BTN_SAFE) {
			enter(ST_SAFE);
		} else if (ev->type == EV_BTN_DISMISS && cur == ST_ALERT) {
			enter(ST_ALERT_MUTED);
		} else if (ev->type == EV_ALL_CLEAR) {
			enter(ST_CLEAR);
		}
		break;

	case ST_SAFE:
		if (ev->type == EV_ALL_CLEAR) {
			enter(ST_CLEAR);
		}
		break;

	case ST_CLEAR:
		if (ev->type == EV_CLEAR_TIMEOUT) {
			enter(ST_IDLE);
		}
		break;
	}
}

int state_post(const struct siwar_event *ev)
{
	return k_msgq_put(&ev_q, ev, K_NO_WAIT);
}

int state_post_simple(enum siwar_event_type type)
{
	struct siwar_event ev = { .type = type };

	return state_post(&ev);
}

enum siwar_state state_get(void)
{
	return cur;
}

static void state_thread(void)
{
	struct siwar_event ev;

	while (true) {
		k_msgq_get(&ev_q, &ev, K_FOREVER);
		handle(&ev);
	}
}

K_THREAD_DEFINE(state_tid, 2048, state_thread, NULL, NULL, NULL, 5, 0, 0);

void state_init(void)
{
	enter(ST_IDLE);
}
