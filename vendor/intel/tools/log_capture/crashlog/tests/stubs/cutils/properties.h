#ifndef __CUTILS_PROPERTIES_H__
#define __CUTILS_PROPERTIES_H__

#define DEFAULT_PROPS       "res/properties.txt"

#define MAX_PROPERTIES      50
#define PROPERTY_KEY_MAX    32
#define PROPERTY_VALUE_MAX  92

int property_get(char *name, char *value, char *def);
int property_set(char *name, char *value);
int reset_property_cache();

#endif /* __CUTILS_PROPERTIES_H__ */
