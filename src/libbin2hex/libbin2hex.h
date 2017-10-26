/***************************************************************
**                                                            **
**                     libbin2hex.h                          **
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
** size         size of the byte array                        **
**                                                            **
** gap          == 0 = no gap between hexadecimal bytes       **
**              > 0 = a gap of 1 space between output bytes   **
**                                                            **
** cols         number of columns in hex output.  Max = 1024  **
**                                                            **
** margin       number of padding spaces to add before each   **
**              output line.  Max = 1024                      **
**                                                            **
** ansi         == 0 = no ansi output                         **
**              > 0 = output printable ansi characters after  **
**                    any hex output                          **
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
along with libbin2hex.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _LIBBIN2HEX_H
#define _LIBBIN2HEX_H

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>

extern int bin2hex(unsigned char *, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);

#endif
