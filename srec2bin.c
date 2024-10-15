/*

SREC2BIN - Convert Motorola S-Record to binary file
Copyright (c) 2001-2004  Anthony Goffart

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define HEADER1 "\nSREC2BIN V1.20 - Convert Motorola S-Record to binary file.\n"
#define HEADER2 "Copyright (c) 2002 Ant Goffart - http://www.s-record.com/\n\n"

#define TRUE 1
#define FALSE 0

#define max(a,b) (((a)>(b))?(a):(b))
#define min(a,b) (((a)<(b))?(a):(b))

#define LINE_LEN 1024

typedef unsigned long dword;
typedef unsigned short word;

char *infilename;
char *outfilename;
FILE *infile, *outfile;

dword max_addr = 0;
dword min_addr = 0;

char filler = 0xff;
int verbose = TRUE;

/***************************************************************************/

int ctoh(char c)
{
   int res = 0;

   if (c >= '0' && c <= '9')
      res = (c - '0');
   else if (c >= 'A' && c <= 'F')
      res = (c - 'A' + 10);
   else if (c >= 'a' && c <= 'f')
      res = (c - 'a' + 10);

   return(res);
}

/***************************************************************************/

dword atoh(char *s)
{
   int i;
   char c;
   dword res = 0;

   strupr(s);

   for (i = 0; i < strlen(s); i++)
   {
      c = s[i];
      res <<= 4;
      res += ctoh(c);
   }
   return(res);
}

/***************************************************************************/

dword file_size(FILE *f)
{
   struct stat info;

   if (!fstat(fileno(f), &info))
      return(info.st_size);
   else
      return(0);
}

/***************************************************************************/

void syntax(void)
{
   fprintf(stderr, HEADER1);
   fprintf(stderr, HEADER2);
   fprintf(stderr, "Syntax: SREC2BIN <options> INFILE OUTFILE\n\n");
   fprintf(stderr, "-help            Show this help.\n");
   fprintf(stderr, "-o <offset>      Start address offset (hex), default = 0.\n");
   fprintf(stderr, "-a <addrsize>    Minimum binary file size (hex), default = 0.\n");
   fprintf(stderr, "-f <fillbyte>    Filler byte (hex), default = FF.\n");
   fprintf(stderr, "-q               Quiet mode\n");
}

/***************************************************************************/

void parse(int scan, dword *max, dword *min)
{
   int i, j;
   char line[LINE_LEN] = "";
   dword address;
   int rec_type, addr_bytes, byte_count;
   unsigned char c;
   unsigned char buf[32];

   do
   {
      fgets(line, LINE_LEN, infile);

      if (line[0] == 'S')                               /* an S-record */
      {
         rec_type = line[1] - '0';

         if ((rec_type >= 1) && (rec_type <= 3))        /* data record */
         {
            address = 0;
            addr_bytes = rec_type + 1;

            for (i = 4; i < (addr_bytes * 2) + 4; i++)
            {
               c = line[i];
               address <<= 4;
               address += ctoh(c);
            }

            byte_count = (ctoh(line[2]) << 4) + ctoh(line[3]);

            byte_count -= (addr_bytes + 1);

            if (scan)
            {
               if (*min > address)
                  *min = address;

               if (*max < (address + byte_count))
                  *max = address + byte_count;
            }
            else
            {
               address -= min_addr;

               if (verbose)
                  fprintf(stderr, "Writing %d bytes at %08lX\r", byte_count, address);

               j = 0;
               for (i = (addr_bytes * 2) + 4; i < (addr_bytes * 2) + (byte_count * 2) + 4; i += 2)
               {
                  buf[j] = (ctoh(line[i]) << 4) + ctoh(line[i+1]);
                  j++;
               }
               fseek(outfile, address, SEEK_SET);
               fwrite(buf, 1, byte_count, outfile);
            }
         }
      }
   }
   while(!feof(infile));

   rewind(infile);
   *max -= 1;
}

/***************************************************************************/

void process(void)
{
   dword i;
   dword blocks, remain;
   dword pmax = 0;
   dword pmin = 0xffffffffl;

   unsigned char buf[32];

   if (verbose)
   {
      fprintf(stderr, HEADER1);
      fprintf(stderr, HEADER2);
      fprintf(stderr, "Input Motorola S-Record file: %s\n", infilename);
      fprintf(stderr, "Output binary file: %s\n", outfilename);
   }

   parse(TRUE, &pmax, &pmin);

   min_addr = min(min_addr, pmin);
   max_addr = max(pmax, min_addr + max_addr);

   blocks = (max_addr - min_addr + 1) / 32;
   remain = (max_addr - min_addr + 1) % 32;

   if (verbose)
   {
      fprintf(stderr, "Mimimum address  = %lXh\n", min_addr);
      fprintf(stderr, "Maximum address  = %lXh\n", max_addr);
      i = max_addr - min_addr + 1;
      fprintf(stderr, "Binary file size = %ld (%lXh) bytes.\n", i, i);
   }

   if ((outfile = fopen(outfilename, "wb")) != NULL)
   {
      for (i = 0; i < 32; i++)
         buf[i] = filler;
      for (i = 0; i < blocks; i++)
         fwrite(buf, 1, 32, outfile);
      fwrite(buf, 1, remain, outfile);

      parse(FALSE, &pmax, &pmin);
      fclose(outfile);
   }
   else
   {
      fprintf(stderr, "Cant create output file %s.\n", outfilename);
      return;
   }

   if (verbose)
      fprintf(stderr, "Processing complete          \n");
}

/***************************************************************************/

int main(int argc, char *argv[])
{
   int i;
   char tmp[16] = "";

   for (i = 1; i < argc; i++)
   {
      if (!strcmp(argv[i], "-q"))
      {
         verbose = FALSE;
         continue;
      }

      else if (!strcmp(argv[i], "-a"))
      {
         sscanf(argv[++i], "%s", tmp);
         max_addr = atoh(tmp) - 1;
         continue;
      }

      else if (!strcmp(argv[i], "-o"))
      {
         sscanf(argv[++i], "%s", tmp);
         min_addr = atoh(tmp);
         continue;
      }

      else if (!strcmp(argv[i], "-f"))
      {
         sscanf(argv[++i], "%s", tmp);
         filler = atoh(tmp) & 0xff;
         continue;
      }

      else if (!strncmp(argv[i], "-h", 2))       /* -h or -help */
      {
         syntax();
         return(0);
      }

      else
      {
         infilename = argv[i];
         outfilename = argv[++i];
      }
   }

   if (infilename == NULL)
   {
      syntax();
      fprintf(stderr, "\n** No input filename specified\n");
      return(1);
   }

   if (outfilename == NULL)
   {
      syntax();
      fprintf(stderr, "\n** No output filename specified\n");
      return(1);
   }

   if ((infile = fopen(infilename, "rb")) != NULL)
   {
      process();
      fclose(infile);
      return(0);
   }
   else
   {
      printf("Input file %s not found\n", infilename);
      return(2);
   }
}

/***************************************************************************/
