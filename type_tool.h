#ifndef __TYPE_TOOL_H__
#define __TYPE_TOOL_H__

#include <libgimp/gimp.h>

struct Setting {
	gchar style_file[256];
	gchar default_fontname[100];
	gdouble font_size;
	gchar target[256];
	gint position;
};

struct Text {
	gchar str[256];
};

struct Font {
	gchar fontname[100];
	gchar tag[2];
	gint black;
};

struct Fonts {
	gint length;
	struct Font fonts[50];
};

#endif /* __TYPE_TOOL_H__ */