/*
 * ZDA content tester V1.0 - zda_tester.c
 *
 * Copyright (C) 2003,2004 Mirko Roller <mirko@mirkoroller.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Changelog:
 *
 *  21 Aug 2004 - Mirko Roller <mirko@mirkoroller.de>
 *   first release 
 *
 */


// 52 byte header for each file
typedef struct {
	char filename[40];
	int  size_uncompressed;
	int  size_compressed;
	int  start_offset;
} ZDAHEADER;

// 12 bytes index
typedef struct {
	char name[4];
	int  records;
	int  size;
} HEAD;

#include <stdio.h>
#include <zlib.h>
#include <string.h>
#include <stdlib.h>


int main(int argc, char *argv[]) {

		FILE *fp;
		ZDAHEADER *zdaheader; 
		HEAD *head;
		int size,err,loop;
		uLongf dlen;
		unsigned char *filein;
		unsigned char *compressed;
		unsigned char *uncompr;

		if (argc == 1 ) {printf ("zda_tester V1.0\nusage: ./zda_tester zda_with_header.zda \n");exit(0);}

		filein = malloc(5000000);
		uncompr= malloc(5000000);
				
		// Read in the zda_with_header file
		fp=fopen (argv[1],"r");
		size = fread(filein, 1, 5000000, fp);
		fclose(fp);
		
		head = (HEAD*) filein ;
		printf ("ZDA Name    : %s  \n",head->name);
		printf ("ZDA records : %i  \n",head->records);
		printf ("ZDA size    : %i  \n",head->size);
		
		for (loop=0;loop<head->records;loop++) {
		
			zdaheader = (ZDAHEADER*) (filein+12+(52*loop));
			printf ("filename    : %s  \n",zdaheader->filename);    
			printf ("uncompressed: %i  \n",zdaheader->size_uncompressed);
			printf ("compressed  : %i  \n",zdaheader->size_compressed);
			printf ("offset      : %i  \n",zdaheader->start_offset);
		
			compressed = (filein+head->size+(zdaheader->start_offset));
		
			dlen=5000000;
			err = uncompress (uncompr, &dlen,compressed,zdaheader->size_compressed);
			printf ("error       :%i \n",err);
		
			//if (err == 0) {
			//  fp=fopen(zdaheader->filename,"w");
			//  fwrite(uncompr,1,dlen,fp);
			//  fclose(fp);
			//}
		
		}
		
		free (filein);
		free (uncompr);    
								
exit(0);
}
