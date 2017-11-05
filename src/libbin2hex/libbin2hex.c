/***************************************************************
**                                                            **
**                     libbin2hex.c                           **
**                                                            **
**    A library to print out the hexadecimal equivalent of    **
** a character array representing any binary bytes.           **
**                                                            **
**                Copyright Paul Tew 2017                     **
**                                                            **
** Exported Functions:                                        **
** -------------------                                        **
** int bin2hex(                                               **
**             unsigned char * byte_array,                    **
**             unsigned int    size,                          **
**             unsigned int    gap,                           **
**             unsigned int    cols,                          **
**             unsigned int    margin,                        **
**             unsigned int    ansi                           **
**            )                                               **
**      Returns 0 if the byte array is converted successfully **
**      -1 if not.                                            **
**                                                            **
** byte_array   an array of binary bytes with values          **
**              0 <= value >= 255                             **
**                                                            **
** size         size of the byte array. Max = 4294967295      **
**                                                            **
** gap          == 0 = no gap between hexadecimal bytes       **
**              > 0 = a gap of 1 space between output bytes   **
**                                                            **
** cols         number of columns in hex output. Max = 1024   **
**                                                            **
** margin       number of padding spaces to add before each   **
**              output line. Max = 1024                       **
**                                                            **
** ansi         == 0 = no ansi output                         **
**              > 0 = output printable ansi characters after  **
**                    any hex output                          **
**                                                            **
** header      == 0 = no header                               **
**             >0 = print a header so long as cols % 8        **
**                                                            **
***************************************************************/

/*
This file is part of libbin2hex.

libbin2hex is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

libbin2hex is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with bin2hex.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "./libbin2hex.h"


extern int bin2hex(unsigned char * byte_array, unsigned int size, unsigned int gap, unsigned int cols, unsigned int margin, unsigned int ansi, unsigned int hdr)
{
  unsigned int i, j, stringlen = 0, line = 0, numlines = 0, charsinlastline = 0, spaces = 0;
  char *string, *pad, hex[4], offset[10], printchar[2], ansistr[34];

  // Ensure that parameters are within boundaries
#if defined NDEBUG // If NDEBUG is defined then use assert.h to check parameters
  assert(gap < 2);
  assert(cols < 1025);
  assert(margin < 1025);
  assert(size < 4294967296);
#endif // NDEBUG
#if !defined NDEBUG // If assert.h is not being used then print an error message & quit anyway
  if ((cols > 1024) | (margin > 1024))
  {
    fprintf(stderr, "\nERROR: bin2hex() function called with columns or margin > 1024\n");
    return -1;
  }
  if (gap > 1)
  {
    fprintf(stderr, "\nERROR: bin2hex() function called with gap > 1\n");
  }
  if (size > 4294967295)
  {
    fprintf(stderr, "\nERROR: bin2hex() function called with size > 4,294,967,295\n");
    return -1;
  }
#endif

  stringlen = (gap * cols) + (cols * 2) + margin + (ansi * cols) + 0x20; // determine max length of a line
                                                                         // with (quite) a bit left over for a terminating 0x00 and other stuff
  numlines = size / cols;
  charsinlastline = size % cols;

  string = malloc(stringlen);
  if (string == NULL)
  {
    fprintf(stderr, "\nERROR: bin2hex() function unable to allocate memory\n");
    return -1;
  }

  if ((cols < 33) & (cols % 8 == 0) & (gap > 0) & (hdr > 0))
  {
    // Build the header string
    snprintf(string, 10 + margin, "%*sOFFSET%*s", margin, "", 3, "");
    for (j = 0; j < cols; j++)
    {
      snprintf(hex, 4, "%.2"PRIX8" ", j);
      strcat(string, hex);
    }
    strcat(string, " ANSI\n");
    // Print the header
    printf("%s", string);
    // Now underline the header
    printf("%*s", margin, "");
    for (j = 0; j < (int)strlen(string); j++)
    {
      printf("-");
    }
    printf("\n");
  }

  while (line < numlines) // line is the number of the whole line we are working on (0 based)
  {
    snprintf(string, margin + 1, "%*s", margin, "");
    if (hdr > 0)
    {
      snprintf(offset, 10, "%.8"PRIX32" ", line * cols);
      strcat(string, offset);
    }
    if (ansi == 1)
    {
      ansistr[0] = 0x20; //Reset ansistr to a single space string
      ansistr[1] = 0x00;
    }
    for(i = 0; i < cols; i++)
    {
      snprintf(hex, 3, "%.2"PRIX8, byte_array[(line * cols) + i]);
      if(gap == 1)
      {
        strcat(hex, " ");
      }
      strcat(string, hex);
      if(ansi == 1)
      {
        if((byte_array[(line * cols) + i] > 0x1F) & (byte_array[(line * cols) + i] < 0x80))
        {
          snprintf(printchar, 2, "%c", byte_array[(line * cols) + i]);
        }
        else
        {
          snprintf(printchar, 2, ".");
        }
        strcat(ansistr, printchar);
      }
    }
    strcat(string, ansistr);
    strcat(string, "\n");
    printf("%s", string);
    line++;
  }
  // Print the last line
  snprintf(string, margin + 1, "%*s", margin, "");
  if (hdr == 1)
  {
    snprintf(offset, 10, "%.8"PRIX32" ", line * cols); // line has been incrmented to the last line
    strcat(string, offset);
  }
  if (ansi == 1)
  {
    ansistr[0] = 0x20; //Reset ansistr to a single space string
    ansistr[1] = 0x00;
  }
  for ( i = 0; i < charsinlastline; i++)
  {
    snprintf(hex, 3, "%.2"PRIX8, byte_array[(line * cols) + i]);
    if (gap == 1)
    {
      strcat(hex, " ");
    }
    strcat(string, hex);
    if (ansi == 1)
    {
      if ((byte_array[(line * cols) + i] > 0x1F) & (byte_array[(line * cols) + i] < 0x80))
      {
        snprintf(printchar, 2, "%c", byte_array[(line * cols) + i]);
      }
      else
      {
        snprintf(printchar, 2, ".");
      }
      strcat(ansistr, printchar);
    }
  }
  spaces = ((cols - charsinlastline) * (2 + gap));
  pad = malloc(spaces + 2);
  if (pad == NULL)
  {
    fprintf(stderr, "\nERROR: bin2hex() function unable to allocate memory\n");
    return -1;
  }
  else
  {
    snprintf(pad, spaces + 1, "%*s", spaces, "");
  }
  strcat(string, pad);
  strcat(string, ansistr);
  printf("%s", string);
  printf("\n");
  free(pad);
  free(string);
  return 0;
}
