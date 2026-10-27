/*
 * السوار الوطني - قائمة الأهل ورسائل «أنا بأمان»
 */
#ifndef SIWAR_FAMILY_H
#define SIWAR_FAMILY_H

int family_init(void);

/* إدارة القائمة (ستُستدعى من تطبيق الجوال عبر البلوتوث لاحقًا) */
int family_add(const char *name, const char *phone);
int family_remove(const char *phone);
int family_count(void);

/* يرسل «أنا بأمان» لكل الأهل */
void family_broadcast_safe(void);

#endif
