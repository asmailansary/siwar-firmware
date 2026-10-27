/*
 * السوار الوطني - آلة الحالات
 * كل ما يحدث في السوار يصل هنا كـ "حدث"، وهذا الملف يقرر ماذا يفعل السوار.
 */
#ifndef SIWAR_STATE_H
#define SIWAR_STATE_H

#include <stdint.h>

enum siwar_state {
	ST_IDLE,         /* الشاشة مطفأة، لا يوجد إنذار */
	ST_AWAKE,        /* الشاشة مضاءة بعد ضغطة زر، تنطفئ بعد ١٠ ثوانٍ */
	ST_ALERT,        /* إنذار: اهتزاز + برتقالي */
	ST_ALERT_MUTED,  /* إنذار بعد إغلاق التنبيه: برتقالي بدون اهتزاز */
	ST_SAFE,         /* ضغط «أنا بأمان»، ينتظر زوال الخطر */
	ST_CLEAR,        /* زال الخطر: أخضر ٣ دقائق ثم ينطفئ */
};

enum siwar_event_type {
	EV_BTN_SAFE,          /* الزر الأيمن الأخضر */
	EV_BTN_DISMISS,       /* الزر الأيسر */
	EV_ALERT,             /* وصل إنذار وطني */
	EV_ALL_CLEAR,         /* وصل إشعار زوال الخطر */
	EV_SCREEN_TIMEOUT,    /* مرت ١٠ ثوانٍ بدون ضغط */
	EV_CLEAR_TIMEOUT,     /* مرت ٣ دقائق على الأخضر */
	EV_FAMILY_SAFE,       /* وصلت «أنا بأمان» من أحد الأهل */
};

struct siwar_event {
	enum siwar_event_type type;
	char name[24];        /* اسم الشخص في EV_FAMILY_SAFE */
};

void state_init(void);
int state_post(const struct siwar_event *ev);
int state_post_simple(enum siwar_event_type type);
enum siwar_state state_get(void);

#endif
