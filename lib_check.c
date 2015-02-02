#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#define MAX_LIB_NR 500
#define MAX_NAME_LENGTH 500
#define BUFFER_SIZE 2000


int check_name (char *buffer_in, char *footprint_name)
{
	int n;
	char read_module_name[MAX_NAME_LENGTH];
	for (n = 0; (buffer_in[strlen("(module ") + n]) != ' '; ++n)
		read_module_name[n] = buffer_in[strlen("(module ") + n];
	read_module_name[n] = '\0';
	return strcmp (read_module_name, footprint_name);
}

int fix_name (char *buffer_in, char *footprint_name, char *buffer_out, int *modified)
{
	int n;
	int m;
	char rest_of_line[BUFFER_SIZE];
	if (buffer_in[strlen("(module ")] == '"')		//it's possible that this string contains whitespaces, then it in must be between double quotes
	{
		for(n = 0; buffer_in[strlen("(module ") + n + 1] != '"'; ++n)		//must be ...+n+1 to ommit first "
			{;}
		n = n + 2;		//here, after adding 2, strlen("(module ") + n points at whitespace after module name in quotation
	}
	else		//if there are no doble quotes check how many characters has the string, count until whitespace
		for(n = 0; (buffer_in[strlen("(module ") + n]) != ' '; ++n)
			{;}		//if there are no quotation, when for loop ends, strlen("(module ") + n points at whitespace after module name
	for(m = 0; (buffer_in[strlen("(module ") + n + m - 1]) != '\0'; ++m)		//save rest of line, following "(module " and MODULENAME, copy also \0
		rest_of_line[m] = buffer_in[strlen("(module ") + n + m];
	sprintf (buffer_out, "(module %s%s", footprint_name, rest_of_line);
	printf ("line:\n%swill be replaced with:\n%s", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

int check_ref (char *buffer_in, char *footprint_name)
{
	int n;
	char read_reference[MAX_NAME_LENGTH];
	for (n = 0; (buffer_in[strlen("  (fp_text reference ") + n]) != ' '; ++n)
		read_reference[n] = buffer_in[strlen("  (fp_text reference ") + n];
	read_reference[n] = '\0';
	return strcmp (read_reference, footprint_name);
}

int fix_ref (char *buffer_in, char *footprint_name, char *buffer_out, int *modified)
{
	int n;
	int m;
	char rest_of_line[BUFFER_SIZE];
	if (buffer_in[strlen("  (fp_text reference ")] == '"')
	{
		for(n = 0; buffer_in[strlen("  (fp_text reference ") + n + 1] != '"'; ++n)
			{;}
		n = n + 2;
	}
	else
		for(n = 0; (buffer_in[strlen("  (fp_text reference ") + n]) != ' '; ++n)
			{;}
	for(m = 0; (buffer_in[strlen("  (fp_text reference ") + n + m - 1]) != '\0'; ++m)
		rest_of_line[m] = buffer_in[strlen("  (fp_text reference ") + n + m];
	sprintf (buffer_out, "  (fp_text reference %s%s", footprint_name, rest_of_line);
	printf ("line:\n%swill be replaced with:\n%s", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

int check_val (char *buffer_in)
{
	int n;
	char read_value[MAX_NAME_LENGTH];
	for (n = 0; (buffer_in[strlen("  (fp_text value ") + n]) != ' '; ++n)
		read_value[n] = buffer_in[strlen("  (fp_text value ") + n];
	read_value[n] = '\0';
	return strcmp (read_value, "VAL**");
}

int fix_val (char *buffer_in, char *buffer_out, int *modified)
{
	int n;
	int m;
	char rest_of_line[BUFFER_SIZE];
	if (buffer_in[strlen("  (fp_text value ")] == '"')
	{
		for(n = 0; buffer_in[strlen("  (fp_text value ") + n + 1] != '"'; ++n)
			{;}
		n = n + 2;
	}
	else
		for(n = 0; (buffer_in[strlen("  (fp_text value ") + n]) != ' '; ++n)
			{;}
	for(m = 0; (buffer_in[strlen("  (fp_text value ") + n + m - 1]) != '\0'; ++m)
		rest_of_line[m] = buffer_in[strlen("  (fp_text value ") + n + m];
	sprintf (buffer_out, "  (fp_text value %s%s", "VAL**", rest_of_line);
	printf ("line:\n%swill be replaced with:\n%s", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

int check_font (char *buffer_in)
{
	return (strcmp (buffer_in, "    (effects (font (size 1 1) (thickness 0.15)))\n") != 0);
}

int fix_font (char *buffer_in, char *buffer_out, int *modified)
{
	sprintf (buffer_out, "    (effects (font (size 1 1) (thickness 0.15)))\n");
	printf ("line:\n%swill be replaced with:\n%s", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

int check_crtyd_line (char *buffer_in)
{
	int n;
	int m;
	char read_courtyard_width[BUFFER_SIZE];
	strcpy (read_courtyard_width, "0.05");
	for (m = 0; (strncmp(buffer_in + m, " (layer F.CrtYd) (width ", strlen(" (layer F.CrtYd) (width ")) != 0) && m < strlen(buffer_in); ++m)
		{;}
	if (m < strlen(buffer_in))
	{
		for (n = 0; (buffer_in[strlen(" (layer F.CrtYd) (width ") + m + n]) != ')'; ++n)
			read_courtyard_width[n] = buffer_in[strlen(" (layer F.CrtYd) (width ") + m + n];
		read_courtyard_width[n] = '\0';
	}
	return strcmp (read_courtyard_width, "0.05");
}

int fix_crtyd_line (char *buffer_in, char *buffer_out, int *modified)
{
	int n;
	char beginning_of_line[BUFFER_SIZE];
	for (n = 0; (strncmp(buffer_in + n, " (layer F.CrtYd) (width ", strlen(" (layer F.CrtYd) (width ")) != 0); ++n)
		beginning_of_line[n] = buffer_in[n];
	beginning_of_line[n] = '\0';
	sprintf(buffer_out,"%s (layer F.CrtYd) (width 0.05))\n", beginning_of_line);
	printf ("line:\n%swill be replaced with:\n%s", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

int check_silks_line (char *buffer_in)
{
	int n;
	int m;
	char read_silks_width[BUFFER_SIZE];
	strcpy (read_silks_width, "0.15");
	for (m = 0; (strncmp(buffer_in + m, " (layer F.SilkS) (width ", strlen(" (layer F.SilkS) (width ")) != 0) && m < strlen(buffer_in); ++m)
		{;}
	if (m < strlen(buffer_in))
	{
		for (n = 0; (buffer_in[strlen(" (layer F.SilkS) (width ") + m + n]) != ')'; ++n)
			read_silks_width[n] = buffer_in[strlen(" (layer F.SilkS) (width ") + m + n];
		read_silks_width[n] = '\0';
	}
	return strcmp (read_silks_width, "0.15");
}

int fix_silks_line (char *buffer_in, char *buffer_out, int *modified)
{
	int n;
	char beginning_of_line[BUFFER_SIZE];
	for (n = 0; (strncmp(buffer_in + n, " (layer F.SilkS) (width ", strlen(" (layer F.SilkS) (width ")) != 0); ++n)
		beginning_of_line[n] = buffer_in[n];
	beginning_of_line[n] = '\0';
	sprintf(buffer_out,"%s (layer F.SilkS) (width 0.15))\n", beginning_of_line);
	printf ("line:\n%swill be replaced with:\n%s", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

int check_unnecessary ()
{
	return 1;
}

int fix_unnecessary (char *buffer_in, char *buffer_out, int *modified)
{
	strcpy (buffer_out, "");
	printf ("line:\n%swill be replaced with:\n\n%s", buffer_in, buffer_out);
	if (*modified >= 0)
		*modified += 1;
	return 0;
}

int check_3d (char *buffer_in, char *pretty_name, char *footprint_name)
{
	char link_3d[BUFFER_SIZE];
	sprintf(link_3d,"  (model %s/%s.wrl\n", pretty_name, footprint_name);
	return strcmp (buffer_in, link_3d);
}

int fix_3d (char *buffer_in, char *pretty_name, char *footprint_name, char *path_to_models, char *buffer_out, int *modified)
{
	char rest_of_line[BUFFER_SIZE];
	char path_3d_wrl[BUFFER_SIZE];
	char path_3d_new_wrl[BUFFER_SIZE];
	char path_3d_wings[BUFFER_SIZE];
	char path_3d_new_wings[BUFFER_SIZE];
	strncpy (rest_of_line, buffer_in + sizeof ("  (model ") - 1, strlen (buffer_in) - sizeof ("  (model "));
	sprintf(path_3d_wrl,"%s/%s", path_to_models, rest_of_line);
	sprintf (path_3d_new_wrl, "%s/%s/%s.wrl", path_to_models, pretty_name, footprint_name);
	strncpy(path_3d_wings, path_3d_wrl, strlen (path_3d_wrl) - strlen (".wrl"));
	strcat(path_3d_wings, ".wings");
	sprintf (path_3d_new_wings, "%s/%s/%s.wings", path_to_models, pretty_name, footprint_name);
	if (access (path_3d_new_wrl, F_OK) || access (path_3d_new_wings, F_OK))
	{

		if (! rename (path_3d_wrl, path_3d_new_wrl))
		{

			rename (path_3d_wings, path_3d_new_wings);
			sprintf (buffer_out, "  (model %s/%s.wrl\n", pretty_name, footprint_name);
			printf ("line:\n%swill be replaced with:\n%s", buffer_in, buffer_out);
			if (*modified >= 0)
				*modified += 1;
			return 0;
		}
		else
		{
			perror ("Couldn't rename");
			*modified = -1;
			return 1;
		}
	}
	else
	{
		perror ("Couldn't rename");
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

	struct dirent *ep;							// structure containing name of file got by readdir
	struct dirent pretty_list[MAX_LIB_NR];		// array containing list of .pretty libraries
	struct dirent footprint_list[MAX_LIB_NR];	// array containing list of .kicad_mod footprints

	char pretty_path[MAX_NAME_LENGTH];			// path to .pretty library
	char kicad_mod_path[MAX_NAME_LENGTH];		// path to .kicad_mod footprint

	char path_to_libs[MAX_NAME_LENGTH];			// path to folder containing .pretty libraries
	char path_to_models[MAX_NAME_LENGTH];		// path to folder containing 3d models

	int i;										// .pretty counter
	int j;										// .kicad_mod counter
	char ch;									// char variable (for copying files)
	int modified;								// set if footprint is modified
	char confirm;								// decide: y for yes, n for no
	char buffer_in[BUFFER_SIZE];				// buffer for line read from file
	char buffer_out[BUFFER_SIZE];				// buffer for line to write to file

	char pretty_name[MAX_NAME_LENGTH];			//name of pretty library (directory name without .pretty)
	char footprint_name[MAX_NAME_LENGTH];		//name of footprint (file name without .kicad_mod)



	printf ("Give path to folder containing .pretty libraries\n");
	scanf ("%s", path_to_libs);
	printf ("Give path to folder containing 3d models\n");
	scanf ("%s", path_to_models);

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
		perror ("Couldn't open directory containing .pretty libraries");
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
			perror ("Couldn't open .pretty library");
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

			if ((file_kicad_mod = fopen(kicad_mod_path, "rt")) != NULL)		//open footprint to edit
			{
				if ((file_kicad_mod_tmp = tmpfile ()) != NULL)		//create temporary file - write to it and next copy to footprint
				{
					//HERE .kicad_mod FOOTPRINTS ARE CHECKED
					for (fgets (buffer_in, BUFFER_SIZE, file_kicad_mod); !feof (file_kicad_mod); fgets (buffer_in, BUFFER_SIZE, file_kicad_mod))		//read lines of .kicad_mod file until reaches EOF (must be \n before EOF)
					{
						strcpy (buffer_out, buffer_in);
//------------------------------------------------------
						if ((strncmp("(module ", buffer_in, strlen("(module ")) == 0))
						{
							if (check_name (buffer_in, footprint_name))
							{
								printf ("%s: %s\n	warning: module name not equal to footprint name\n", pretty_name, footprint_name);
								printf ("	fix it? (press y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_name (buffer_in, footprint_name, buffer_out, &modified);
							}
						}
//-------
						else if ((strncmp("  (fp_text reference ", buffer_in, strlen("  (fp_text reference ")) == 0))
						{
							if (check_ref (buffer_in, footprint_name))
							{
								printf ("%s: %s\n	warning: reference not equal to footprint name\n", pretty_name, footprint_name);
								printf ("	fix it? (press y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_ref (buffer_in, footprint_name, buffer_out, &modified);
							}
						}

						else if ((strncmp("  (fp_text value ", buffer_in, strlen("  (fp_text value ")) == 0))
						{
							if (check_val (buffer_in))
							{
								printf ("%s: %s\n	warning: value not equal to VAL**\n", pretty_name, footprint_name);
								printf ("	fix it? (press y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_val (buffer_in, buffer_out, &modified);
							}
						}
//-------
						else if (strncmp("    (effects (font ", buffer_in, strlen("    (effects (font ")) == 0)
						{
							if (check_font (buffer_in))
							{
								printf ("%s: %s\n	warning: text size is not 1 x 1 x 0.15 mm\n", pretty_name, footprint_name);
								printf ("	fix it? (press y for yes or n for no)");
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
							if (check_crtyd_line (buffer_in))
							{
								printf ("%s: %s\n	warning: courtyard line width not equal to 0.05\n", pretty_name, footprint_name);
								printf ("	fix it? (press y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_crtyd_line (buffer_in, buffer_out, &modified);
							}
						}
//-------
						else if ((strncmp("  (fp_line ", buffer_in, strlen("  (fp_line ")) == 0) || (strncmp("  (fp_circle ", buffer_in, strlen("  (fp_circle ")) == 0) || (strncmp("  (fp_arc ", buffer_in, strlen("  (fp_arc ")) == 0) || (strncmp("  (fp_poly ", buffer_in, strlen("  (fp_poly ")) == 0) || (strncmp("  (fp_curve ", buffer_in, strlen("  (fp_curve ")) == 0))
						{
							if (check_silks_line (buffer_in))
							{
								printf ("%s: %s\n	warning: silkscreen line width not equal to 0.15\n", pretty_name, footprint_name);
								printf ("	fix it? (press y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_silks_line (buffer_in, buffer_out, &modified);
							}
						}

						//-------
						else if ((strncmp("  (autoplace_cost", buffer_in, strlen("  (autoplace_cost")) == 0) || (strncmp("  (solder_", buffer_in, strlen("  (solder_")) == 0) || (strncmp("  (clearance ", buffer_in, strlen("  (clearance ")) == 0))
						{
							if (check_unnecessary ())
							{
								printf ("%s: %s\n	warning: unnecessary footprint properties set\n", pretty_name, footprint_name);
								printf ("	fix it? (press y for yes or n for no)");
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
								printf ("%s: %s\n	warning: 3d model path not equal to footprint path\n", pretty_name, footprint_name);
								printf ("	fix it? (press y for yes or n for no)");
								do
									scanf (" %c", &confirm);
								while ((confirm != 'y') && (confirm != 'n'));
								if (confirm == 'y')
									fix_3d (buffer_in, pretty_name, footprint_name, path_to_models, buffer_out, &modified);
							}
						}
//------------------------------------------------------
						fprintf (file_kicad_mod_tmp, "%s", buffer_out);
					}
				}
				else
					perror ("Couldn't create temporary .kicad_mod file");
				fclose (file_kicad_mod);
			}
			else
				perror ("Couldn't open .kicad_mod file");


			if (modified > 0)
			{
				printf ("\nConfirm to apply changes to:\n	%s: %s\n(press y for yes or n for no)\n", pretty_name, footprint_name);
				do
					scanf (" %c", &confirm);
				while ((confirm != 'y') && (confirm != 'n'));
				if (confirm == 'y')
				{
					if ((file_kicad_mod = fopen(kicad_mod_path, "wt")) != NULL)
					{
						rewind (file_kicad_mod_tmp);
						while ((ch = fgetc (file_kicad_mod_tmp)) != EOF)
							fputc (ch, file_kicad_mod);
						fclose (file_kicad_mod);
					}
					else
						perror ("Couldn't open .kicad_mod file");
				}
			}
			else if (modified < 0)
				printf("\nFAILED!!!\n	%s/%s not changed", pretty_name, footprint_name);
			fclose (file_kicad_mod_tmp);
		}
	}

	printf ("Check finished");
	return 0;
}
