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
#include <limits.h>

#include "feh.h"
#include "options.h"
#include "debug.h"
#include "exif.h"
#include "exif_canon.h"
#include "exif_nikon.h"
#include "exif_cfg.h"


/* remove all spaces on the right end of a string */
void exif_trim_spaces(char *str)
{
  char *end;
  
  for (end = str; *str!='\0'; str++) 
  {
    if (*str != ' ')
    {
      end = str + 1;
    }
  }
  *end = '\0';
}



/* show given exif tag content with tag name */
void exif_get_tag(ExifData *d, ExifIfd ifd, ExifTag tag, char* buffer, unsigned int maxsize)
{
  char s[EXIF_MAX_DATA];
  ExifEntry *entry = NULL;
  
  if ( (d != NULL) && (buffer != NULL) && (maxsize > 0) )
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
void exif_get_tag_content(ExifData *d, ExifIfd ifd, ExifTag tag, char* buffer, unsigned int maxsize)
{
  char s[EXIF_MAX_DATA];
  ExifEntry *entry = NULL;

  if ( (d != NULL) && (buffer != NULL) && (maxsize > 0) )
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
        D(("%s - %s\n", exif_tag_get_name_in_ifd(tag,ifd), s));
        snprintf(buffer, (size_t)maxsize, "%s", s);
      }
    }
  }

}



/* Show the given MakerNote tag if it exists */
void exif_get_mnote_tag(ExifData *d, unsigned int tag, char* buffer, unsigned int maxsize)
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
      D(("%d/%d %d 0x%2x %s; %s\n", i, num, exif_mnote_data_get_id(mn, i), 
        exif_mnote_data_get_id(mn, i),
        exif_mnote_data_get_name(mn,i), 
        exif_mnote_data_get_title(mn, i) ));
      
      if (exif_mnote_data_get_id(mn, i) == tag) 
      {
        if (exif_mnote_data_get_value(mn, i, buf, sizeof(buf))) 
        {
          /* Don't bother printing it if it's entirely blank */
          exif_trim_spaces(buf);
          if (*buf != '\0')
          {
             D(("%s\n", buf));
             snprintf(buffer, (size_t)maxsize, "%s: %s\n", exif_mnote_data_get_title(mn, i), buf);
          }
        }
      }
    }
  }
}



/* get gps coordinates if available */
void exif_get_gps_coords(ExifData * ed, char *buffer, unsigned int maxsize)
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




/* get all exif data in readable form */
void exif_get_info(ExifData * ed, char *buffer, unsigned int maxsize)
{
  ExifEntry *entry = NULL;
  char buf[EXIF_STD_BUF_LEN];
  unsigned short int i = 0;
  
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
    /* show normal exif tags. list must be defined in exif_cfg.h  */
    while ( (i < USHRT_MAX) && (Exif_tag_list[i].ifd != EXIF_IFD_COUNT) )
    {
      exif_get_tag(ed, Exif_tag_list[i].ifd, Exif_tag_list[i].tag, buffer + strlen(buffer), maxsize - strlen(buffer));
      i++; 
    }

    /* show vendor specific makernote tags */
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
          /* show nikon makernote exif tags. list must be defined in exif_cfg.h  */
          i=0;
          while ( (i < USHRT_MAX) && (Exif_makernote_nikon_tag_list[i] != EXIF_NIKON_MAKERNOTE_END) )
          {
            exn_get_mnote_nikon_tags(ed, Exif_makernote_nikon_tag_list[i], 
                                     buffer + strlen(buffer), maxsize - strlen(buffer));
            i++; 
          }

        }
        else if ( (strcmp(buf, "Canon") == 0) )
        {
          /* show canon makernote exif tags. list must be defined in exif_cfg.h  */
          i=0;
          while ( (i < USHRT_MAX) && (Exif_makernote_canon_tag_list[i] != EXIF_CANON_MAKERNOTE_END) )
          {
            exc_get_mnote_canon_tags(ed, Exif_makernote_canon_tag_list[i], 
                                     buffer + strlen(buffer), maxsize - strlen(buffer));
            i++; 
          }
        
        }
        else
        {
        }
      }
      
    }
    
    /* show gps coordinates */
    exif_get_gps_coords(ed, buffer + strlen(buffer), maxsize - strlen(buffer));

  }
  
}

#endif
