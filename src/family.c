/*
 * السوار الوطني - قائمة الأهل ورسائل «أنا بأمان»
 *
 * - القائمة محفوظة داخل السوار (تبقى بعد إطفائه).
 * - «أنا بأمان» تُرسل كرسالة قصيرة مخفية بين الأساور عبر شبكة الجوال،
 *   بدون إنترنت.
 * - عند استقبال رسالة من رقم موجود في القائمة، يظهر اسمه على الشاشة.
 *
 * صيغة الرسالة الحالية: "SW1 SAFE"
 * TODO(أمان): إضافة توقيع HMAC بمفتاح العائلة + عدّاد ضد إعادة الإرسال،
 * حتى لا يستطيع أحد تزوير «أنا بأمان».
 */
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <modem/sms.h>

#include "family.h"
#include "state.h"

LOG_MODULE_REGISTER(family, LOG_LEVEL_INF);

#define SAFE_MSG "SW1 SAFE"
#define NAME_LEN 24
#define PHONE_LEN 16

struct member {
	char name[NAME_LEN];
	char phone[PHONE_LEN];
};

static struct {
	uint8_t count;
	struct member m[CONFIG_SIWAR_MAX_FAMILY];
} fam;

/* ---------------- الحفظ داخل السوار ---------------- */

static int fam_set(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	if (strcmp(key, "list") == 0 && len == sizeof(fam)) {
		ssize_t r = read_cb(cb_arg, &fam, sizeof(fam));

		return r < 0 ? (int)r : 0;
	}
	return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(siwar, "siwar", NULL, fam_set, NULL, NULL);

static int fam_save(void)
{
	return settings_save_one("siwar/list", &fam, sizeof(fam));
}

/* آخر ٩ أرقام تكفي للمطابقة بين 05XXXXXXXX و +9665XXXXXXXX */
static const char *tail9(const char *p)
{
	size_t n = strlen(p);

	return n > 9 ? p + n - 9 : p;
}

static int find(const char *phone)
{
	for (int i = 0; i < fam.count; i++) {
		if (strcmp(tail9(fam.m[i].phone), tail9(phone)) == 0) {
			return i;
		}
	}
	return -1;
}

int family_add(const char *name, const char *phone)
{
	if (find(phone) >= 0) {
		return -EEXIST;
	}
	if (fam.count >= CONFIG_SIWAR_MAX_FAMILY) {
		return -ENOMEM;
	}
	struct member *m = &fam.m[fam.count++];

	strncpy(m->name, name, NAME_LEN - 1);
	m->name[NAME_LEN - 1] = '\0';
	strncpy(m->phone, phone, PHONE_LEN - 1);
	m->phone[PHONE_LEN - 1] = '\0';
	LOG_INF("Family member added: %s", m->name);
	return fam_save();
}

int family_remove(const char *phone)
{
	int i = find(phone);

	if (i < 0) {
		return -ENOENT;
	}
	memmove(&fam.m[i], &fam.m[i + 1], (fam.count - i - 1) * sizeof(struct member));
	fam.count--;
	return fam_save();
}

int family_count(void)
{
	return fam.count;
}

/* ---------------- إرسال «أنا بأمان» ---------------- */

void family_broadcast_safe(void)
{
	if (fam.count == 0) {
		LOG_WRN("No family members to notify");
		return;
	}
	for (int i = 0; i < fam.count; i++) {
		int err = sms_send_text(fam.m[i].phone, SAFE_MSG);

		if (err) {
			/* TODO: إعادة المحاولة عند رجوع التغطية */
			LOG_ERR("Sending to %s failed (%d)", fam.m[i].name, err);
		} else {
			LOG_INF("'I am safe' sent to %s", fam.m[i].name);
		}
	}
}

/* ---------------- استقبال «أنا بأمان» من الأهل ---------------- */

static void sms_cb(struct sms_data *const data, void *context)
{
	if (data->type != SMS_TYPE_DELIVER || data->payload == NULL) {
		return;
	}
	if (strncmp(data->payload, SAFE_MSG, strlen(SAFE_MSG)) != 0) {
		return; /* رسالة عادية، ليست من نظام السوار */
	}
	const char *from = data->header.deliver.originating_address.address_str;
	int i = find(from);

	if (i < 0) {
		LOG_WRN("'I am safe' from unknown number ignored");
		return;
	}
	struct siwar_event ev = { .type = EV_FAMILY_SAFE };

	strncpy(ev.name, fam.m[i].name, sizeof(ev.name) - 1);
	state_post(&ev);
}

int family_init(void)
{
	int err = settings_subsys_init();

	if (err) {
		LOG_ERR("Settings init failed (%d)", err);
		return err;
	}
	settings_load_subtree("siwar");

	if (fam.count == 0 && strlen(CONFIG_SIWAR_DEV_FAMILY_PHONE) > 0) {
		family_add(CONFIG_SIWAR_DEV_FAMILY_NAME, CONFIG_SIWAR_DEV_FAMILY_PHONE);
	}
	LOG_INF("Family members stored: %d", fam.count);

	int handle = sms_register_listener(sms_cb, NULL);

	if (handle < 0) {
		LOG_ERR("SMS listener failed (%d)", handle);
		return handle;
	}
	return 0;
}
