/*
 * السوار الوطني - واجهة السوار (الضوء، الاهتزاز، الصوت، الشاشة، الأزرار)
 *
 * على لوحة التطوير Thingy:91 X: الضوء = الإضاءة الملونة، والاهتزاز والشاشة
 * تُعرض في السجل فقط لأنها غير موجودة في اللوحة.
 * على السوار الحقيقي: تُستبدل الدوال نفسها بمحرك الاهتزاز DRV2605L،
 * والطنّان، وشاشة AMOLED، دون تغيير باقي النظام.
 */
#ifndef SIWAR_UI_H
#define SIWAR_UI_H

enum ui_light {
	UI_LIGHT_OFF,
	UI_LIGHT_AMBER,
	UI_LIGHT_GREEN,
};

int ui_init(void);

void ui_light(enum ui_light color);

void ui_vibrate_alert(void);  /* اهتزاز متكرر للإنذار */
void ui_vibrate_short(void);  /* هزة خفيفة واحدة */
void ui_vibrate_stop(void);

void ui_sound_alert(void);

void ui_screen_off(void);
void ui_screen_status(void);           /* البطارية وحالة الاتصال */
void ui_screen_text(const char *text);
void ui_family_safe(const char *name); /* «فلان بأمان ✓» */

int ui_battery_percent(void);

#endif
