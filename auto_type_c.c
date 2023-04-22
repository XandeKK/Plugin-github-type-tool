#ifndef FALSE
#define FALSE   (0)
#endif


#ifndef TRUE
#define TRUE    (!FALSE)
#endif

#include <libgimp/gimp.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "trim.c"

static void init (void);

static  void run (const gchar      *name,
        gint              nparams,
        const GimpParam  *param,
        gint             *nreturn_vals,
        GimpParam       **return_vals);

static void query (void);

GimpPlugInInfo PLUG_IN_INFO = {
	init,
	NULL,
	query,
	run
};


static void
	query (void)
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
    },
    {
      GIMP_PDB_STRING,
      "text",
      "Text"
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
}


static void
	run (const gchar      *name,
	   gint              nparams,
	   const GimpParam  *param,
	   gint             *nreturn_vals,
	   GimpParam       **return_vals)
	{

	static GimpParam  values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	GimpRunMode       run_mode;

	/* Setting mandatory output values */
	*nreturn_vals = 1;
	*return_vals  = values;

	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = status;

	gchar *text = param[3].data.d_string;
	gchar text_copy[strlen(text)];
	gchar *text_trim, *text_splited;
	gchar fontname[] = "Wild Words Thin";
	gchar delim[] = " ";

	gboolean non_empty;
	gboolean okay = FALSE;

	gdouble font_size = 31; // get of settings
	gdouble position[2];

	gint image = param[1].data.d_image;
	gint x1, y1, x2, y2;
	gint width_selection, height_selection, width_limit;
	gint width_line = 0, max_width_text = 0, max_height_text = 0;
	gint text_layer;

	gimp_selection_bounds(image, &non_empty, &x1, &y1, &x2, &y2);

	if (strlen(text) < 1) {
		gimp_message("Need a text!");
		return;
	}

	if (!non_empty) {
		gimp_message("Nothing is selected!");
		return;
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

	text_layer = gimp_text_fontname(image, -1, position[0], position[1], text, 0, TRUE, font_size, GIMP_PIXELS, "Wild Words Thin");

	gimp_text_layer_set_justification(text_layer, 2);
	gimp_text_layer_resize(text_layer, max_width_text, max_height_text + 50);

}


// static void init (void) {
// 	char cwd[PATH_MAX];
// 	char setting[] = "/.setting_type_tool_gimp";
// 	FILE *file_read, *file_write;

// 	strcat(cwd,setting);

//    if (getcwd(cwd, sizeof(cwd)) != NULL) {
// 			file_read = fopen(cwd, "r");
// 			if(file_read != NULL) {
// 				fclose(file_read);
// 			} else {
// 				file_write = fopen(cwd, "w");
// 			}
//    }
// }

MAIN()