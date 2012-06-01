/* exif.c

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

#ifdef HAVE_LIBEXIF

#include <stdio.h>
#include <string.h>
#include <libexif/exif-data.h>

#include "feh.h"
#include "options.h"
#include "debug.h"
#include "exif.h"
#include "exif_nikon.h"


/* remove all spaces on the right end of a string */
static void exif_trim_spaces(char *str)
{
  char *end;

  for (end = str; *str!='\0'; str++)
  {
    if (*str != ' ')
    {
      end = str+1;
    }
  }
  *end = '\0';
}


/* show given exif tag content */
static void exif_get_tag(ExifData *d, ExifIfd ifd, ExifTag tag, char* buffer, unsigned int maxsize)
{
  char s[MAX_EXIF_DATA];

  if ( (d != NULL) && (buffer != NULL) && (maxsize>0) )
  {
    ExifEntry *entry = exif_content_get_entry(d->ifd[ifd], tag);
    if (entry != NULL)
    {
      /* Get the contents of the tag in human-readable form */
      exif_entry_get_value(entry, s, maxsize);

      /* Don't bother printing it if it's entirely blank */
      exif_trim_spaces(s);
      if (*s != '\0')
      {
        D(("%s: %s\n", exif_tag_get_name_in_ifd(tag,ifd), s));
        snprintf(buffer, (size_t)maxsize, "%s: %s\n", exif_tag_get_name_in_ifd(tag,ifd), s);
      }
    }
  }
}



/* Show the given MakerNote tag if it exists */
static void exif_get_mnote_tag(ExifData *d, unsigned int tag, char* buffer, unsigned int maxsize)
{
  ExifMnoteData *mn = NULL;
  int i, num;
  char buf[1024];

  if ( (d!=NULL) && (buffer!=NULL) && (maxsize > 0) )
  {
    mn = exif_data_get_mnote_data(d);
  }
  else
  {
    return;
  }

  if ( mn != NULL )
  {
    num = exif_mnote_data_count(mn);

    /* Loop through all MakerNote tags, searching for the desired one */
    for (i=0; i < num; ++i)
    {
      if (exif_mnote_data_get_id(mn, i) == tag)
      {
        if (exif_mnote_data_get_value(mn, i, buf, sizeof(buf)))
        {
          /* Don't bother printing it if it's entirely blank */
          exif_trim_spaces(buf);
          if (*buf != '\0')
          {
             snprintf(buffer, (size_t)maxsize, "%s: %s\n", exif_mnote_data_get_title(mn, i), buf);
          }
        }
      }
    }
  }
}



/* return data structure with exif data if available */
ExifData * exif_get_data(char *path)
{
  ExifData *ed = NULL;

  /* Load an ExifData object from an EXIF file */
  ed = exif_data_new_from_file(path);
  if (ed == NULL)
  {
    D(("File not readable or no Exif data present in %s\n", path));
  }

  return(ed);
}



/* get exif data in readable form */
void exif_get_info(ExifData * ed, char *buffer, unsigned int maxsize)
{
  ExifEntry *entry = NULL;
  char buf[128];
  unsigned int exn_fcm = (EXN_FLASH_CONTROL_MODES_MAX-1); /* default to N/A */
  unsigned int version = 0;
  unsigned int length = 0;

  if ( (buffer == NULL) || (maxsize == 0) )
  {
    return;
  }
  else if (ed == NULL)
  {
    snprintf(buffer, (size_t)maxsize, "%s\n", "No Exif data in file.");
    return;
  }
  else
  {
    /* normal exif tags */
    exif_get_tag(ed, EXIF_IFD_0, EXIF_TAG_MAKE, buffer, maxsize);
    exif_get_tag(ed, EXIF_IFD_0, EXIF_TAG_MODEL, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_0, EXIF_TAG_IMAGE_DESCRIPTION, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_ORIGINAL, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_DIGITIZED, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_TIME, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_SHUTTER_SPEED_VALUE, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_FNUMBER, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_APERTURE_VALUE, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_BIAS_VALUE, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_FOCAL_LENGTH, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_MODE, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_PROGRAM, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_SCENE_CAPTURE_TYPE, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_FLASH, buffer + strlen(buffer), maxsize - strlen(buffer));
    exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_FLASH_ENERGY, buffer + strlen(buffer), maxsize - strlen(buffer));

    /* vendor specific makernote tags */
    entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_MAKE);
    if (entry != NULL)
    {

      if (exif_entry_get_value(entry, buf, sizeof(buf)))
      {
        exif_trim_spaces(buf);

        /* Nikon */
        if ( strcmp(buf, "Nikon") != 0 )
        {
          /* this is a nikon camera */

          buf[0] = '\0';
          exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_FLASH, buf, sizeof(buf));
          exif_trim_spaces(buf);

          if ( !(strcmp("Flash: Flash did not fire\n", buf) == 0) )
          {
            /* extended flash info if it was fired */

            /* Flash Setting */
            exif_get_mnote_tag(ed, 8, buffer + strlen(buffer), maxsize - strlen(buffer));
            /* Flash Mode */
            exif_get_mnote_tag(ed, 9, buffer + strlen(buffer), maxsize - strlen(buffer));
            /* flash exposure bracket value */
            exif_get_mnote_tag(ed, 24, buffer + strlen(buffer), maxsize - strlen(buffer));
            /* Flash used */
            exif_get_mnote_tag(ed, 135, buffer + strlen(buffer), maxsize - strlen(buffer));

            /* Flash info: control mode. */
            /* libexif does not support flash info 168 yet. so we have to parse the debug data :-( */
            buf[0] = '\0';
            exif_get_mnote_tag(ed, 168, buf, sizeof(buf));
            sscanf(buf, "(null): %u bytes unknown data: 303130%02X%*10s%02X", &length, &version, &exn_fcm);
            exn_fcm = exn_fcm & EXN_FLASH_CONTROL_MODE_MASK;

            if ( (exn_fcm < EXN_FLASH_CONTROL_MODES_MAX)
                 && ( ((length == 22) && (version == '3'))      /* Nikon FlashInfo0103 */
                      || ((length == 22) && (version == '4'))   /* Nikon FlashInfo0104 */
                      || ((length == 21) && (version == '2'))   /* Nikon FlashInfo0102 */
                      || ((length == 19) && (version == '0'))   /* Nikon FlashInfo0100 */
                    )
               )
            {
              snprintf(buffer + strlen(buffer), maxsize - strlen(buffer), "NikonFlashControlMode: %s\n",
                EXN_NikonFlashControlModeValues[exn_fcm]);
            }
          }
          /* Lens */
          exif_get_mnote_tag(ed, 132, buffer + strlen(buffer), maxsize - strlen(buffer));
          /* Digital Vari-Program */
          exif_get_mnote_tag(ed, 171, buffer + strlen(buffer), maxsize - strlen(buffer));

        }

      }
    }
  }
}

#endif
