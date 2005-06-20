/*
 * ZDA compressor V1.0 - zda_compressor.c
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
int headerpos=0;
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

    FILE *fp,*fp2;
    ZDAHEADER zdaheader[256]; // space for 256 files
    HEAD head;
    
    unsigned char *compr, *filein;
    int err,pos=0,size,loop;

    if (argc == 1 ) {printf ("zda_compressor V1.0\nusage: ./zda_compressor file1 file2 file3 ... \n");exit(0);}

    memset (zdaheader,0,256*52); //clear all headers
    filein = malloc(5000000);
    compr  = malloc(5000000);
        
    printf ("filename,normal size,crunched size,offset \n");
    
    fp2=fopen("data.zda","w");
    for (loop=1;loop<argc;loop++) {
    
       uLong comprLen   = 5000000;
       fp=fopen (argv[loop],"r");
       size = fread(filein, 1, 5000000, fp);
       fclose(fp);
      
       err = compress(compr, &comprLen, filein, size);
    
       // comprLen lenght of compressed data
       // compr    compressed data
    
       //fp2=fopen("data.zda","w");
       fwrite(compr,1,comprLen,fp2);
       //fclose(fp2);

       printf ("%s,%d,%ld,%d \n",argv[loop],size,comprLen,pos);
       if (strlen(argv[loop])>39) argv[loop][39]=0;
       strcpy (zdaheader[headerpos].filename,argv[loop]);
       zdaheader[headerpos  ].size_uncompressed=size;
       zdaheader[headerpos  ].size_compressed  =comprLen;
       zdaheader[headerpos++].start_offset     =pos;

       pos+=comprLen;    
    
    }
    fclose(fp2);
    free (filein);
    free (compr);
    strcpy (head.name,"ZDA");
    head.records = headerpos;
    head.size = (headerpos*52)+12;

    // writing new file with header
    fp2=fopen("data_with_header.zda","w");
    fwrite(&head,12,1,fp2);
    fwrite(zdaheader,52,headerpos,fp2);
       fp=fopen ("data.zda","r");
       size=1;
       while (size) {
         char byte[1];
         size = fread(byte, 1, 1, fp);
         fwrite(byte,size,1,fp2);
       }

    fclose(fp);
    fclose(fp2);
    
     
exit(0);
}
