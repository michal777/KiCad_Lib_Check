#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_LIB_NR 500
#define MAX_NAME_LENGTH 500
#define BUFFER_SIZE 2000

#ifdef __MINGW32__
	#include <windows.h>
	#define _RED_ SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), FOREGROUND_RED);
	#define _YELLOW_ SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
	#define _CYAN_ SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE);
	#define _GREEN_ SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
#endif

#ifdef __linux__
	#define _RED_ printf("\033[22;31m");
	#define _YELLOW_ printf("\033[01;33m");
	#define _CYAN_ printf("\033[22;36m");
	#define _GREEN_ printf("\033[22;32m");
#endif






int check_name_locked (char *buffer_in, char *footprint_name)		//returns non zero if wrong module name or footprint locked
{
	int n;
	char read_module_name[MAX_NAME_LENGTH];
	for (n = 0; buffer_in[n] != '\n'; ++n)		// check if footprint is locked (search " locked " in first line)
		if (!strncmp (buffer_in + n, " locked ", strlen (" locked ")))
			return 1;
	for (n = 0; (buffer_in[strlen("(module ") + n]) != ' '; ++n)		// read module name from first line to check if the same as footprint name
		read_module_name[n] = buffer_in[strlen("(module ") + n];
	read_module_name[n] = '\0';
	return strcmp (read_module_name, footprint_name);
}

int fix_name_locked (char *buffer_in, char *footprint_name, char *buffer_out, int *modified)		// sets proper module name and removes "locked" property (if was present)
{
	int n;
	int m;
	char rest_of_line[BUFFER_SIZE];
	for(n = 0; strncmp (buffer_in + n, " (layer ", strlen(" (layer ")) ; ++n)		// find in first line beginning of " (layer "
		{;}
	for(m = 0; (buffer_in[n + m - 1]) != '\n'; ++m)		// save rest of line - following "(module <name> [locked] "
		rest_of_line[m] = buffer_in[n + m];
	sprintf (buffer_out, "(module %s%s", footprint_name, rest_of_line);		// assembly first line: take proper name, ignore " locked ", copy layer and time-stamp
	printf ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
	if (*modified >= 0)		// increment value of modified only if there were no errors (modified == -1)
		*modified += 1;
	return 0;
}

int check_tags (char *buffer_in)		//returns non zero if comma in tags found
{
	int n;
	for (n = 0; buffer_in[n] != '\n'; ++n)		// search comma
		if (buffer_in[n] == ',')
			return 1;
	return 0;
}

int fix_tags (char *buffer_in, char *buffer_out, int *modified)		// sets proper module name and removes "locked" property (if was present)
{
	int n;
	int m;
	puts("asdf");
	for (n = 0, m = 0; buffer_in[n] != '\n'; ++n, ++m)		// search and replace comma
	{	puts("zxcv");
		if ((buffer_in[n] == ',') && (buffer_in[n + 1] != ' '))
			buffer_out[m] = ' ';
		else if ((buffer_in[n] == ',') && (buffer_in[n + 1] == ' '))
			--m;
		else
			buffer_out[m] = buffer_in[n];
	}
	printf ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
	if (*modified >= 0)		// increment value of modified only if there were no errors (modified == -1)
		*modified += 1;
	return 0;
}

int check_ref (char *buffer_in)		// returns non zero if reference is not "REF**" or is on wrong layer or is hidden
{
	int n;
	char read_ref[MAX_NAME_LENGTH];
	int layer_ok = 0;
	for (n = 0; buffer_in[n] != '\n'; ++n)		// check if reference is hidden
		if (!strncmp (buffer_in + n, " hide", strlen (" hide")))
			return 1;
	for(n = 0; buffer_in[n] != '\n' ; ++n)		// check if reference is on F.SilkS
		layer_ok = layer_ok | (! strncmp (buffer_in + n, " (layer F.SilkS)", strlen(" (layer F.SilkS)")));
	if (! layer_ok)
		return 1;
	for (n = 0; (buffer_in[strlen("  (fp_text reference ") + n]) != ' '; ++n)		// read reference to check if it's "REF**"
		read_ref[n] = buffer_in[strlen ("  (fp_text reference ") + n];
	read_ref[n] = '\0';
	return strcmp (read_ref, "REF**");
}

int fix_ref (char *buffer_in, char *buffer_out, int *modified)		// sets default reference ("REF**" on F.SilkS, not hidden)
{
	int n;
	int m;
	char text_pos[BUFFER_SIZE];
	for(n = 0; strncmp (buffer_in + n, "(at ", strlen("(at ")); ++n)		// find "(at " to read position of reference
		{;}
	for(m = 0; buffer_in[n + m - 1] != ')'; ++m)		// read position of reference
		text_pos[m] = buffer_in[n + m];
	text_pos[m] = '\0';
	sprintf (buffer_out, "  (fp_text reference %s %s %s\n", "REF**", text_pos, "(layer F.SilkS)");		// assembly line with default reference (at original position)
	printf ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

int check_val (char *buffer_in, char *footprint_name)		// returns non zero if value is not the same as footprint name or is on wrong layer or is hidden;
{
	int n;
	char read_value[MAX_NAME_LENGTH];
	int layer_ok = 0;
	for (n = 0; buffer_in[n] != '\n'; ++n)		// check if value is hidden
		if (!strncmp (buffer_in + n, " hide", strlen (" hide")))
			return 1;
	for(n = 0; buffer_in[n] != '\n' ; ++n)		// check if value is on F.Fab
		layer_ok = layer_ok | (! strncmp (buffer_in + n, " (layer F.Fab)", strlen(" (layer F.Fab)")));
	if (! layer_ok)
		return 1;
	for (n = 0; (buffer_in[strlen ("  (fp_text value ") + n]) != ' '; ++n)		// read value to check if it's the same as footprint
		read_value[n] = buffer_in[strlen("  (fp_text value ") + n];
	read_value[n] = '\0';
	return strcmp (read_value, footprint_name);
}

int fix_val (char *buffer_in, char *footprint_name, char *buffer_out, int *modified)		// sets default value (footprint name on F.Fab, not hidden)
{
	int n;
	int m;
	char text_pos[BUFFER_SIZE];
	for(n = 0; strncmp (buffer_in + n, "(at ", strlen("(at ")); ++n)		// find "(at " to read position of value
		{;}
	for(m = 0; buffer_in[n + m - 1] != ')'; ++m)		// read position of value
		text_pos[m] = buffer_in[n + m];
	text_pos[m] = '\0';
	sprintf (buffer_out, "  (fp_text value %s %s %s\n", footprint_name, text_pos, "(layer F.Fab)");		// assembly line with default value (at original position)
	printf ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

int check_font (char *buffer_in)		// returns non zero if text field has default size and thickness
{
	return (strcmp (buffer_in, "    (effects (font (size 1 1) (thickness 0.15)))\n") != 0);
}

int fix_font (char *buffer_in, char *buffer_out, int *modified)		// set default text size and thickness
{
	sprintf (buffer_out, "    (effects (font (size 1 1) (thickness 0.15)))\n");
	printf ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

int check_line (char *buffer_in)		// returns non zero if there is wrong line width or layer
{
	int n;
	int m;
	double d_pos_x;
	double d_pos_y;
	for (n = 0; buffer_in[n] != '\n'; ++n)
		if (!strncmp (buffer_in + n, " (layer F.CrtYd) ", strlen (" (layer F.CrtYd) ")))
		{
			for (n = 0; buffer_in[n] != '\n'; ++n)
				if (!strncmp (buffer_in + n, " (start ", strlen (" (start ")))
				{
					sscanf(buffer_in + n + strlen (" (start"), "%lg", &d_pos_x);
					for (m = 1; buffer_in[n + strlen (" (start") + m] != ' '; ++m)
					{;}
					sscanf(buffer_in + n + strlen (" (start") + m, "%lg", &d_pos_y);
					int pos_s_x = d_pos_x * 1000000;
					int pos_s_y = d_pos_y * 1000000;
					if (((pos_s_x % 50000) != 0) || ((pos_s_y % 50000) != 0))
						return 1;
				}
			for (n = 0; buffer_in[n] != '\n'; ++n)
				if (!strncmp (buffer_in + n, " (end ", strlen (" (end ")))
				{
					sscanf(buffer_in + n + strlen (" (end"), "%lg", &d_pos_x);
					for (m = 1; buffer_in[n + strlen (" (end") + m] != ' '; ++m)
					{;}
					sscanf(buffer_in + n + strlen (" (end") + m, "%lg", &d_pos_y);
					int pos_e_x = d_pos_x * 1000000;
					int pos_e_y = d_pos_y * 1000000;
					if (((pos_e_x % 50000) != 0) || ((pos_e_y % 50000) != 0))
						return 1;
				}
			for (n = 0; buffer_in[n] != '\n'; ++n)
				if (!strncmp (buffer_in + n, " (width 0.05))", strlen (" (width 0.05))")))
					return 0;
				return 1;
		}
	for (n = 0; buffer_in[n] != '\n'; ++n)
		if (!strncmp (buffer_in + n, " (layer F.SilkS) ", strlen (" (layer F.SilkS) ")))
		{
			for (n = 0; buffer_in[n] != '\n'; ++n)
				if (!strncmp (buffer_in + n, " (width 0.15))", strlen (" (width 0.15))")))
					return 0;
				return 1;
		}
	for (n = 0; buffer_in[n] != '\n'; ++n)
		if (!strncmp (buffer_in + n, " (layer B.CrtYd) ", strlen (" (layer B.CrtYd) ")))
		{
			_YELLOW_
			printf ("There is courtyard on bottom layer. Please make sure it's necessary to use bottom layers\n");
			for (n = 0; buffer_in[n] != '\n'; ++n)
				if (!strncmp (buffer_in + n, " (start ", strlen (" (start ")))
				{
					sscanf(buffer_in + n + strlen (" (start"), "%lg", &d_pos_x);
					for (m = 1; buffer_in[n + strlen (" (start") + m] != ' '; ++m)
					{;}
					sscanf(buffer_in + n + strlen (" (start") + m, "%lg", &d_pos_y);
					int pos_s_x = d_pos_x * 1000000;
					int pos_s_y = d_pos_y * 1000000;
					if (((pos_s_x % 50000) != 0) || ((pos_s_y % 50000) != 0))
						return 1;
				}
			for (n = 0; buffer_in[n] != '\n'; ++n)
				if (!strncmp (buffer_in + n, " (end ", strlen (" (end ")))
				{
					sscanf(buffer_in + n + strlen (" (end"), "%lg", &d_pos_x);
					for (m = 1; buffer_in[n + strlen (" (end") + m] != ' '; ++m)
					{;}
					sscanf(buffer_in + n + strlen (" (end") + m, "%lg", &d_pos_y);
					int pos_e_x = d_pos_x * 1000000;
					int pos_e_y = d_pos_y * 1000000;
					if (((pos_e_x % 50000) != 0) || ((pos_e_y % 50000) != 0))
						return 1;
				}
			for (n = 0; buffer_in[n] != '\n'; ++n)
				if (!strncmp (buffer_in + n, " (width 0.05))", strlen (" (width 0.05))")))
					return 0;
				return 1;
		}
	for (n = 0; buffer_in[n] != '\n'; ++n)
		if (!strncmp (buffer_in + n, " (layer B.SilkS) ", strlen (" (layer B.SilkS) ")))
		{
			_YELLOW_
			printf ("There is silkscreen on bottom layer. Please make sure it's necessary to use bottom layers\n");
			for (n = 0; buffer_in[n] != '\n'; ++n)
				if (!strncmp (buffer_in + n, " (width 0.15))", strlen (" (width 0.15))")))
					return 0;
				return 1;
		}
	return 0;
}

int fix_line (char *buffer_in, char *buffer_out, int *modified)		// set default line width
{
	int n;
	int m;
	char beginning_of_line[BUFFER_SIZE];
	for (n = 0; buffer_in[n] != '\n'; ++n)
		if (!strncmp (buffer_in + n, " (layer F.CrtYd) ", strlen (" (layer F.CrtYd) ")))
		{
			for (m = 0; (strncmp(buffer_in + m, " (layer ", strlen(" (layer ")) != 0); ++m)
				beginning_of_line[m] = buffer_in[m];
			beginning_of_line[m] = '\0';
			sprintf(buffer_out,"%s (layer F.CrtYd) (width 0.05))\n", beginning_of_line);
		}
	for (n = 0; buffer_in[n] != '\n'; ++n)
		if (!strncmp (buffer_in + n, " (layer F.SilkS) ", strlen (" (layer F.SilkS) ")))
		{
			for (m = 0; (strncmp(buffer_in + m, " (layer ", strlen(" (layer ")) != 0); ++m)
				beginning_of_line[m] = buffer_in[m];
			beginning_of_line[m] = '\0';
			sprintf(buffer_out,"%s (layer F.SilkS) (width 0.15))\n", beginning_of_line);
		}
	for (n = 0; buffer_in[n] != '\n'; ++n)
		if (!strncmp (buffer_in + n, " (layer B.CrtYd) ", strlen (" (layer B.CrtYd) ")))
		{
			for (m = 0; (strncmp(buffer_in + m, " (layer ", strlen(" (layer ")) != 0); ++m)
				beginning_of_line[m] = buffer_in[m];
			beginning_of_line[m] = '\0';
			sprintf(buffer_out,"%s (layer B.CrtYd) (width 0.05))\n", beginning_of_line);
		}
	for (n = 0; buffer_in[n] != '\n'; ++n)
		if (!strncmp (buffer_in + n, " (layer B.SilkS) ", strlen (" (layer B.SilkS) ")))
		{
			for (m = 0; (strncmp(buffer_in + m, " (layer ", strlen(" (layer ")) != 0); ++m)
				beginning_of_line[m] = buffer_in[m];
			beginning_of_line[m] = '\0';
			sprintf(buffer_out,"%s (layer B.SilkS) (width 0.15))\n", beginning_of_line);
		}
	printf ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

int check_unnecessary ()		// returns 1 (if called it must be warning)
{
	return 1;
}

int fix_unnecessary (char *buffer_in, char *buffer_out, int *modified)		// remove (usually unnecessary) lines starting with "  (autoplace_cost", "  (solder_", "  (clearance "
{
	strcpy (buffer_out, "");
	printf ("line:\n%swill be replaced with:\n\n%s", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

int check_3d (char *buffer_in, char *pretty_name, char *footprint_name)		// returns non zero if 3d model is not LIBRARYNAME.3dshapes/FOOTPRINTNAME.wrl
{
	char link_3d[BUFFER_SIZE];
	sprintf(link_3d,"  (model %s.3dshapes/%s.wrl\n", pretty_name, footprint_name);
	return strcmp (buffer_in, link_3d);
}

int fix_3d (char *buffer_in, char *pretty_name, char *footprint_name, char *path_to_models, char *buffer_out, FILE *file_wrl_tmp, FILE *file_wings_tmp, char *path_3d_wrl, char *path_3d_wings, char *path_3d_new_3dshapes,	char *path_3d_new_wrl, char *path_3d_new_wings, char *path_3d_3dshapes, int *wings_exists, int *change_3d, int *modified)
{
	int n;
	char ch;
	int intc;
	char confirm;
	FILE *file_wrl;
	FILE *file_wings;
	char read_3d_path[BUFFER_SIZE];
	strncpy (read_3d_path, buffer_in + strlen ("  (model "), strlen (buffer_in) - strlen ("  (model "));		// read 3d path
	read_3d_path[strlen (buffer_in) - strlen ("  (model ") - 1] = '\0';
	sprintf (path_3d_wrl,"%s/%s", path_to_models, read_3d_path);		// global path to 3d model
	sprintf (path_3d_new_wrl, "%s/%s.3dshapes/%s.wrl", path_to_models, pretty_name, footprint_name);
	sprintf (path_3d_new_3dshapes, "%s/%s.3dshapes", path_to_models, pretty_name);		// new global path
	strcpy(path_3d_wings, path_3d_wrl);		// take also global paths to .wings files
	path_3d_wings[strlen (path_3d_wrl) - strlen (".wrl")] = '\0';
	strcat(path_3d_wings, ".wings");
	sprintf (path_3d_new_wings, "%s/%s.3dshapes/%s.wings", path_to_models, pretty_name, footprint_name);
	strcpy(path_3d_3dshapes, path_3d_wrl);		// take also global paths to .wings files
	for (n = strlen (path_3d_3dshapes); (path_3d_3dshapes[n] != '/') && (n > 0); --n)
	{;}
	path_3d_3dshapes[n] = '\0';		// old global path to 3d .3dshapes

	if (access (path_3d_new_wings, F_OK) && access (path_3d_new_wrl, F_OK))		// 3d path can be changed only if it won't overwrite anything
	{
		if ((file_wrl = fopen (path_3d_wrl, "rb")) != NULL)		// if .wrl linked properly, copy it to temporary file and set new path in .kicad_mod
		{
			while ((ch = fgetc (file_wrl)) != EOF)
				fputc (ch, file_wrl_tmp);
			fclose (file_wrl);
			sprintf (buffer_out, "  (model %s.3dshapes/%s.wrl\n", pretty_name, footprint_name);		// set new path to 3d model in .kicad_mod file
			printf ("line:\n%swill be replaced with:\n%s\n\n", buffer_in, buffer_out);
		}
		else		// error - won't modify if wrong 3d link
		{
			_RED_
			printf ("Couldn't open .wrl file\n");
			*modified = -1;
			return 1;
		}
		if ((file_wings = fopen (path_3d_wings, "rb")) != NULL)		// if .wings corresponding with .wrl exists, copy it to temporary file
		{
			while ((intc = fgetc (file_wings)) != EOF)
				fputc (intc, file_wings_tmp);
			fclose (file_wings);
			*wings_exists = 1;		// set to inform that .wings should be copied from temporary file
		}
		else	// if .wings corresponding with .wrl not exist, ask if it should exist, if it should but not found - error (won't modify)
		{
			_YELLOW_
			printf ("Couldn't open .wings file\n");
			printf ("	does it exist? (enter y for yes or n for no)\n");
			do
				scanf (" %c", &confirm);
			while ((confirm != 'y') && (confirm != 'n'));
			if (confirm == 'y')
			{
				_RED_
				printf ("can't find .wings file\n\n");
				*modified = -1;
				return 1;
			}
			*wings_exists = 0;
		}
		*change_3d = 1;
		if (*modified >= 0)
			*modified += 1;
		return 0;
	}
	_RED_
	printf ("Couldn't rename 3D - file exists in new location\n");
	*modified = -1;
	return 1;
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

	char path_to_libs[MAX_NAME_LENGTH];			// path to directory containing .pretty libraries
	char pretty_path[MAX_NAME_LENGTH];			// path to .pretty library
	char kicad_mod_path[MAX_NAME_LENGTH];		// path to .kicad_mod footprint
	char path_to_models[MAX_NAME_LENGTH];		// path to directory containing .3dshapes directories
	char path_3d_3dshapes[BUFFER_SIZE];			// path to .3dshapes directory
	char path_3d_new_3dshapes[BUFFER_SIZE];		// new path to .3dshapes directory
	char path_3d_wrl[BUFFER_SIZE];				// path to original .wrl file
	char path_3d_new_wrl[MAX_NAME_LENGTH];		// path to new .wrl file
	char path_3d_wings[BUFFER_SIZE];			// path to original .wings file
	char path_3d_new_wings[MAX_NAME_LENGTH];	// path to new .wings file

	char pretty_name[MAX_NAME_LENGTH];			// name of pretty library (directory name without .pretty)
	char footprint_name[MAX_NAME_LENGTH];		// name of footprint (file name without .kicad_mod)
	char buffer_in[BUFFER_SIZE];				// buffer for line read from file
	char buffer_out[BUFFER_SIZE];				// buffer for line to write to file

	int n;										// counter for checking filename
	int m;										// counter for pads and attributes
	int i;										// .pretty counter
	int j;										// .kicad_mod counter
	char ch;									// char variable (for copying files)
	int intc;									// int variable (for copying files)
	int modified;								// set if footprint is modified
	int change_3d;								// set if 3d is modified
	char confirm;								// decide: y for yes, n for no
	int wings_exists;							// set if .wings file exists
	int tht;									// count tht pads
	int smd;									// count smd pads
	int attr;									// normal = 0, insert = 1, virtual = 2

	_CYAN_
	printf ("\nThis software checks some of KiCad Library Convention (KLC) rules and helps with adjusting footprints to the convention\n");
	_RED_
	printf ("\nIT MAY DESTROY YOUR DATA\nPlease make sure you have BACKUP COPY of footprints and 3d models\nand you enter VAILD PATHS\nPlease make sure you understand KLC and keep in mind that not all of\nsuggested changes must be in line with your intentions\n\n");
	_CYAN_
	printf ("\nYou will get information about possible KLC deviations and\n(if you accept) wrong lines will be changed\n");
	_GREEN_
	printf ("Please enter path to directory containing .pretty libraries\n");
	_RED_
	scanf ("%s", path_to_libs);
	_GREEN_
	printf ("Please enter path to directory containing .3dshapes directories with 3d models\n");
	_RED_
	scanf ("%s", path_to_models);
	_RED_
	printf ("\nPlease make sure you have backup copy of your data\nand entered paths are ok (press enter to continue)\n\n");
	getchar ();

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
		_RED_
		printf ("Couldn't open directory containing .pretty libraries");
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
		sprintf(pretty_path,"%s/%s", path_to_libs, pretty_list[i].d_name);
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
			_RED_
			printf ("Couldn't open .pretty library");
			exit (1);
		}


		//HERE OPERATIONS ON FOOTPRINTS AND 3D MODELS ARE DONE
		//---------------------------------------------------------------
		//	do following operations on all footprints in current library
		//---------------------------------------------------------------
		for (j = 0; strcmp(footprint_list[j].d_name, "||\\//||") != 0; ++j)
		{
			sprintf(kicad_mod_path,"%s/%s/%s", path_to_libs, pretty_list[i].d_name, footprint_list[j].d_name);		//prepare path to read original .kicad_mod file
			sprintf(pretty_name,"%.*s", (unsigned int)(strlen(pretty_list[i].d_name) - strlen(".pretty")), pretty_list[i].d_name);	//name of pretty library (directory name without .pretty)
			sprintf(footprint_name,"%.*s", (unsigned int)(strlen(footprint_list[j].d_name) - strlen(".kicad_mod")), footprint_list[j].d_name);	//name of footprint (file name without .kicad_mod)
			modified = 0;
			change_3d = 0;
			tht = 0;
			smd = 0;
			attr = 0;

			for (n = 0; pretty_name[n] != '\0'; ++n)
				if (!( ((pretty_name[n] >= 'a') && (pretty_name[n] <= 'z')) || ((pretty_name[n] >= 'A') && (pretty_name[n] <= 'Z')) || ((pretty_name[n] >= '0') && (pretty_name[n] <= '9')) || (pretty_name[n] == '.') || (pretty_name[n] <= '-') || (pretty_name[n] <= '_') ))
				{
					_RED_
					printf ("bad library name");
					exit (1);
				}
			for (n = 0; footprint_name[n] != '\0'; ++n)
				if (!( ((footprint_name[n] >= 'a') && (footprint_name[n] <= 'z')) || ((footprint_name[n] >= 'A') && (footprint_name[n] <= 'Z')) || ((footprint_name[n] >= '0') && (footprint_name[n] <= '9')) || (footprint_name[n] == '.') || (footprint_name[n] <= '-') || (footprint_name[n] <= '_') ))
				{
					_RED_
					printf ("bad footprint name");
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
								_YELLOW_
								printf ("%s: %s\n	warning: module name not equal to footprint name or footprint locked\n", pretty_name, footprint_name);
								printf ("	fix it? (enter y for yes or n for no)");
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
								_YELLOW_
								printf ("%s: %s\n	warning: comma in tags found\n", pretty_name, footprint_name);
								printf ("	fix it? (enter y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_tags (buffer_in, buffer_out, &modified);
							}
						}

						else if ((strncmp("  (fp_text reference ", buffer_in, strlen("  (fp_text reference ")) == 0))
						{
							if (check_ref (buffer_in))
							{
								_YELLOW_
								printf ("%s: %s\n	warning: reference not equal to REF** or hidden or wrong layer\n", pretty_name, footprint_name);
								printf ("	fix it? (enter y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_ref (buffer_in, buffer_out, &modified);
							}
						}

						else if ((strncmp("  (fp_text value ", buffer_in, strlen("  (fp_text value ")) == 0))
						{
							if (check_val (buffer_in, footprint_name))
							{
								_YELLOW_
								printf ("%s: %s\n	warning: value not equal to footprint name or hidden or wrong layer\n", pretty_name, footprint_name);
								printf ("	fix it? (enter y for yes or n for no)");
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
								_YELLOW_
								printf ("%s: %s\n	warning: text size is not 1 x 1 x 0.15 mm\n", pretty_name, footprint_name);
								printf ("	fix it? (enter y for yes or n for no)");
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
							if (check_line (buffer_in))
							{
								_YELLOW_
								printf ("%s: %s\n	warning: silkscreen line width not equal to 0.15 or courtyard line width not equal to 0.05\n", pretty_name, footprint_name);
								printf ("	fix it? (enter y for yes or n for no)");
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
								_YELLOW_
								printf ("%s: %s\n	warning: unnecessary footprint properties set\n", pretty_name, footprint_name);
								printf ("	fix it? (enter y for yes or n for no)");
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
								_YELLOW_
								printf ("%s: %s\n	warning: 3d model path not equal to footprint path\n", pretty_name, footprint_name);
								printf ("	fix it? (enter y for yes or n for no)");
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
					_RED_
					printf ("Couldn't create temporary files");
				}
				fclose (file_kicad_mod);
			}
			else
			{
				_RED_
				printf ("Couldn't open .kicad_mod file");
			}


			//-----------------------------------------------------------------------------------------
			// If all went OK - apply changes (copy temporary files to .kicad_mod and new .wrl/.wings
			//-----------------------------------------------------------------------------------------
			if (modified > 0)
			{
				_CYAN_
				printf ("\nConfirm to apply changes to:\n	%s: %s\n(enter y for yes or n for no)\n\n", pretty_name, footprint_name);
				do
					scanf (" %c", &confirm);
				while ((confirm != 'y') && (confirm != 'n'));
				if (confirm == 'y')
				{
					if ((file_kicad_mod = fopen(kicad_mod_path, "wb")) != NULL)		// copy .kicad_mod to new location (from temporary file containing fixed footprint)
					{
						rewind (file_kicad_mod_tmp);
						while ((ch = fgetc (file_kicad_mod_tmp)) != EOF)
							fputc (ch, file_kicad_mod);
						fclose (file_kicad_mod);
					}
					else
					{
						_RED_
						printf ("Couldn't create .kicad_mod file\n");
					}

					if (change_3d)
					{
						if (access (path_3d_new_3dshapes, F_OK))		// create directory with proper name if not exists
						{
							#if defined(__MINGW32__)
								mkdir (path_3d_new_3dshapes);
							#else
								mkdir (path_3d_new_3dshapes, 0777);
							#endif
						}
						if ((file_new_wrl = fopen(path_3d_new_wrl, "wb")) != NULL)		// copy .wrl to new location (from temporary file created by function fix_3d)
						{
							rewind (file_wrl_tmp);
							while ((ch = fgetc (file_wrl_tmp)) != EOF)
								fputc (ch, file_new_wrl);
							fclose (file_new_wrl);
							unlink (path_3d_wrl);		// remove old .wrl file
						}
						else
						{
							_RED_
							printf ("Couldn't create .wrl file\n");
						}

						if (((file_new_wings = fopen(path_3d_new_wings, "wb")) != NULL) && wings_exists)		// copy .wings to new location (from temporary file created by function fix_3d)
						{
							rewind (file_wings_tmp);
							while ((intc = fgetc (file_wings_tmp)) != EOF)
								fputc (intc, file_new_wings);
							fclose (file_new_wings);
							unlink (path_3d_wings);		// remove old .wings file
						}
						else
						{
							_RED_
							printf ("Couldn't create .wings file\n");
						}
						rmdir (path_3d_3dshapes);		// remove old .3dshapes directory if empty
					}
				}
			}
			else if (modified < 0)
			{
				_RED_
				printf("\nFAILED!!!\n	%s/%s not changed\n\n", pretty_name, footprint_name);
			}
			fclose (file_kicad_mod_tmp);
			fclose (file_wrl_tmp);
		}
	}

	if ((smd - tht > 0) && (attr == 0))
	{
		_CYAN_
		printf ("\nINFO: attributes are 'normal' but most of pads are SMD - please check attributes\n");
	}
	if ((tht - smd > 0) && (attr > 0))
	{
		_CYAN_
		printf ("\nINFO: attributes are 'normal+insert' but most of pads are THT - please check attributes\n");
	}

	_GREEN_
	printf ("\nCheck finished\n");
	return 0;
}
