/*
 * السوار الوطني - استقبال الإنذار الوطني
 *
 * الفكرة: نطلب من شريحة الاتصال أن تستقبل رسائل البث الخلوي على قنوات
 * الإنذار العام (AT+CSCB)، ثم نراقب الإشعار القياسي +CBM الذي يصل عند
 * وصول رسالة بث (المعيار 3GPP TS 27.005).
 *
 * ⚠️ أهم شيء نتحقق منه أول ما يصل الجهاز:
 *   هل شريحة nRF9151 تدعم AT+CSCB وتُرسل +CBM؟
 *   إن لم تدعمه، نغيّر الشريحة كما في ملف المواصفات (البند الإلزامي).
 *
 * TODO: التفريق بين «إنذار» و«زوال الخطر» يعتمد على أرقام الرسائل
 * والمحتوى المستخدم في منظومة الإنذار الوطنية، ويُضبط بعد التنسيق مع الجهات.
 */
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/at_monitor.h>
#include <nrf_modem_at.h>

#include "alert.h"
#include "state.h"

LOG_MODULE_REGISTER(alert, LOG_LEVEL_INF);

static void cbs_handler(const char *notif);

AT_MONITOR(cbs_monitor, "+CBM", cbs_handler);

static void cbs_handler(const char *notif)
{
	LOG_WRN("Cell Broadcast received: %s", notif);

	/* TODO: قراءة رقم الرسالة من الإشعار وتحديد نوعها.
	 * مبدئيًا: أي رسالة على قنوات الإنذار = إنذار.
	 */
	state_post_simple(EV_ALERT);
}

int alert_init(void)
{
	/* تفعيل استقبال قنوات الإنذار */
	int err = nrf_modem_at_printf("AT+CSCB=0,\"%s\"", CONFIG_SIWAR_CBS_CHANNELS);

	if (err) {
		LOG_ERR("Modem refused AT+CSCB (%d). Cell Broadcast may not be supported.", err);
		LOG_ERR("Use the two-button test (hold 3 s) until this is resolved.");
		return err;
	}
	LOG_INF("Listening for national alerts on channels %s", CONFIG_SIWAR_CBS_CHANNELS);
	return 0;
}
