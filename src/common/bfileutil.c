#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/vfs.h> // statfs

#include "boithohome.h"


// return -1 on error
long long kbytes_left_in_dir(char *dir_path) {
         struct statfs buf;

         if (statfs(dir_path, &buf) == -1) {
		 perror(dir_path);
		 return -1;
	 }
	 return (buf.f_bsize * buf.f_bavail) / 1024; // kb
}


/*
ripped from http://www.opensource.apple.com/darwinsource/tarballs/other/distcc-31.0.81.tar.gz
eks use: mkdir_p("/tmp/aaa/bb/",0755) 
*/
/* -*- c-file-style: "java"; indent-tabs-mode: nil; fill-column: 78 -*-
 *
 * distcc -- A simple distributed compiler system
 *
 * Copyright (C) 2003, 2005 by Apple Computer, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

/**
 * Create <code>path_to_dir</code>, including any intervening directories.
 * Behaves similarly to <code>mkdir -p</code>.
 * Permissions are set to <code>mode</code>.
 *
 * <code>path_to_dir</code> must not be <code>NULL</code>
 **/


int bmkdir_p(const char *path_to_dir,int mode)
{
    char *path = strdup(path_to_dir);
    char current_path[PATH_MAX];
    char *latest_path_element;

    current_path[0] = '\0';
    latest_path_element = strtok(path, "/");

    while ( strlen(current_path) < PATH_MAX - NAME_MAX - 1 ) {
        if ( latest_path_element == NULL ) {
            break;
        } else {
            strcat(current_path, "/");
            strcat(current_path, latest_path_element);

            if ( mkdir(current_path, mode) == 0 ) {
		#ifdef DEBUG
                	fprintf(stderr,"info: Created directory %s\n", current_path);
		#endif
            } else {
                if ( errno == EEXIST ) {
			#ifdef DEBUG
				fprintf(stderr,"info: Directory exists: %s\n", current_path);
			#endif
                } else {
                    fprintf(stderr,"Unable to create directory %s: %s",
                                 current_path, strerror(errno));
                    return 0;
                }
            }

            latest_path_element = strtok(NULL, "/");
        }
    }

    free(path);

    if ( latest_path_element == NULL ) {
        return 1;
    } else {
        return 0;
    }
}

char *sfindductype(char filepath[]) {
        int len = strlen(filepath);
        int i;
        char *cp;
        char *type;

        for(i=len;i>0;i--) {

                if (filepath[i] == '/') {
                        return NULL;
                }
                else if (filepath[i] == '.') {

                        cp = (char *)((int)filepath + i +1);

                        type = malloc(strlen(cp) +1);
                        strcpy(type,cp);

			//s�ker oss til f�rste % eller space for � fjerne de
			if ( ( (cp = strchr(type,'%')) != NULL ) || ((cp = strchr(type,'%')) != NULL)) {
				cp[0] = '\0';
			}

                        return type;
                }
        }
        return NULL;
}


/*
	rekursiv sletting av en mappe. Skal bare selltte den mappen, og undermapper. Skal ikke
	kunne da en dagur og gi den "xx / yy", og den sletter /
*/
int rrmdir(char dir[]) {
        DIR *dirp;
        struct dirent *dp;
	char path[PATH_MAX];

	#ifdef DEBUG
		printf("rrmdir: opening dir \"%s\"\n",dir);
	#endif

	if ((dirp = opendir(dir)) == NULL) {

		//hvis mappen ikke finnes s� er den n� sletter, og vi returnerer at atl er ok, selv om vi ikke gjorde noe
		if (errno == ENOENT) {
			#ifdef DEBUG
				printf("Folder \"%s\" don't exist. Ignoring delete command\n");
			#endif
			return 1;
		}

		perror(dir);
		return 0;
	}

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.') {
                        continue;
                }

		snprintf(path,sizeof(path),"%s/%s",dir,dp->d_name);

		if (dp->d_type == DT_DIR) {
			#ifdef DEBUG 
				printf("rrmdir: dir: %s\n",path);
			#endif
			rrmdir(path);
		} else {
			#ifdef DEBUG
				printf("rrmdir: file: %s\n",path);
			#endif

			/* Slett barnet */
			if (remove(path) != 0) {
				perror(path);
			}
		}
	}	

	closedir(dirp);


	//sletter seg selv
	if (remove(dir) != 0) {
        	perror(dir);
        }

	return 1;
}



int readfile_into_buf( char *filename, char **buf ) {

    FILE	*file = fopen(filename, "r");
    int		i;

    if (!file)
	{
	    fprintf(stderr, "Could not open %s\n", filename);
	    return 0;
	}

    // Get filesize:
    struct stat	fileinfo;
    fstat( fileno( file ), &fileinfo );

    int		size = fileinfo.st_size;
    *buf = (char*)malloc(sizeof(char)*size);

    for (i=0; i<size;)
	{
	    i+= fread( (void*)&((*buf)[i]), sizeof(char), size-i, file );
	}

    fclose(file);

    return size;
}

FILE *stretchfile(FILE *FH,char mode[],char file[], off_t branksize) {

        //stenger ned filen
        fclose(FH);

        //�pner for skriving ogs�
        if ( (FH = fopen(file,"r+b")) == NULL ) {
                return NULL;
        }


        //Stretch the file size to the size.
        if (fseek(FH, branksize +1, SEEK_SET) == -1) {
                perror("Error calling fseek() to 'stretch' the file");
                exit(EXIT_FAILURE);
        }

        /* Something needs to be written at the end of the file to
        * have the file actually have the new size.
        * Just writing an empty string at the current file position will do.
        *
        * Note:
        *  - The current position in the file is at the end of the stretched
        *    file due to the call to lseek().
        *  - An empty string is actually a single '\0' character, so a zero-byte
        *    will be written at the last byte of the file.
        */
        if (fwrite("", 1, 1, FH) != 1) {
                perror("Error writing last byte of the file");
                exit(EXIT_FAILURE);
        }

        fclose(FH);

        //�pner med �nsket mode
        if ( (FH = fopen(file,mode)) == NULL ) {
                return NULL;
        }

        return FH;

}

#ifdef SD_CLOEXEC
//rutiner som gj�r slik at filer ikke arves ved exex og fork(?). Det er et problem med filfiltere i bbdn som
//arver �pne lot filer, og dermed hinder indeksering.
//ukjent om batomicallyopen bli close'et eller fclose't. Hvis fclose blir den da renamet.
int closeAtExexo(int fd) {

        int flags;
        flags = fcntl(fd, F_GETFD);
        if (flags == -1) {
		return 0;
        }
       	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
		return 0;
	}

	return 1;
}

int fcloseAtExexo(FILE *FILEHANDLER) {
	return closeAtExexo(fileno(FILEHANDLER));
}

#endif

