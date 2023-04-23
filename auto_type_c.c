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
#include "trim.c"
#include "auto_type_c.h"

static  void run (const gchar *name, gint nparams, const GimpParam *param, gint *nreturn_vals, GimpParam **return_vals);
static void query (void);
void set_text(gint *image, const gchar *text);
const gchar *get_text_from_file(gchar *filename, int position);
int is_space(gchar *text);
void get_setting();
void update_position();
void save_setting();


GimpPlugInInfo PLUG_IN_INFO = {
	NULL,
	NULL,
	query,
	run
};

struct Setting setting = {
	"",
	"",
	31,
	"",
	0,
};

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
    "plug-in-auto-type",
    "Auto type!",
    "Is a Auto type",
    "Alexandre dos Santos Alves",
    "Copyright Alexandre",
    "2023",
    "_Auto type...",
    "RGB*, GRAY*",
    GIMP_PLUGIN,
    G_N_ELEMENTS (args), 0,
    args, NULL);
  gimp_plugin_menu_register ("plug-in-auto-type",
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

	// Setting mandatory output values
	*nreturn_vals = 1;
	*return_vals  = values;

	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = status;

	gint image = param[1].data.d_image;
	drawable = gimp_drawable_get (param[2].data.d_drawable);

	get_setting();
	const gchar *text = get_text_from_file(setting.target, setting.position);
	set_text(&image, text);
	update_position();
	save_setting();

	gimp_displays_flush ();
	gimp_drawable_detach (drawable);
}

void set_text(gint *image, const gchar *text) {
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

	gimp_selection_bounds(*image, &non_empty, &x1, &y1, &x2, &y2);

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
	width_limit = width_selection * 0.8;

	while (!okay) {
		strcpy(text_copy, text);
		text_trim = trim(text_copy);
		text_splited = strtok(text_trim, delim);

		while (text_splited != NULL) {
			gint width, height, ascent, descent;
			gimp_text_get_extents_fontname(text_splited, font_size, GIMP_PIXELS, fontname, &width, &height, &ascent, &descent);

			if (width >= width_limit) {
				font_size -= 1;
				width_line = 0;
				max_height_text = 0;
				okay = FALSE;
				break;
			}

			if ((width_line + width) > width_limit) {
				max_height_text += height + ascent + descent;
				width_line = 0;
			}

			width_line += width;
			okay = TRUE;

			if (width_line > max_width_text) {
				max_width_text = width_line;
			}

			text_splited = strtok(NULL, delim);
		}
	}

	if (max_height_text == 0) {
		gint width, height, ascent, descent;
		gimp_text_get_extents_fontname(text, font_size, GIMP_PIXELS, fontname, &width, &height, &ascent, &descent);
		max_height_text += height + ascent + descent;
	}

	position[0] = (gdouble) x2 - width_selection * 0.5 - max_width_text * 0.5;
	position[1] = (gdouble) y2 - height_selection * 0.5 - max_height_text * 0.5;

	text_layer = gimp_text_fontname(*image, -1, position[0], position[1], text, 0, TRUE, font_size, GIMP_PIXELS, fontname);

	gimp_text_layer_set_justification(text_layer, 2);
	gimp_text_layer_resize(text_layer, max_width_text, max_height_text + 100);
}

const char *get_text_from_file(char *filename, int position) {
	FILE* file_ptr;
  static char str[256];
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
	gchar cwd[PATH_MAX];
	FILE* file_ptr;
  gchar str[256], *setting_str;
  gchar dir[] = "/.gimp_setting_type_tool";
  gchar delim[] = "|";
  gint count = 0;

	if (getcwd(cwd, sizeof(cwd)) == NULL) {
	  char error[100];
  	snprintf(error, sizeof error, "unable to access file - %s (%d)", __FILE__ ,__LINE__);
	  gimp_message(error);
		gimp_quit();
	}

  strcat(cwd, dir);
  file_ptr = fopen(cwd, "r");

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
			strcpy(setting.default_fontname,setting_str);
			break;
		case 1:
			setting.font_size = atoi(setting_str);
			break;
		case 2:
			strcpy(setting.target,setting_str);
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

void update_position() {
	FILE* file_ptr;
  char str[256];
  gint line_count = 0;
  file_ptr = fopen(setting.target, "r");
  
  if (NULL == file_ptr) {
  	char error[100];
  	snprintf(error, sizeof error, "unable to access file - %s (%d)", __FILE__ ,__LINE__);
	  gimp_message(error);
  	gimp_quit();
  }

  while (fgets(str, 256, file_ptr) != NULL) {
  	line_count++;
  	if (line_count > setting.position) {
  		if (!is_space(str)) {
  			setting.position = line_count;
  			break;
  		}
  	}
  }

  fclose(file_ptr);
}

void save_setting() {
	gchar cwd[PATH_MAX];
	FILE* file_setting;
  gchar dir[] = "/.gimp_setting_type_tool";

  if (getcwd(cwd, sizeof(cwd)) == NULL) {
  	char error[100];
  	snprintf(error, sizeof error, "unable to access file - %s (%d)", __FILE__ ,__LINE__);
	  gimp_message(error);
		gimp_quit();
	}

	strcat(cwd, dir);
	file_setting = fopen (cwd, "w");

	fprintf(file_setting, "%s|%f|%s|%d|%s", setting.default_fontname, setting.font_size, setting.target, setting.position, setting.style_file);

	fclose(file_setting);
}

MAIN()