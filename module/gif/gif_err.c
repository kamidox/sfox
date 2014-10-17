/*****************************************************************************
 *   "Gif-Lib" - Yet another gif library.                     
 *                                         
 * Written by:  Gershon Elber            IBM PC Ver 0.1,    Jun. 1989    
 *****************************************************************************
 * Handle error reporting for the GIF library.                     
 *****************************************************************************
 * History:                                     
 * 17 Jun 89 - Version 1.0 by Gershon Elber.                     
 ****************************************************************************/

#include "gif_lib.h"

int _GifError = 0;

/*****************************************************************************
 * Return the last GIF error (0 if none) and reset the error.             
 ****************************************************************************/
int
GifLastError(void) {
    int i = _GifError;

    _GifError = 0;

    return i;
}

