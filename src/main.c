/*
 * السوار الوطني - نظام السوار
 * فكرة وابتكار: إسماعيل النجار الأنصاري. جميع الحقوق محفوظة.
 *
 * ترتيب التشغيل:
 *   ١. الواجهة (الأزرار والضوء)
 *   ٢. شريحة الاتصال والاتصال بالشبكة
 *   ٣. قائمة الأهل والرسائل
 *   ٤. استقبال الإنذار الوطني
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>

#include "state.h"
#include "ui.h"
#include "alert.h"
#include "family.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static void lte_handler(const struct lte_lc_evt *const evt)
{
	if (evt->type == LTE_LC_EVT_NW_REG_STATUS) {
		bool on = evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ||
			  evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING;

		LOG_INF("Network: %s", on ? "connected" : "searching");
	}
}

int main(void)
{
	LOG_INF("Siwar national alert band starting");

	if (ui_init()) {
		LOG_ERR("UI init failed");
	}
	state_init();

	int err = nrf_modem_lib_init();

	if (err) {
		LOG_ERR("Modem init failed (%d)", err);
		return 0;
	}

	err = lte_lc_connect_async(lte_handler);
	if (err) {
		LOG_ERR("LTE connect failed (%d)", err);
	}

	family_init();
	alert_init();

	LOG_INF("Ready. Hold both buttons 3 s for a test alert.");
	return 0;
}
