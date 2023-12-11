#ifndef _CFMS_APMIB_H_
#define _CFMS_APMIB_H_

int cfms_apmib_init(void);
int cfms_apmib_set(char *key, char *value);
int cfms_apmib_get(char *key, char *value, int len);
void cfms_apmib_show(void);
int cfms_apmib_save2flash(void);
int cfms_apmib_default(void);
int cfms_apmib_convert_mib(void);
int cfms_apmib_unset(char *key);
void cfms_apmib_loadfile(char *file);

#endif
