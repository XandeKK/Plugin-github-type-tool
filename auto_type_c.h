#ifndef __AUTO_TYPE_H__
#define __AUTO_TYPE_H__

#include <libgimp/gimp.h>

struct Setting {
	gchar style_file[256];
	gchar default_fontname[100];
	gdouble font_size;
	gchar target[256];
	int position;
};

struct Text {
		int length;
		char *str_modified[50];
		char str_joined[256];
};

#endif /* __AUTO_TYPE_H__ */