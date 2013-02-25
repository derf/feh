/* exif_cfg.h

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

#ifndef EXIF_CFG_H
#define EXIF_CFG_H

#include <libexif/exif-data.h>

typedef struct
{
  ExifIfd ifd;  /* section */
  ExifTag tag;  /* tag */
} t_EXIF_INFO;


/* show these standard tags. section must be given first, than the tag itself */
/* definition: http://libexif.sourceforge.net/api/exif-tag_8h.html */
const t_EXIF_INFO Exif_tag_list [] =
{
  {EXIF_IFD_0, EXIF_TAG_MAKE},
  {EXIF_IFD_0, EXIF_TAG_MODEL},
  {EXIF_IFD_0, EXIF_TAG_IMAGE_DESCRIPTION},
  {EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_ORIGINAL},
  {EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_TIME},
  {EXIF_IFD_EXIF, EXIF_TAG_SHUTTER_SPEED_VALUE},
  {EXIF_IFD_EXIF, EXIF_TAG_FNUMBER},
  {EXIF_IFD_EXIF, EXIF_TAG_APERTURE_VALUE},
  {EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_BIAS_VALUE},
  {EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS},
  {EXIF_IFD_EXIF, EXIF_TAG_FOCAL_LENGTH},
  {EXIF_IFD_EXIF, EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM},
  {EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_MODE},
  {EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_PROGRAM},
  {EXIF_IFD_EXIF, EXIF_TAG_SCENE_CAPTURE_TYPE},
  {EXIF_IFD_EXIF, EXIF_TAG_FLASH},
  
  {EXIF_IFD_COUNT, 0}	/* end marker */
};



/* Nikon */

#define EXIF_NIKON_MAKERNOTE_END 0  /* end marker: if 0 used as a tag we must find something else */

/* show these nikon makernote tags */
const unsigned int Exif_makernote_nikon_tag_list [] =
{

  6,
  8,   /* Flash Setting */
  9,   /* Flash Mode */
  135, /* Flash used */
  18,  /* Flash Exposure Comp */
  168, /* Flash info: control mode */

  2,   /* ISO. Has some more info than EXIF_TAG_ISO_SPEED_RATINGS but also fails on Lo.1 */  
  5,   /* White Balance */
  132, /* Lens */
  171, /* Digital Vari-Program */
  34,  /* Active D-Lighting */

  35,  /* PictureControlData */
  183, /* AFInfo2 */

  EXIF_NIKON_MAKERNOTE_END   /* end marker */
};



/* Canon */
#define EXIF_CANON_MAKERNOTE_END 0xFFFF  /* end marker: if this is used as a tag we must find something else */

/* show these canon makernote tags */
const unsigned int Exif_makernote_canon_tag_list [] =
{
  8,  /* Image Number */
  9,  /* Owner Name */
  
  EXIF_CANON_MAKERNOTE_END   /* end marker */
};


#endif
