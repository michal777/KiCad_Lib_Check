/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tool checking some of KiCad Library Convention rules in footprints (pretty format)
// Author: Michał Stec
//
// Tests and fixes:
// wrong module name in file header (not equal to footprint name), footprint locked (shouldn't be locked in library)
// commas in tags (should be spaces)
// reference wrong or on wrong layer (should be **REF on silkscreen)
// value wrong or on wrong layer (should be footprint name on assembly)
// font size (should be 1 x 1 mm, line 0.15 mm)
// wrong courtyard or silkscreen line width (should be 0.05 mm and 0.15 mm)
// settings that are usually not necessary in official library (autoplace cost, solder paste settings, clearance at footprint level)
// not standard 3d link (should be *.3dshapes/*.wrl), moves files to standard location in packages3d/
//
// Additional tests:
// information if reference is on bottom layer (usually all are on top)
// information if value is on bottom layer (usually all are on top)
// information if silkscreen or courtyard line is on bottom layer (usually all are on top)
// warning courtyard is not rounded to 0.05 mm (must be rounded)
// information if most of pads are tht and attribute is normal+insert or pads are smd and attribute is normal
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_LIB_NR 500
#define BUFFER_SIZE 2000

#if defined(__MINGW32__)
	#include <windows.h>
#endif

#if defined(__MINGW32__)
	#define printf_RED(...) {SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), FOREGROUND_RED); printf (__VA_ARGS__);}
	#define printf_YELLOW(...) {SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN); printf (__VA_ARGS__);}
	#define printf_CYAN(...) {SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE); printf (__VA_ARGS__);}
	#define printf_GREEN(...) {SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), FOREGROUND_GREEN); printf (__VA_ARGS__);}
	#define scanf_RED(...) {SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), FOREGROUND_RED); scanf (__VA_ARGS__);}
	#define scanf_YELLOW(...) {SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN); scanf (__VA_ARGS__);}
	#define scanf_CYAN(...) {SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE); scanf (__VA_ARGS__);}
	#define scanf_GREEN(...) {SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), FOREGROUND_GREEN); scanf (__VA_ARGS__);}
#else
	#define printf_RED(...) {printf("\033[22;31m"); printf (__VA_ARGS__);}
	#define printf_YELLOW(...) {printf("\033[01;33m"); printf (__VA_ARGS__);}
	#define printf_CYAN(...) {printf("\033[22;36m"); printf (__VA_ARGS__);}
	#define printf_GREEN(...) {printf("\033[22;32m"); printf (__VA_ARGS__);}
	#define scanf_RED(...) {printf("\033[22;31m"); scanf (__VA_ARGS__);}
	#define scanf_YELLOW(...) {printf("\033[01;33m"); scanf (__VA_ARGS__);}
	#define scanf_CYAN(...) {printf("\033[22;36m"); scanf (__VA_ARGS__);}
	#define scanf_GREEN(...) {printf("\033[22;32m"); scanf (__VA_ARGS__);}
#endif



// returns nonzero if wrong module name or footprint locked
int check_name_locked (char *buffer_in, char *footprint_name)
{
	int n;
	char read_module_name[BUFFER_SIZE];

	for (n = 0; buffer_in[n] != '\n'; ++n)		// check if footprint is locked (search " locked " in first line)
		if (!strncmp (buffer_in + n, " locked ", strlen (" locked ")))
			return 1;

	for (n = 0; (buffer_in[strlen("(module ") + n]) != ' '; ++n)		// read module name from first line to check if the same as footprint name
		read_module_name[n] = buffer_in[strlen("(module ") + n];
	read_module_name[n] = '\0';
	return strcmp (read_module_name, footprint_name);
}

// sets proper module name and removes "locked" property (if was present)
int fix_name_locked (char *buffer_in, char *footprint_name, char *buffer_out, int *modified)
{
	int n;
	int m;
	char rest_of_line[BUFFER_SIZE];

	for(n = 0; strncmp (buffer_in + n, " (layer ", strlen(" (layer ")) ; ++n)		// find in first line beginning of " (layer "
		{;}
	for(m = 0; (buffer_in[n + m - 1]) != '\n'; ++m)		// save rest of line - following "(module <name> [locked] "
		rest_of_line[m] = buffer_in[n + m];
	rest_of_line[m] = '\0';

	sprintf (buffer_out, "(module %s%s", footprint_name, rest_of_line);		// assembly first line: take proper name, ignore " locked ", copy layer and time-stamp

	printf_YELLOW ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
	if (*modified >= 0)		// increment value of modified only if there were no errors (modified == -1)
		*modified += 1;
	return 0;
}

// returns nonzero if comma in tags found
int check_tags (char *buffer_in)
{
	int n;

	for (n = 0; buffer_in[n] != '\n'; ++n)		// search comma
		if (buffer_in[n] == ',')
			return 1;
	return 0;
}

// removes comma from tags
int fix_tags (char *buffer_in, char *buffer_out, int *modified)
{
	int n;
	int m;

	for (n = 0, m = 0; buffer_in[n - 1] != '\n'; ++n, ++m)		// search and replace comma
	{
		if ((buffer_in[n] == ',') && (buffer_in[n + 1] != ' '))
			buffer_out[m] = ' ';
		else if ((buffer_in[n] == ',') && (buffer_in[n + 1] == ' '))
			--m;
		else
			buffer_out[m] = buffer_in[n];
	}
	buffer_out[m] = '\0';

	printf_YELLOW ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

// returns nonzero if reference is not "REF**" or is on wrong layer or is hidden
int check_ref (char *buffer_in, char *pretty_name, char *footprint_name)
{
	int n;
	char read_ref[BUFFER_SIZE];
	int layer_ok = 0;

	for (n = 0; buffer_in[n] != '\n'; ++n)		// check if reference is hidden
		if (!strncmp (buffer_in + n, " hide", strlen (" hide")))
			return 1;

	for(n = 0; buffer_in[n] != '\n' ; ++n)		// check if reference is on B.SilkS
		if (! strncmp (buffer_in + n, " (layer B.SilkS)", strlen(" (layer B.SilkS)")))		// if reference found on back silkscreen - set layer_ok and warn that it may be wrong
		{
			printf_CYAN ("%s: %s\n	There is reference on bottom layer.\n	Please make sure it's necessary to use bottom layers\n", pretty_name, footprint_name);
			layer_ok = 1;
		}
	for(n = 0; buffer_in[n] != '\n' ; ++n)		// check if reference is on F.SilkS
		if (! strncmp (buffer_in + n, " (layer F.SilkS)", strlen(" (layer F.SilkS)")))		// if reference found on front silkscreen - set layer_ok
			layer_ok = 1;
	if (! layer_ok)		// if reference is not on front or bottom silkscreen layer it's wrong
		return 1;

	for (n = 0; (buffer_in[strlen("  (fp_text reference ") + n]) != ' '; ++n)		// read reference to check if it's "REF**"
		read_ref[n] = buffer_in[strlen ("  (fp_text reference ") + n];
	read_ref[n] = '\0';
	return strcmp (read_ref, "REF**");
}

// sets default reference ("REF**" on F.SilkS/B.SilkS, not hidden)
int fix_ref (char *buffer_in, char *buffer_out, int *modified)
{
	int n;
	int m;
	char text_pos[BUFFER_SIZE];

	for(n = 0; strncmp (buffer_in + n, "(at ", strlen("(at ")); ++n)		// find "(at " to read position of reference
		{;}
	for(m = 0; buffer_in[n + m - 1] != ')'; ++m)		// read position of reference
		text_pos[m] = buffer_in[n + m];
	text_pos[m] = '\0';

	sprintf (buffer_out, "  (fp_text reference %s %s %s\n", "REF**", text_pos, "(layer F.SilkS)");		// assembly line with default reference at original position
	for(n = 0; buffer_in[n] != '\n' ; ++n)		// check if reference is on B.SilkS
		if ((! strncmp (buffer_in + n, " (layer B.SilkS)", strlen(" (layer B.SilkS)"))) || (! strncmp (buffer_in + n, " (layer B.Fab)", strlen(" (layer B.Fab)"))))
			sprintf (buffer_out, "  (fp_text reference %s %s %s\n", "REF**", text_pos, "(layer B.SilkS)");		// assembly line with default reference at original position (overwrite), keep on B.SilkS if it was on B.SilkS or B.Fab

	printf_YELLOW ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

// returns nonzero if value is not the same as footprint name or is on wrong layer or is hidden;
int check_val (char *buffer_in, char *pretty_name, char *footprint_name)
{
	int n;
	char read_value[BUFFER_SIZE];
	int layer_ok = 0;

	for (n = 0; buffer_in[n] != '\n'; ++n)		// check if value is hidden
		if (!strncmp (buffer_in + n, " hide", strlen (" hide")))
			return 1;

	for(n = 0; buffer_in[n] != '\n' ; ++n)		// check if value is on B.Fab
		if (! strncmp (buffer_in + n, " (layer B.Fab)", strlen(" (layer B.Fab)")))		// if value found on back assembly - set layer_ok and warn that it may be wrong
		{
			printf_CYAN ("%s: %s\n	There is value on bottom layer.\n	Please make sure it's necessary to use bottom layers\n", pretty_name, footprint_name);
			layer_ok = 1;
		}
	for(n = 0; buffer_in[n] != '\n' ; ++n)		// check if value is on F.Fab
		if (! strncmp (buffer_in + n, " (layer F.Fab)", strlen(" (layer F.Fab)")))		// if value found on front assembly - set layer_ok
			layer_ok = 1;
	if (! layer_ok)
		return 1;

	for (n = 0; (buffer_in[strlen ("  (fp_text value ") + n]) != ' '; ++n)		// read value to check if it's the same as footprint
		read_value[n] = buffer_in[strlen("  (fp_text value ") + n];
	read_value[n] = '\0';
	return strcmp (read_value, footprint_name);
}

// sets default value (footprint name on F.Fab/B.Fab, not hidden)
int fix_val (char *buffer_in, char *footprint_name, char *buffer_out, int *modified)
{
	int n;
	int m;
	char text_pos[BUFFER_SIZE];

	for(n = 0; strncmp (buffer_in + n, "(at ", strlen("(at ")); ++n)		// find "(at " to read position of value
		{;}
	for(m = 0; buffer_in[n + m - 1] != ')'; ++m)		// read position of value
		text_pos[m] = buffer_in[n + m];
	text_pos[m] = '\0';

	sprintf (buffer_out, "  (fp_text value %s %s %s\n", footprint_name, text_pos, "(layer F.Fab)");		// assembly line with default value at original position
	for(n = 0; buffer_in[n] != '\n' ; ++n)		// check if value is on F.Fab
		if ((! strncmp (buffer_in + n, " (layer B.Fab)", strlen(" (layer B.Fab)"))) || (! strncmp (buffer_in + n, " (layer B.SilkS)", strlen(" (layer B.SilkS)"))))
			sprintf (buffer_out, "  (fp_text value %s %s %s\n", footprint_name, text_pos, "(layer B.Fab)");		// assembly line with default value at original position (overwrite), keep on B.Fab if it was on B.Fab or B.SilkS

	printf_YELLOW ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

// returns nonzero if text field has default size and thickness
int check_font (char *buffer_in)
{
	return (strcmp (buffer_in, "    (effects (font (size 1 1) (thickness 0.15)))\n") != 0);
}

// set default text size and thickness
int fix_font (char *buffer_in, char *buffer_out, int *modified)
{
	sprintf (buffer_out, "    (effects (font (size 1 1) (thickness 0.15)))\n");

	printf_YELLOW ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

// returns nonzero if there is wrong line width or layer
int check_line (char *buffer_in, char *pretty_name, char *footprint_name)
{
	int n;
	int m;
	double d_pos_x;
	double d_pos_y;

	for (n = 0; buffer_in[n] != '\n'; ++n)		// check if there is proper rounding 0.05 of courtyard lines (only fp_line)
		if ((!strncmp (buffer_in + n, " (layer F.CrtYd) ", strlen (" (layer F.CrtYd) "))) || (!strncmp (buffer_in + n, " (layer B.CrtYd) ", strlen (" (layer B.CrtYd) "))))
		{
			for (n = 0; buffer_in[n] != '\n'; ++n)
				if (!strncmp (buffer_in + n, " (start ", strlen (" (start ")))		// search position of start of line
				{
					sscanf(buffer_in + n + strlen (" (start"), "%lg", &d_pos_x);		// read x position as double
					for (m = 1; buffer_in[n + strlen (" (start") + m] != ' '; ++m)		// find space before y position
					{;}
					sscanf(buffer_in + n + strlen (" (start") + m, "%lg", &d_pos_y);		// read y position
					int pos_x = (d_pos_x + ((d_pos_x >= 0) ? 0.0000001 : -0.0000001)) * 1000000;		// convert position to nanometers (add/subtract 1/10^7 to avoid wrong rounding and cast to int)
					int pos_y = (d_pos_y + ((d_pos_y >= 0) ? 0.0000001 : -0.0000001)) * 1000000;
					if (((pos_x % 50000) != 0) || ((pos_y % 50000) != 0))		// check if there is proper rounding (0.05 mm)
						printf_RED ("%s: %s\n	WRONG ROUNDING OF COURTYARD LINE !!!\n", pretty_name, footprint_name);
				}
			for (n = 0; buffer_in[n] != '\n'; ++n)
				if (!strncmp (buffer_in + n, " (end ", strlen (" (end ")))		// search position of end of line
				{
					sscanf(buffer_in + n + strlen (" (end"), "%lg", &d_pos_x);
					for (m = 1; buffer_in[n + strlen (" (end") + m] != ' '; ++m)
					{;}
					sscanf(buffer_in + n + strlen (" (end") + m, "%lg", &d_pos_y);
					int pos_x = (d_pos_x + ((d_pos_x >= 0) ? 0.0000001 : -0.0000001)) * 1000000;
					int pos_y = (d_pos_y + ((d_pos_y >= 0) ? 0.0000001 : -0.0000001)) * 1000000;
					if (((pos_x % 50000) != 0) || ((pos_y % 50000) != 0))
						printf_RED ("%s: %s\n	WRONG ROUNDING OF COURTYARD LINE !!!\n", pretty_name, footprint_name);
				}
		}

	for (n = 0; buffer_in[n] != '\n'; ++n)		// inform about (maybe unnecessary) use of bottom layers
		if ((!strncmp (buffer_in + n, " (layer B.CrtYd) ", strlen (" (layer B.CrtYd) "))) || (!strncmp (buffer_in + n, " (layer B.SilkS) ", strlen (" (layer B.SilkS) "))) || (!strncmp (buffer_in + n, " (layer B.Fab) ", strlen (" (layer B.Fab) "))))
			printf_CYAN ("%s: %s\n	There is silkscreen/courtyard/assembly line on bottom layer.\n	Please make sure if it's necessary to use bottom layers\n", pretty_name, footprint_name);

	for (n = 0; buffer_in[n] != '\n'; ++n)		// check line widths
		if ((!strncmp (buffer_in + n, " (layer F.SilkS) ", strlen (" (layer F.SilkS) "))) || (!strncmp (buffer_in + n, " (layer B.SilkS) ", strlen (" (layer B.SilkS) "))))
		{
			for (n = 0; buffer_in[n] != '\n'; ++n)		// check if silkscreen line width is 0.15 mm
				if (!strncmp (buffer_in + n, " (width 0.15))", strlen (" (width 0.15))")))
					return 0;
				return 1;
		}
		else if ((!strncmp (buffer_in + n, " (layer F.CrtYd) ", strlen (" (layer F.CrtYd) "))) || (!strncmp (buffer_in + n, " (layer B.CrtYd) ", strlen (" (layer B.CrtYd) "))))
		{
			for (n = 0; buffer_in[n] != '\n'; ++n)		// check if courtyard line width is 0.05 mm
				if (!strncmp (buffer_in + n, " (width 0.05))", strlen (" (width 0.05))")))
					return 0;
			return 1;
		}

	return 0;
}

// set default line width
int fix_line (char *buffer_in, char *buffer_out, int *modified)
{
	int n;
	int m;
	char beginning_of_line[BUFFER_SIZE];

	for (n = 0; buffer_in[n] != '\n'; ++n)		// set width of F.CrtYd line
		if (!strncmp (buffer_in + n, " (layer F.CrtYd) ", strlen (" (layer F.CrtYd) ")))
		{
			for (m = 0; (strncmp(buffer_in + m, " (layer ", strlen(" (layer ")) != 0); ++m)
				beginning_of_line[m] = buffer_in[m];
			beginning_of_line[m] = '\0';
			sprintf (buffer_out,"%s (layer F.CrtYd) (width 0.05))\n", beginning_of_line);
		}
	for (n = 0; buffer_in[n] != '\n'; ++n)		// set width of F.SilkS line
		if (!strncmp (buffer_in + n, " (layer F.SilkS) ", strlen (" (layer F.SilkS) ")))
		{
			for (m = 0; (strncmp(buffer_in + m, " (layer ", strlen(" (layer ")) != 0); ++m)
				beginning_of_line[m] = buffer_in[m];
			beginning_of_line[m] = '\0';
			sprintf (buffer_out,"%s (layer F.SilkS) (width 0.15))\n", beginning_of_line);
		}
	for (n = 0; buffer_in[n] != '\n'; ++n)		// set width of B.CrtYd line
		if (!strncmp (buffer_in + n, " (layer B.CrtYd) ", strlen (" (layer B.CrtYd) ")))
		{
			for (m = 0; (strncmp(buffer_in + m, " (layer ", strlen(" (layer ")) != 0); ++m)
				beginning_of_line[m] = buffer_in[m];
			beginning_of_line[m] = '\0';
			sprintf (buffer_out,"%s (layer B.CrtYd) (width 0.05))\n", beginning_of_line);
		}
	for (n = 0; buffer_in[n] != '\n'; ++n)		// set width of B.SilkS line
		if (!strncmp (buffer_in + n, " (layer B.SilkS) ", strlen (" (layer B.SilkS) ")))
		{
			for (m = 0; (strncmp(buffer_in + m, " (layer ", strlen(" (layer ")) != 0); ++m)
				beginning_of_line[m] = buffer_in[m];
			beginning_of_line[m] = '\0';
			sprintf (buffer_out,"%s (layer B.SilkS) (width 0.15))\n", beginning_of_line);
		}

	printf_YELLOW ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

// returns 1 (if called it must be warning)
int check_unnecessary ()
{
	return 1;
}

// remove (usually unnecessary) lines starting with "  (autoplace_cost", "  (solder_", "  (clearance "
int fix_unnecessary (char *buffer_in, char *buffer_out, int *modified)
{
	strcpy (buffer_out, "");

	printf_YELLOW ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

// returns nonzero if 3d model is not LIBRARYNAME.3dshapes/FOOTPRINTNAME.wrl
int check_3d (char *buffer_in, char *pretty_name, char *footprint_name)
{
	char link_3d[BUFFER_SIZE];
	sprintf (link_3d,"  (model %s.3dshapes/%s.wrl\n", pretty_name, footprint_name);
	return strcmp (buffer_in, link_3d);
}

// changes path to 3d model, copies .wrl and .wings to temporary files, prepares paths to move 3d models to proper locations (in main())
int fix_3d (char *buffer_in, char *pretty_name, char *footprint_name, char *path_to_models, char *buffer_out, FILE *file_wrl_tmp, FILE *file_wings_tmp, char *path_3d_wrl, char *path_3d_wings, char *path_3d_new_3dshapes,	char *path_3d_new_wrl, char *path_3d_new_wings, char *path_3d_3dshapes, int *wings_exists, int *change_3d, int *modified)
{
	int n;
	int intc;
	char confirm;
	FILE *file_wrl;
	FILE *file_wings;
	char read_3d_path[BUFFER_SIZE];
	int quoted = 0;

	// old .wrl
	if (! strncmp (buffer_in,"  (model \"", strlen ("  (model \"")))		// read .wrl path from file if quoted
	{
		strncpy (read_3d_path, buffer_in + strlen ("  (model \""), strlen (buffer_in) - strlen ("  (model \""));		// read 3d path from .kicad_mod
		read_3d_path[strlen (buffer_in) - strlen ("  (model \"") - 2] = '\0';		// add '\0' at the end to create string (overwrite " before \n)
		sprintf (path_3d_wrl,"%s/%s", path_to_models, read_3d_path);		// global path to old 3d .wrl model (path to packages3d provided by user + path read from file)
		quoted = 1;		// set quoted, if the path is correct but quoted - quotation will be removed
	}
	else		// read .wrl path from file if not quoted
	{
		strncpy (read_3d_path, buffer_in + strlen ("  (model "), strlen (buffer_in) - strlen ("  (model "));		// read 3d path from .kicad_mod
		read_3d_path[strlen (buffer_in) - strlen ("  (model ") - 1] = '\0';		// add '\0' at the end to create string (overwrite \n)
		sprintf (path_3d_wrl,"%s/%s", path_to_models, read_3d_path);		// global path to old 3d .wrl model (path to packages3d provided by user + path read from file)
	}
	// old .wings
	strcpy (path_3d_wings, path_3d_wrl);
	path_3d_wings[strlen (path_3d_wrl) - strlen (".wrl")] = '\0';
	strcat(path_3d_wings, ".wings");		// global path to old 3d .wings model - obtain by cutting ".wrl" and adding .wings to path to .wrl
	// old .3dshapes
	strcpy(path_3d_3dshapes, path_3d_wrl);
	for (n = strlen (path_3d_3dshapes); (path_3d_3dshapes[n] != '/') && (n > 0); --n)
	{;}
	path_3d_3dshapes[n] = '\0';		// global path to old .3dshapes directory (or other directory with .wrl/.wings)
	// new .wrl
	sprintf (path_3d_new_wrl, "%s/%s.3dshapes/%s.wrl", path_to_models, pretty_name, footprint_name);		// global path to new .wrl file (with desired name and location)
	// new .wings
	sprintf (path_3d_new_wings, "%s/%s.3dshapes/%s.wings", path_to_models, pretty_name, footprint_name);		// global path to new .wings file (with desired name and location)
	// new .3dshapes
	sprintf (path_3d_new_3dshapes, "%s/%s.3dshapes", path_to_models, pretty_name);		// global path to new .3dshapes (in desired location)

	if (quoted && ! strcmp (path_3d_wrl, path_3d_new_wrl))		// if the path is correct but quoted - quotation will be removed
	{
		sprintf (buffer_out, "  (model %s.3dshapes/%s.wrl\n", pretty_name, footprint_name);		// remove quotation in path to 3d model in .kicad_mod file
		printf_YELLOW ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
		if (*modified >= 0)
			*modified += 1;
		return 0;
	}
	else if (access (path_3d_new_wings, F_OK) && access (path_3d_new_wrl, F_OK))		// 3d path can be changed only if it won't overwrite anything
	{
		if ((file_wrl = fopen (path_3d_wrl, "rb")) != NULL)		// if .wrl linked properly, copy it to temporary file and set new path in .kicad_mod
		{
			while ((intc = fgetc (file_wrl)) != EOF)		// copy .wrl file to temporary file
				fputc (intc, file_wrl_tmp);
			fclose (file_wrl);
			sprintf (buffer_out, "  (model %s.3dshapes/%s.wrl\n", pretty_name, footprint_name);		// set new path to 3d model in .kicad_mod file
			printf_YELLOW ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
		}
		else		// don't allow to modify anything if wrong 3d link
		{
			printf_RED ("Couldn't open .wrl file\n");
			*modified = -1;
			return 1;
		}

		if ((file_wings = fopen (path_3d_wings, "rb")) != NULL)		// if .wings corresponding with .wrl exists, copy it to temporary file
		{
			while ((intc = fgetc (file_wings)) != EOF)
				fputc (intc, file_wings_tmp);
			fclose (file_wings);
			*wings_exists = 1;		// set to inform that .wings should be copied from temporary file (in main())
		}
		else	// if .wings corresponding with .wrl not exist, ask if it should exist, if it should but not found - error (won't modify)
		{
			printf_YELLOW ("Couldn't open .wings file\n");
			printf_YELLOW ("	does it exist? (enter y for yes or n for no)\n");		// ask if .wings exists
			do
				scanf (" %c", &confirm);
			while ((confirm != 'y') && (confirm != 'n'));
			if (confirm == 'y')		// if there should be .wings file (but it's not found) don't allow to modify anything
			{
				printf_RED ("can't find .wings file\n\n");
				*modified = -1;
				return 1;
			}
			else
				*wings_exists = 0;		// if it's accepted that there is no .wings corresponding to .wrl - reset wings_exists
		}

		*change_3d = 1;		// if .wrl exists and .wings exists (or it's accepted that not exists) allow modify footprint and model (increment modify) and inform that changes affects also 3d model (set change_3d)
		if (*modified >= 0)
			*modified += 1;
		return 0;
	}
	else		// don't allow to modify anything if 3d model would be overwritten
	{
		printf_RED ("Couldn't rename 3D - file exists in new location\n");
		*modified = -1;
		return 1;
	}
}



int main (void)
{

	DIR *dir_pretty;							// directory containing .pretty libraries
	DIR *dir_library;							// directory containing .kicad_mod footprints

	FILE *file_kicad_mod;						// .kicad_mod file
	FILE *file_kicad_mod_tmp;					// temporary .kicad_mod file
	FILE *file_new_wrl;							// new .wrl file
	FILE *file_wrl_tmp;							// temporary .wrl file
	FILE *file_new_wings;						// new .wings file
	FILE *file_wings_tmp;						// temporary .wings file
	struct dirent *ep;							// structure containing name of file got by readdir
	struct dirent pretty_list[MAX_LIB_NR];		// array containing list of .pretty libraries
	struct dirent footprint_list[MAX_LIB_NR];	// array containing list of .kicad_mod footprints

	char path_to_libs[BUFFER_SIZE];			// path to directory containing .pretty libraries
	char pretty_path[BUFFER_SIZE];			// path to .pretty library
	char kicad_mod_path[BUFFER_SIZE];		// path to .kicad_mod footprint
	char path_to_models[BUFFER_SIZE];		// path to directory containing .3dshapes directories
	char path_3d_3dshapes[BUFFER_SIZE];			// path to .3dshapes directory
	char path_3d_new_3dshapes[BUFFER_SIZE];		// new path to .3dshapes directory
	char path_3d_wrl[BUFFER_SIZE];				// path to original .wrl file
	char path_3d_new_wrl[BUFFER_SIZE];		// path to new .wrl file
	char path_3d_wings[BUFFER_SIZE];			// path to original .wings file
	char path_3d_new_wings[BUFFER_SIZE];	// path to new .wings file

	char pretty_name[BUFFER_SIZE];			// name of pretty library (directory name without .pretty)
	char footprint_name[BUFFER_SIZE];		// name of footprint (file name without .kicad_mod)
	char buffer_in[BUFFER_SIZE];				// buffer for line read from file
	char buffer_out[BUFFER_SIZE];				// buffer for line to write to file

	int n;										// counter for checking filename
	int m;										// counter for pads and attributes
	int i;										// .pretty counter
	int j;										// .kicad_mod counter
	int intc;									// int variable (for copying files)
	int modified;								// set if footprint is modified
	int change_3d;								// set if 3d is modified
	char confirm;								// decide: y for yes, n for no, or 'press enter to continue'
	int wings_exists;							// set if .wings file exists
	int tht;									// count tht pads
	int smd;									// count smd pads
	int attr;									// normal = 0, insert = 1, virtual = 2

	printf_CYAN ("\nThis software checks some of KiCad Library Convention (KLC) rules and\nhelps with adjusting footprints to the convention\n");
	printf_RED ("\nIT MAY DESTROY YOUR DATA\nPlease make sure you have BACKUP COPY of footprints and 3d models\nand you enter VAILD PATHS\nPlease make sure you understand KLC and keep in mind that not all of\nsuggested changes must be in line with your intentions\n");
	printf_CYAN ("\nYou will get information about possible KLC deviations and\n(if you accept) wrong lines will be changed.\n");
	printf_CYAN ("Use at your own risk.\n\n");
	printf_GREEN ("Please enter path to directory containing .pretty libraries\n");
	scanf_RED ("%s", path_to_libs);
	printf_GREEN ("Please enter path to directory \"packages3d\"\n");
	scanf_RED ("%s", path_to_models);
	printf_YELLOW ("\n\n");

//////////////////////////////////////////////////////////////////////
//	get list of .pretty libraries to array
//////////////////////////////////////////////////////////////////////
	dir_pretty = opendir (path_to_libs);
	if (dir_pretty != NULL)
	{
		for (i = 0; (ep = readdir (dir_pretty)) != NULL; ++i)
		{
			if (strncmp (ep->d_name + strlen (ep->d_name) - strlen (".pretty"), ".pretty", strlen (".pretty")) == 0)		//add only .pretty files
				pretty_list[i] = *ep;
			else
				--i;
		}
      	closedir (dir_pretty);
      	strcpy (pretty_list[i].d_name, "||\\//||");		//add "||\\//||" at the end of the table to recognize when it finishes
    }
	else
	{
		printf_RED ("Couldn't open directory containing .pretty libraries\n");
		exit (1);
	}


//////////////////////////////////////////////////////////////////////
//	do following operations on all .pretty libraries
//////////////////////////////////////////////////////////////////////
	for (i = 0; strcmp (pretty_list[i].d_name, "||\\//||") != 0; ++i)
	{
		//---------------------------------------------------------------
		//	get list of .kicad_mod footprints in current .pretty library
		//---------------------------------------------------------------
		sprintf (pretty_path,"%s/%s", path_to_libs, pretty_list[i].d_name);
		dir_library = opendir (pretty_path);		//open .pretty with footprints to modify
		if (dir_library != NULL)
		{
			for (j = 0; (ep = readdir (dir_library)) != NULL; ++j)
			{
				if (strncmp (ep->d_name + strlen (ep->d_name) - strlen (".kicad_mod"), ".kicad_mod", strlen (".kicad_mod")) == 0)		//add only .kicad_mod files
					footprint_list[j] = *ep;
				else
					--j;
			}
		  	closedir (dir_library);
		  	strcpy (footprint_list[j].d_name, "||\\//||");		//add "||\\//||" at the end of the table to recognize when it finishes
		}
		else
		{
			printf_RED ("Couldn't open .pretty library\n");
			exit (1);
		}


		//HERE OPERATIONS ON FOOTPRINTS AND 3D MODELS ARE DONE
		//---------------------------------------------------------------
		//	do following operations on all footprints in current library
		//---------------------------------------------------------------
		for (j = 0; strcmp(footprint_list[j].d_name, "||\\//||") != 0; ++j)
		{
			sprintf (kicad_mod_path,"%s/%s/%s", path_to_libs, pretty_list[i].d_name, footprint_list[j].d_name);		//prepare path to read original .kicad_mod file
			sprintf (pretty_name,"%.*s", (unsigned int)(strlen(pretty_list[i].d_name) - strlen(".pretty")), pretty_list[i].d_name);	//name of pretty library (directory name without .pretty)
			sprintf (footprint_name,"%.*s", (unsigned int)(strlen(footprint_list[j].d_name) - strlen(".kicad_mod")), footprint_list[j].d_name);	//name of footprint (file name without .kicad_mod)
			modified = 0;
			change_3d = 0;
			wings_exists = 0;
			tht = 0;
			smd = 0;
			attr = 0;

			for (n = 0; pretty_name[n] != '\0'; ++n)
				if (!( ((pretty_name[n] >= 'a') && (pretty_name[n] <= 'z')) || ((pretty_name[n] >= 'A') && (pretty_name[n] <= 'Z')) || ((pretty_name[n] >= '0') && (pretty_name[n] <= '9')) || (pretty_name[n] == '.') || (pretty_name[n] <= '-') || (pretty_name[n] <= '_') ))
				{
					printf_RED ("bad library name\n");
					exit (1);
				}
			for (n = 0; footprint_name[n] != '\0'; ++n)
				if (!( ((footprint_name[n] >= 'a') && (footprint_name[n] <= 'z')) || ((footprint_name[n] >= 'A') && (footprint_name[n] <= 'Z')) || ((footprint_name[n] >= '0') && (footprint_name[n] <= '9')) || (footprint_name[n] == '.') || (footprint_name[n] <= '-') || (footprint_name[n] <= '_') ))
				{
					printf_RED ("bad footprint name\n");
					exit (1);
				}


			if ((file_kicad_mod = fopen(kicad_mod_path, "rt")) != NULL)		//open footprint to edit
			{
				if (((file_kicad_mod_tmp = tmpfile ()) != NULL) && ((file_wrl_tmp = tmpfile ()) != NULL) && ((file_wings_tmp = tmpfile ()) != NULL))		//create temporary file - write to it and next copy to footprint
				{
					//HERE .kicad_mod FOOTPRINTS ARE CHECKED
					for (fgets (buffer_in, BUFFER_SIZE, file_kicad_mod); !feof (file_kicad_mod); fgets (buffer_in, BUFFER_SIZE, file_kicad_mod))		//read lines of .kicad_mod file until reaches EOF (must be \n before EOF)
					{
						strcpy (buffer_out, buffer_in);
//------------------------------------------------------
						if ((strncmp("(module ", buffer_in, strlen("(module ")) == 0))
						{
							if (check_name_locked (buffer_in, footprint_name))
							{
								printf_YELLOW ("%s: %s\n	warning: module name not equal to footprint name or footprint locked\n", pretty_name, footprint_name);
								printf_YELLOW ("	fix it? (enter y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_name_locked (buffer_in, footprint_name, buffer_out, &modified);
							}
						}
//-------
						if ((strncmp("  (tags ", buffer_in, strlen("  (tags ")) == 0))
						{
							if (check_tags (buffer_in))
							{
								printf_YELLOW ("%s: %s\n	warning: comma in tags found\n", pretty_name, footprint_name);
								printf_YELLOW ("	fix it? (enter y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_tags (buffer_in, buffer_out, &modified);
							}
						}

						else if ((strncmp("  (fp_text reference ", buffer_in, strlen("  (fp_text reference ")) == 0))
						{
							if (check_ref (buffer_in, pretty_name, footprint_name))
							{
								printf_YELLOW ("%s: %s\n	warning: reference not equal to REF** or hidden or wrong layer\n", pretty_name, footprint_name);
								printf_YELLOW ("	fix it? (enter y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_ref (buffer_in, buffer_out, &modified);
							}
						}

						else if ((strncmp("  (fp_text value ", buffer_in, strlen("  (fp_text value ")) == 0))
						{
							if (check_val (buffer_in, pretty_name, footprint_name))
							{
								printf_YELLOW ("%s: %s\n	warning: value not equal to footprint name or hidden or wrong layer\n", pretty_name, footprint_name);
								printf_YELLOW ("	fix it? (enter y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_val (buffer_in, footprint_name, buffer_out, &modified);
							}
						}
//-------
						else if (strncmp("    (effects (font ", buffer_in, strlen("    (effects (font ")) == 0)
						{
							if (check_font (buffer_in))
							{
								printf_YELLOW ("%s: %s\n	warning: text size is not 1 x 1 x 0.15 mm\n", pretty_name, footprint_name);
								printf_YELLOW ("	fix it? (enter y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_font (buffer_in, buffer_out, &modified);
							}
						}
//-------
						else if ((strncmp("  (fp_line ", buffer_in, strlen("  (fp_line ")) == 0) || (strncmp("  (fp_circle ", buffer_in, strlen("  (fp_circle ")) == 0) || (strncmp("  (fp_arc ", buffer_in, strlen("  (fp_arc ")) == 0) || (strncmp("  (fp_poly ", buffer_in, strlen("  (fp_poly ")) == 0) || (strncmp("  (fp_curve ", buffer_in, strlen("  (fp_curve ")) == 0))
						{
							if (check_line (buffer_in, pretty_name, footprint_name))
							{
								printf_YELLOW ("%s: %s\n	warning: silkscreen line width not equal to 0.15 or courtyard line width not equal to 0.05\n", pretty_name, footprint_name);
								printf_YELLOW ("	fix it? (enter y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_line (buffer_in, buffer_out, &modified);
							}
						}
//-------
						else if ((strncmp("  (autoplace_cost", buffer_in, strlen("  (autoplace_cost")) == 0) || (strncmp("  (solder_", buffer_in, strlen("  (solder_")) == 0) || (strncmp("  (clearance ", buffer_in, strlen("  (clearance ")) == 0))
						{
							if (check_unnecessary ())
							{
								printf_YELLOW ("%s: %s\n	warning: unnecessary footprint properties set\n", pretty_name, footprint_name);
								printf_YELLOW ("	fix it? (enter y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_unnecessary (buffer_in, buffer_out, &modified);
							}
						}

//-------
						else if (strncmp("  (model ", buffer_in, strlen("  (model ")) == 0)		//search line with link to 3d model
						{
							if (check_3d (buffer_in, pretty_name, footprint_name))
							{
								printf_YELLOW ("%s: %s\n	warning: 3d model path not equal to footprint path\n", pretty_name, footprint_name);
								printf_YELLOW ("	fix it? (enter y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_3d (buffer_in, pretty_name, footprint_name, path_to_models, buffer_out, file_wrl_tmp, file_wings_tmp, path_3d_wrl, path_3d_wings, path_3d_new_3dshapes,	path_3d_new_wrl, path_3d_new_wings, path_3d_3dshapes, &wings_exists, &change_3d, &modified);
							}
						}

						else if (strncmp("  (pad ", buffer_in, strlen("  (pad ")) == 0)		//search line with link to 3d model
						{
							for (m = 0; buffer_in[m] != '\n'; ++m)		// count smd pads
								if (!strncmp (buffer_in + m, " smd", strlen (" smd")))
									++smd;
							for (m = 0; buffer_in[m] != '\n'; ++m)		// count tht pads
								if (!strncmp (buffer_in + m, " thru_hole", strlen (" thru_hole")))
									++tht;
						}
						else if (strncmp("  (attr ", buffer_in, strlen("  (attr ")) == 0)		//search line with link to 3d model
						{
							for (m = 0; buffer_in[m] != '\n'; ++m)		// count tht pads
								if (!strncmp (buffer_in + m, " smd", strlen (" smd")))
									attr = 1;
							for (m = 0; buffer_in[m] != '\n'; ++m)		// count tht pads
								if (!strncmp (buffer_in + m, " virtual", strlen (" virtual")))
									attr = 2;
						}

//------------------------------------------------------
						fprintf (file_kicad_mod_tmp, "%s", buffer_out);
					}
				}
				else
				{
					printf_RED ("Couldn't create temporary files\n");
					exit (1);
				}
				fclose (file_kicad_mod);
			}
			else
				printf_RED ("Couldn't open .kicad_mod file\n");



			//-------------------------------------------------------------------------------------------------------------------------------------
			// If all went OK - apply changes:
			// copy temporary files to .kicad_mod and new .wrl/.wings
			// remove old .wrl/.wings
			// create new .3dshapes if not exist, remove old folder with .wrl/.wings if empty
			//-------------------------------------------------------------------------------------------------------------------------------------
			if (modified > 0)		// never get into this block that changes files and directories if nothing modified (modify == 0) or error (modify == -1)
			{
				printf_CYAN ("\nConfirm to apply changes to:\n	%s: %s\n(enter y for yes or n for no)\n\n", pretty_name, footprint_name);
				do
					scanf (" %c", &confirm);
				while ((confirm != 'y') && (confirm != 'n'));
				if (confirm == 'y')
				{
					if ((file_kicad_mod = fopen(kicad_mod_path, "wb")) != NULL)		// copy .kicad_mod to new location (from temporary file containing fixed footprint)
					{
						rewind (file_kicad_mod_tmp);
						while ((intc = fgetc (file_kicad_mod_tmp)) != EOF)
							fputc (intc, file_kicad_mod);
						fclose (file_kicad_mod);
					}
					else
						printf_RED ("Couldn't create .kicad_mod file\n");


					if (change_3d)		// change 3d (.wrl, .wings, folders) if there were any changes in 3d models
					{
						if (access (path_3d_new_3dshapes, F_OK))		// create directory with proper name if not exists
						{
							#if defined(__MINGW32__)
								mkdir (path_3d_new_3dshapes);
							#else
								mkdir (path_3d_new_3dshapes, 0751);
							#endif
						}

						if ((file_new_wrl = fopen(path_3d_new_wrl, "wb")) != NULL)		// copy .wrl to new location (from temporary file created by function fix_3d)
						{
							rewind (file_wrl_tmp);
							while ((intc = fgetc (file_wrl_tmp)) != EOF)
								fputc (intc, file_new_wrl);
							fclose (file_new_wrl);
							unlink (path_3d_wrl);		// remove old .wrl file
						}
						else
							printf_RED ("ERROR: Couldn't create .wrl file!\nSomething went wrong - broken link in .kicad_mod!!!\n");

						if (wings_exists)
						{
							if ((file_new_wings = fopen(path_3d_new_wings, "wb")) != NULL)		// copy .wings to new location (from temporary file created by function fix_3d)
							{
								rewind (file_wings_tmp);
								while ((intc = fgetc (file_wings_tmp)) != EOF)
									fputc (intc, file_new_wings);
								fclose (file_new_wings);
								unlink (path_3d_wings);		// remove old .wings file
							}
							else
								printf_RED ("ERROR: Couldn't create .wings file!\nSomething went wrong - .wrl and .wings can be inconsistent!!!\n");
						}

						rmdir (path_3d_3dshapes);		// remove old .3dshapes directory if empty
					}
				}
			}
			else if (modified < 0)
				printf_RED ("\nFAILED!!!\n	%s/%s not changed\n\n", pretty_name, footprint_name);
			fclose (file_kicad_mod_tmp);
			fclose (file_wrl_tmp);
			fclose (file_wings_tmp);

			if ((smd - tht > 0) && (attr == 0))
				printf_CYAN ("\n%s/%s\n	INFO: attributes are 'normal' but most of pads are SMD\n	please check attributes\n", pretty_name, footprint_name);

			if ((tht - smd > 0) && (attr == 1))
				printf_CYAN ("\n%s/%s\n	INFO: attributes are 'normal+insert' but most of pads are THT\n	please check attributes\n", pretty_name, footprint_name);
		}
	}


	printf_GREEN ("\nCheck finished\n");
	return 0;
}
