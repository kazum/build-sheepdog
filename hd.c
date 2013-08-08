/****************************************************************************
 *                                                                          *
 * Copyright (C) 1997-2007 A.J.Dunk.                                        *
 *                                                                          *
 * This program is free software. It may be modified and                    *
 * redistributed but must not be sold for profit.                           *
 * This notice must be present in all modified versions.                    *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************/
/* HISTORY                                                                  */
/****************************************************************************/
/* DATE      AUTHOR           COMMENT                                       */
/*--------------------------------------------------------------------------*/
/* 1997      Anthony Dunk     Initial version                               */
/* 1998      Anthony Dunk     Wrapped up multiple hex tools in one          */
/* 19/02/03  Anthony Dunk     Updated USAGE info and fixing a few bugs      */
/* 9/9/04    Carl Eric Codere Improved -c option			    */
/* 1/11/04   Anthony Dunk     Added -diff option                            */
/* 12/9/07   Anthony Dunk     Added -e option for EBCDIC                    */
/*                                                                          */
/****************************************************************************/

/* hd.c          13/2/97  AJD

   This program reads the given file (or stdin) and outputs it as hexadecimal
   bytes and ascii strings (un-printable characters are displayed as dots).

   Usage:  hd [-d|-c|-e] [filename]                      - Dump binary file in ascii hex
           hd -f string [filename]			 - Find string in binary file
           hd -a                                         - Convert ascii hex to Binary
                                                           (assumes address + 16 bytes per line)
           hd -x[8|16|32]                                - Convert 8, 16, or 32 bit ascii hex to
                                                           Binary (assumes one byte/word per line)
           hd -diff filename1 filename2 [n]              - Compare files for differences. n is the
                                                           optional number of bytes which must be
                                                           identical for a difference stretch to end
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAX_LINE_LEN   256

/* EBCDIC to ASCII table */
static unsigned char ebcasc[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

	0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x5b,0x2e,0x3c,0x28,0x2b,0x21,
	0x26,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x5d,0x24,0x2a,0x29,0x3b,0x5e,
	0x2d,0x2f,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x2c,0x25,0x5f,0x3e,0x3f,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x60,0x3a,0x23,0x40,0x27,0x3d,0x22,

	0x00,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
	0x68,0x69,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,
	0x71,0x72,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x73,0x74,0x75,0x76,0x77,0x78,
	0x79,0x7a,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

	0x7b,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
	0x48,0x49,0x00,0x00,0x00,0x00,0x00,0x00,
	0x7d,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,
	0x51,0x52,0x00,0x00,0x00,0x00,0x00,0x00,
	0x5c,0x00,0x53,0x54,0x55,0x56,0x57,0x58,
	0x59,0x5a,0x00,0x00,0x00,0x00,0x00,0x00,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
	0x38,0x39,0x00,0x00,0x00,0x00,0x00,0x00
};


#define BYTES_PER_LINE 16
#define UNPRINTABLE "."

main (argc,argv)
    int argc;
    char* argv[];
{
    char  filename [MAX_LINE_LEN];
    FILE  *file;
    char  bytes[BYTES_PER_LINE];
    char  pre_bytes[BYTES_PER_LINE];
    int   i, j;
    int   filename_provided=0;
    int   filesize;
    int   bytes_read=0;
    long  pos=0L;
    int   data_only=0;
    int   C_style=0;
    int   nEBCDIC = 0;

    if (argc>5 || (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"-?"))))
   {
      fprintf(stderr,"\n");
      fprintf(stderr, "Hex Dump Utility (1997-2004)\n");
      fprintf(stderr, "----------------------------\n\n");
      fprintf(stderr, "Written by Anthony Dunk\n\n");
      fprintf(stderr,"\n");
      fprintf(stderr,"       OPTIONS:\n");
      fprintf(stderr,"           -d: Supresses the line addresses.\n");
      fprintf(stderr,"           -c: Outputs the data in a form suitable for C programs.\n");
      fprintf(stderr,"           -e: Convert EBCDIC characters to ASCII.\n");
      fprintf(stderr,"           -f: Searches for all occurrences of the string.\n");
      fprintf(stderr,"               The string can be a quoted or unquoted string of\n");
      fprintf(stderr,"               characters or a sequence of hex bytes of the form\n");
      fprintf(stderr,"               \\x41\\x0D\\x0A\n");
      fprintf(stderr,"           -a: Converts a hex dump back into a binary file. This\n");
      fprintf(stderr,"               allows you to modify binary files using a text editor.\n");
      fprintf(stderr,"           -x: Reads a series of 8, 16, or 32 bit hex numbers in (one per line)\n");
      fprintf(stderr,"               and outputs the numbers as binary data.\n");
      fprintf(stderr,"\n");
        return 1;
    }

    /* No special options, so do a hex dump */

    if (argc==2)
    {
        if (!strcmp(argv[1],"-e")) nEBCDIC=1;
        else if (!strcmp(argv[1],"-d") || !strcmp(argv[1],"-c"))
        {
             data_only=1;
             if (!strcmp(argv[1],"-c")) C_style=1;
        }
        else {
            /* If filename given, store it. */
            strcpy(filename,argv[1]);
            filename_provided=1;
        }
    }
    else if (argc==3)
         {
                if (!strcmp(argv[1],"-e")) nEBCDIC=1;
                if (!strcmp(argv[1],"-d") || !strcmp(argv[1],"-c"))
                {
                     data_only=1;
                     if (!strcmp(argv[1],"-c")) C_style=1;
                }

        /* If filename given, store it. */
        strcpy(filename,argv[2]);
        filename_provided=1;
    }


    /* Open the input file */
    if (filename_provided)
        file=fopen(filename,"rb");
    else
        file=stdin;

    if (file==NULL)
    {
        fprintf(stderr,"%s: Could not open input file.\n",argv[0]);
        return 1;
    }
    if (file != stdin)
    {
      /* If the input filename is not stdin, then get the size of
         the file. */
      if (fseek(file,0,SEEK_END)!=0)
      {
          fprintf(stderr,"%s: Could not seek in input file.\n",argv[0]);
          return 1;
      }
      filesize = ftell(file);
      fseek(file,0,SEEK_SET);

      if (C_style)
      {
         printf("#define DATA_SIZE  %d\n",filesize);
         printf("const char data[DATA_SIZE] = {\n");
      }
    }
    while (!feof(file))
    {
	    static int skip = 0;
        bytes_read=fread(bytes,1,BYTES_PER_LINE,file);
	if (pos > 0) {
		if (memcmp(bytes, pre_bytes, BYTES_PER_LINE) == 0) {
			if (skip == 0)
				printf("*\n");
			pos+=bytes_read;
			memcpy(pre_bytes, bytes, BYTES_PER_LINE);
			skip = 1;
			continue;
		}
	}
	skip = 0;
        if (!data_only) {printf("%08lx  ",pos);}
        pos+=bytes_read;
	memcpy(pre_bytes, bytes, BYTES_PER_LINE);
        for (i=0; i<BYTES_PER_LINE; i++)
        {
            /* If the value is stdin, we require special processing */
            if (file == stdin)
            {
               if (i<bytes_read)
               {
               if (C_style) {
                  printf("0x%02x,",bytes[i] & 0xff);
               }
               else {
                  printf("%02x ",bytes[i] & 0xff);
               }
               }
               else
                   break;
            } else
            {
               if ((i<bytes_read) && ((filesize-1) != (pos-bytes_read+i)))
               {
                  if (C_style) {
                     printf("0x%02x,",bytes[i] & 0xff);
                  }
                  else {
                     printf("%02x ",bytes[i] & 0xff);
                  }
               }
               else
               if ((i<bytes_read) && ((filesize-1) == (pos-bytes_read+i)))
               {
                  if (C_style) {
                     printf("0x%02x};",bytes[i] & 0xff);
                  }
                  else {
                     printf("%02x ",bytes[i] & 0xff);
                  }
               } else
               {
                 break;
               }
            }

         if (!C_style) {
            if (i+1==(BYTES_PER_LINE/2)) printf(" ");
         }
        }

        /* Pad out rest of line (if necessary). */
        for (j=i; j<BYTES_PER_LINE; j++)
        {
            printf("   ");
            if (j+1==(BYTES_PER_LINE/2)) printf(" ");
        }

        if (!data_only) {

            /* Display ascii output. */
            printf(" |");
            for (j=0; j<i; j++)
            {
                    if (nEBCDIC==1)
		    {
                       int c = ebcasc[(unsigned char)bytes[j]];
                       if (c!=0)
                          printf("%c",c);
                       else
                  	  printf(UNPRINTABLE);
                    }
                    else
                    {
                        if (bytes[j]>=' ' && bytes[j]<0x7F)
                          printf("%c",bytes[j]);
                        else
                  	  printf(UNPRINTABLE);
                    }
            }
            printf("|");
        }
        printf("\n");
    }

    if (!data_only) {printf("%08lx\n",pos);}
    if (filename_provided)
        fclose(file);
    return 0;
}

