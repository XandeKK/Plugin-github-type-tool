#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

#include <libgimp/gimp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <math.h>
#include "trim.c"
#include "type_tool.h"

static  void run (const gchar *name, gint nparams, const GimpParam *param, gint *nreturn_vals, GimpParam **return_vals);
static void query (void);
int set_text_new(gint image, gchar *text);
const gchar *get_text_from_file(gchar *filename, int position);
void get_setting();
void get_style();
void update_position();
void save_setting();
void slice(const gchar * str, gchar * buffer, size_t start, size_t end);
int is_space(gchar *text);
int only_tag(const gchar *tag);
const gchar *get_biggest_word(const gchar *text);

char homedir[256];

GimpPlugInInfo PLUG_IN_INFO = {
	NULL,
	NULL,
	query,
	run
};

struct Text current_text;

struct Setting setting = {
	"",
	"",
	31,
	"",
	0,
};

struct Font current_font = {
	"",
	"",
	1,
};

struct Fonts fonts;

static void query (void)
	{

  static GimpParamDef args[] = {
    {
      GIMP_PDB_INT32,
      "run-mode",
      "Run mode"
    },
    {
      GIMP_PDB_IMAGE,
      "image",
      "Input image"
    },
    {
      GIMP_PDB_DRAWABLE,
      "drawable",
      "Input drawable"
    }
  };

  gimp_install_procedure (
    "plug-in-type-tool",
    "Auto type!",
    "Is a Auto type",
    "Alexandre dos Santos Alves",
    "Copyright Alexandre",
    "2023",
    "_Type Tool",
    "RGB*, GRAY*",
    GIMP_PLUGIN,
    G_N_ELEMENTS (args), 0,
    args, NULL);
  gimp_plugin_menu_register ("plug-in-type-tool",
                               "<Image>/Tools/"); 
}

static void run (
	const gchar      *name,
  gint              nparams,
  const GimpParam  *param,
  gint             *nreturn_vals,
  GimpParam       **return_vals)
	{

	static GimpParam  values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	GimpRunMode       run_mode;
	GimpDrawable     *drawable;

	*nreturn_vals = 1;
	*return_vals  = values;

	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = status;

	char *homedir_tmp;

	if ((homedir_tmp = getenv("HOME")) == NULL) {
    homedir_tmp = getpwuid(getuid())->pw_dir;
	}

	strcpy(homedir, homedir_tmp);
	strcat(homedir, "/.gimp_setting_type_tool");

	gint is_to_ignore = 0;
	gint image = param[1].data.d_image;

	gimp_debug_timer_start();

	while(!is_to_ignore) {
		get_setting();
		get_style();
		const gchar *text = get_text_from_file(setting.target, setting.position);
		is_to_ignore = set_text_new(image, text);
		update_position();	
		save_setting();
	}

	gimp_displays_flush ();
	gimp_debug_timer_end();
}

int set_text_new(gint image, gchar *text) {
	gchar text_copy[strlen(text)];
	gchar *text_trim, *text_splited;
	gchar *fontname = setting.default_fontname;
	gchar delim[] = " ";

	gboolean non_empty;
	gboolean okay = FALSE;

	gdouble font_size = setting.font_size;
	gdouble position[2];

	gint x1, y1, x2, y2;
	gint width_selection, height_selection, width_limit;
	gint width_line = 0, max_width_text = 0, max_height_text = 0;
	gint text_layer;

	gimp_selection_bounds(image, &non_empty, &x1, &y1, &x2, &y2);

	if (strlen(text) < 1) {
		gimp_message("Need a text!");
  	gimp_quit();
	}

	if (!non_empty) {
		gimp_message("Nothing is selected!");
		gimp_quit();
	}

	width_selection = x2 - x1;
	height_selection = y2 - y1;
	width_limit = width_selection * 0.78;

	const gchar *biggest_word = get_biggest_word(text);

	gint width, height, ascent, descent;
	gimp_text_get_extents_fontname(biggest_word, font_size, GIMP_PIXELS, current_font.fontname, &width, &height, &ascent, &descent);

	font_size = floor((font_size * width_limit) / width);

	if (font_size > setting.font_size) {
		font_size = setting.font_size;
	}

	text_trim = trim(text);
	text_splited = strtok(text_trim, delim);

	while (text_splited != NULL) {
		int is_tag = FALSE;
		if (only_tag(text_splited)) {
		  for (int i = 0; i < fonts.length; ++i) {
		  	if (strcmp(fonts.fonts[i].tag, text_splited) == 0) {
		  		if (strcmp(fonts.fonts[i].fontname, "Black") == 0) {
		  			current_font.black = 1;
		  		} else if (strcmp(fonts.fonts[i].fontname, "White") == 0) {
		  			current_font.black = 0;
		  		} else if (strcmp(fonts.fonts[i].fontname, "Ignore") == 0) {
		  			return 0;
		  		}
		  		else {
		  			strcpy(current_font.fontname, fonts.fonts[i].fontname);
		  		}
		  		is_tag = TRUE;
		  		break;
		  	}
		  }
		  if (is_tag) {
		  	text_splited = strtok(NULL, delim);
		  	continue;
		  }
		}

		strcat(current_text.str, text_splited);
		strcat(current_text.str, " ");

		gint width, height, ascent, descent;
		gimp_text_get_extents_fontname(text_splited, font_size, GIMP_PIXELS, current_font.fontname, &width, &height, &ascent, &descent);

		if ((width_line + width) > width_limit) {
			max_height_text += height + ascent + descent;
			width_line = 0;
		}

		width_line += width;

		if (width_line > max_width_text) {
			max_width_text = width_line;
		}

		text_splited = strtok(NULL, delim);
	}

	slice(current_text.str, current_text.str, 0, strlen(current_text.str) - 2);

	if (max_height_text == 0) {
		gint width, height, ascent, descent;
		gimp_text_get_extents_fontname(current_text.str, font_size, GIMP_PIXELS, current_font.fontname, &width, &height, &ascent, &descent);
		max_height_text += height + ascent + descent;
	}

	position[0] = (gdouble) x2 - width_selection * 0.5 - max_width_text * 0.5;
	position[1] = (gdouble) y2 - height_selection * 0.5 - max_height_text * 0.5;

	if (position[1] < y1) {
		position[1] = (gdouble) y1 * 1.15;
	}

	text_layer = gimp_text_fontname(image, -1, position[0], position[1], current_text.str, 0, TRUE, font_size, GIMP_PIXELS, current_font.fontname);

	gimp_text_layer_set_justification(text_layer, 2);
	gimp_text_layer_resize(text_layer, max_width_text, max_height_text + 100);
	if (current_font.black) {
		GimpRGB color = { 0, 0, 0, 1 };
		gimp_text_layer_set_color(text_layer, &color);
	} else {
		GimpRGB color = { 255, 255, 255, 1 };
		gimp_text_layer_set_color(text_layer, &color);
	}

	return 1;
}

const gchar *get_text_from_file(char *filename, int position) {
	FILE* file_ptr;
  static gchar str[256];
  gchar *output;
  gint line_count = 0;
  file_ptr = fopen(filename, "r");
  
  if (NULL == file_ptr) {
  	char error[100];
  	snprintf(error, sizeof error, "unable to access file - %s (%d)", __FILE__ ,__LINE__);
	  gimp_message(error);
  	gimp_quit();
  }

  while (fgets(str, 256, file_ptr) != NULL) {
  	line_count++;
  	if (line_count >= position) {
  		if (!is_space(str)) {
  			output = str;
  			break;
  		}
  	}
  }

  fclose(file_ptr);
  return output;
}

int is_space(char *text) {
	regex_t reegex;
	int value;
	value = regcomp( &reegex, "^[ \t\n\r\f\v]*$", 0 );

	value = regexec( &reegex, text,0, NULL, 0 );

	if (value == 0) {
		return 1; // Found
	}
	return 0; // Error
}

void get_setting() {
	FILE* file_ptr;
  gchar str[256], *setting_str;
  gchar delim[] = "|";
  gint count = 0;

  file_ptr = fopen(homedir, "r");

  if (NULL == file_ptr) {
  	char error[100];
  	snprintf(error, sizeof error, "unable to access file - %s (%d)", __FILE__ ,__LINE__);
	  gimp_message(error);
  	gimp_quit();
  }

  fgets(str, 256, file_ptr);

  setting_str = strtok(str, delim);

	while (setting_str != NULL) {

		switch (count) {
		case 0:
			strcpy(setting.default_fontname, setting_str);
			strcpy(current_font.fontname, setting_str);
			break;
		case 1:
			setting.font_size = atoi(setting_str);
			break;
		case 2:
			strcpy(setting.target, setting_str);
			break;
		case 3:
			setting.position = atoi(setting_str);
			break;
		case 4:
			strcpy(setting.style_file, setting_str);
			break;
		}

		count++;
  	setting_str = strtok(NULL, delim);
	}

  fclose(file_ptr);
}

void get_style() {
	FILE* file_ptr;
  gchar str[100];
  file_ptr = fopen(setting.style_file, "r");

  if (NULL == file_ptr) {
    gchar error[100];
  	snprintf(error, sizeof error, "unable to access file - %s (%d)", __FILE__ ,__LINE__);
	  gimp_message(error);
  	gimp_quit();
  }

  fonts.length = 0;

  while (fgets(str, 100, file_ptr) != NULL) {
  	char *text_trim = trim(str);
  	slice(text_trim, fonts.fonts[fonts.length].tag, 0, 1);
  	slice(text_trim, fonts.fonts[fonts.length].fontname, 3, strlen(text_trim));
  	fonts.length++;
  }

  fclose(file_ptr);
}

void update_position() {
  regex_t reegex;
	int value;
	FILE* file_ptr;
  gchar str[256];
  gint line_count = 0;
  file_ptr = fopen(setting.target, "r");

  for (int i = 0; i < fonts.length; ++i) {
  	if (strcmp(fonts.fonts[i].fontname, "Ignore") == 0) {
			regcomp( &reegex, fonts.fonts[i].tag, 0 );
  	}
  }
  
  if (NULL == file_ptr) {
  	char error[100];
  	snprintf(error, sizeof error, "unable to access file - %s (%d)", __FILE__ ,__LINE__);
	  gimp_message(error);
  	gimp_quit();
  }

  while (fgets(str, 256, file_ptr) != NULL) {
  	line_count++;
  	if (line_count > setting.position) {
  		if (!is_space(str) && regexec( &reegex, str, 0, NULL, 0 )) {
  			setting.position = line_count;
  			break;
  		}
  	}
  }

  fclose(file_ptr);
}

void save_setting() {
	FILE* file_setting;
	file_setting = fopen (homedir, "w");
  
  if (NULL == file_setting) {
  	char error[100];
  	snprintf(error, sizeof error, "unable to access file - %s (%d)", __FILE__ ,__LINE__);
	  gimp_message(error);
  	gimp_quit();
  }

	fprintf(file_setting, "%s|%f|%s|%d|%s", setting.default_fontname, setting.font_size, setting.target, setting.position, setting.style_file);

	fclose(file_setting);
}

void slice(const gchar * str, gchar * buffer, size_t start, size_t end) {
    size_t j = 0;
    for ( size_t i = start; i <= end; ++i ) {
        buffer[j++] = str[i];
    }
    buffer[j] = 0;
}

int only_tag(const gchar *tag) {
	regex_t reegex;
	int value;
	value = regcomp( &reegex, "[^A-Za-z0-9 ]", 0 );

	value = regexec( &reegex, tag, 0, NULL, 0 );

	if (value == 0) {
		return 1; // Found
	}
	return 0; // Error
}

const gchar *get_biggest_word(const gchar *text) {
	gint size = strlen(text);
	gint i, len, max = -1;
	static gchar longestWord[50];

	for(i = 0; i <= size; i++) {
		if(text[i] == ' ' || text[i] == '\0') {
      if(len > max) {
				max = len;
        strncpy(longestWord, &text[i - len], len);
      }
      len = 0;
    } else {
			len++;
    }
  }

	return longestWord;
}

MAIN()
