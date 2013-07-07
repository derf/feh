/* exif_nikon.h

Copyright (C) 2012      Dennis Real.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies of the Software and its documentation and acknowledgment shall be
given in the documentation and software packages that this Software was
used.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef EXIF_NIKON_H
#define EXIF_NIKON_H

/* Flash control mode */
/* http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/Nikon.html#FlashControlMode */
#define EXN_FLASH_CONTROL_MODES_MAX 9
char *EXN_NikonFlashControlModeValues[EXN_FLASH_CONTROL_MODES_MAX] = {"Off", 
                                       "iTTL-BL", "iTTL", "Auto Aperture", 
                                       "Automatic", "GN (distance priority)", 
                                       "Manual", "Repeating Flash", 
                                       "N/A" /* "N/A" is not a nikon setting */
                                       };

#define EXN_FLASH_CONTROL_MODE_MASK 0x7F

#endif
