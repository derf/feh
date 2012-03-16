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
#include "exif_cfg.h"

static void exif_trim_spaces(char *str);
static void exif_get_tag(ExifData *d, ExifIfd ifd, ExifTag tag, char* buffer, unsigned int maxsize);
static void exif_get_tag_content(ExifData *d, ExifIfd ifd, ExifTag tag, char* buffer, unsigned int maxsize);
static void exif_get_mnote_tag(ExifData *d, unsigned int tag, char* buffer, unsigned int maxsize);
static void exif_get_mnote_nikon_tags(ExifData *ed, char * buffer, unsigned int maxsize);
static void exif_get_gps_coords(ExifData * ed, char *buffer, unsigned int maxsize);

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


/* show given exif tag content with tag name */
static void exif_get_tag(ExifData *d, ExifIfd ifd, ExifTag tag, char* buffer, unsigned int maxsize)
{
  char s[EXIF_MAX_DATA];
  ExifEntry *entry = NULL;
  
  if ( (d != NULL) && (buffer != NULL) && (maxsize>0) )
  {
    entry = exif_content_get_entry(d->ifd[ifd], tag);
    if (entry != NULL) 
    {
      /* Get the contents of the tag in human-readable form */
      exif_entry_get_value(entry, s, EXIF_MAX_DATA);

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



/* show given exif tag content without tag name */
static void exif_get_tag_content(ExifData *d, ExifIfd ifd, ExifTag tag, char* buffer, unsigned int maxsize)
{
  char s[EXIF_MAX_DATA];
  ExifEntry *entry = NULL;

  if ( (d != NULL) && (buffer != NULL) && (maxsize>0) )
  {
    entry = exif_content_get_entry(d->ifd[ifd], tag);
    if (entry != NULL) 
    {
      /* Get the contents of the tag in human-readable form */
      exif_entry_get_value(entry, s, EXIF_MAX_DATA);

      /* Don't bother printing it if it's entirely blank */
      exif_trim_spaces(s);
      if (*s != '\0')
      {
        D(("%s", exif_tag_get_name_in_ifd(tag,ifd), s));
        snprintf(buffer, (size_t)maxsize, "%s", s);
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



/* get interesting nikon maker note tags in readable form */
static void exif_get_mnote_nikon_tags(ExifData *ed, char * buffer, unsigned int maxsize)
{
  char buf[EXIF_STD_BUF_LEN];
  unsigned int exn_fcm = (EXN_FLASH_CONTROL_MODES_MAX-1); /* default to N/A */
  unsigned int version = 0;
  unsigned int length = 0;

  buf[0] = '\0';
  exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_FLASH, buf, sizeof(buf));
  exif_trim_spaces(buf);

  if ( !(strcmp("Flash: Flash did not fire\n", buf) == 0) )
  {
    /* show extended flash info if flash was fired */
  
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

  return;
}



/* get gps coordinates if available */
static void exif_get_gps_coords(ExifData * ed, char *buffer, unsigned int maxsize)
{
  char buf[EXIF_STD_BUF_LEN];
  
  buf[0] = '\0';
  exif_get_tag_content(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_LATITUDE_REF, buf, sizeof(buf));
  if ( buf[0] != '\0' )
  {
    snprintf(buffer + strlen(buffer), maxsize - strlen(buffer), "GPS: %s ", buf);
  }
  else
  {
    return;
  }

  buf[0] = '\0';
  exif_get_tag_content(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_LATITUDE, buf, sizeof(buf));
  if ( buf[0] != '\0' )
  {
    snprintf(buffer + strlen(buffer), maxsize - strlen(buffer), "%s ", buf);
  }
  else
  {
    return;
  }

  buf[0] = '\0';
  exif_get_tag_content(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_LONGITUDE_REF, buf, sizeof(buf));
  if ( buf[0] != '\0' )
  {
    snprintf(buffer + strlen(buffer), maxsize - strlen(buffer), ", %s ", buf);
  }
  else
  {
    return;
  }

  buf[0] = '\0';
  exif_get_tag_content(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_LONGITUDE, buf, sizeof(buf));
  if ( buf[0] != '\0' )
  {
    snprintf(buffer + strlen(buffer), maxsize - strlen(buffer), "%s ", buf);
  }
  else
  {
    return;
  }

  buf[0] = '\0';
  exif_get_tag_content(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_MAP_DATUM, buf, sizeof(buf));
  if ( buf[0] != '\0' )
  {
    snprintf(buffer + strlen(buffer), maxsize - strlen(buffer), "(%s)\n", buf);
  }
  else
  {
    return;
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
  char buf[EXIF_STD_BUF_LEN];
  int i = 0;
  
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
    while ( Exif_tag_list[i].ifd != EXIF_IFD_COUNT )
    {
      exif_get_tag(ed, Exif_tag_list[i].ifd, Exif_tag_list[i].tag, buffer + strlen(buffer), maxsize - strlen(buffer));
      i++; 
    }

    /* vendor specific makernote tags */
    entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_MAKE);
    if (entry != NULL) 
    {
                
      if (exif_entry_get_value(entry, buf, sizeof(buf))) 
      {
        exif_trim_spaces(buf);
        
        if ( (strcmp(buf, "NIKON CORPORATION") == 0)
             || (strcmp(buf, "Nikon") == 0) 
             || (strcmp(buf, "NIKON") == 0)
           )
        {
          /* this is a nikon camera */
          exif_get_mnote_nikon_tags(ed, buffer + strlen(buffer), maxsize - strlen(buffer));
        }

      }
      
    }
    
    /* gps info */
    exif_get_gps_coords(ed, buffer + strlen(buffer), maxsize - strlen(buffer));

  }
  
}

#endif
