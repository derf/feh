/* exif_nikon.c

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
#include <libexif/exif-data.h>

#include "feh.h"
#include "debug.h"
#include "exif.h"
#include "exif_nikon.h"


/* Flash control mode */
/* http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/Nikon.html#FlashControlMode */
#define EXN_FLASH_CONTROL_MODES_MAX 9
char *EXN_NikonFlashControlModeValues[EXN_FLASH_CONTROL_MODES_MAX] =
    { "Off",
	"iTTL-BL", "iTTL", "Auto Aperture",
	"Automatic", "GN (distance priority)",
	"Manual", "Repeating Flash",
	"N/A"			/* "N/A" is not a nikon setting */
};

#define EXN_FLASH_CONTROL_MODE_MASK 0x7F


/* AFInfo2 */
/* http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/Nikon.html#AFInfo2 */
#define EXN_CONTRAST_DETECT_AF_MAX 2
char *EXN_NikonContrastDetectAF[EXN_CONTRAST_DETECT_AF_MAX] =
    { "Off", "On" };

/* AFArea Mode for ContrastDetectAF Off */
#define EXN_AF_AREA_MODE_P_MAX 13
char *EXN_NikonAFAreaModePhase[EXN_AF_AREA_MODE_P_MAX] = {
	"Single Area", "Dynamic Area", "Dynamic Area (closest subject)",
	"Group Dynamic ", "Dynamic Area (9 points) ",
	"Dynamic Area (21 points)",
	"Dynamic Area (51 points) ",
	"Dynamic Area (51 points, 3D-tracking)",
	"Auto-area", "Dynamic Area (3D-tracking)", "Single Area (wide)",
	"Dynamic Area (wide)", "Dynamic Area (wide, 3D-tracking)"
};

/* AFArea Mode for ContrastDetectAF On */
#define EXN_AF_AREA_MODE_C_MAX 5
char *EXN_NikonAFAreaModeContr[EXN_AF_AREA_MODE_C_MAX] = {
	"Contrast-detect",
	"Contrast-detect (normal area)",
	"Contrast-detect (wide area)",
	"Contrast-detect (face priority)",
	"Contrast-detect (subject tracking)"
};

#define EXN_PHASE_DETECT_AF_MAX 4
char *EXN_NikonPhaseDetectAF[EXN_PHASE_DETECT_AF_MAX] =
    { "Off", "On (51-point)",
	"On (11-point)", "On (39-point)"
};

/* PrimaryAFPoint and AFPointsUsed only valid with PhaseDetectAF == On */

#define EXN_PRIM_AF_PT_51_MAX 52
char *EXN_Prim_AF_Pt_51[EXN_PRIM_AF_PT_51_MAX] =
    { "(none)", "C6 (Center)", "B6", "A5",
	"D6", "E5", "C7", "B7", "A6", "D7", "E6", "C5", "B5", "A4", "D5",
	"E4", "C8", "B8",
	"A7", "D8", "E7", "C9", "B9", "A8", "D9", "E8", "C10", "B10", "A9",
	"D10", "E9",
	"C11", "B11", "D11", "C4", "B4", "A3", "D4", "E3", "C3", "B3",
	"A2", "D3", "E2",
	"C2", "B2", "A1", "D2", "E1", "C1", "B1", "D1"
};

#define EXN_PRIM_AF_PT_11_MAX 12
char *EXN_Prim_AF_Pt_11[EXN_PRIM_AF_PT_11_MAX] =
    { "(none)", "Center", "Top", "Bottom",
	"Mid-left", "Upper-left", "Lower-left", "Far Left", "Mid-right",
	"Upper-right",
	"Lower-right", "Far Right"
};

#define EXN_PRIM_AF_PT_39_MAX 40
char *EXN_Prim_AF_Pt_39[EXN_PRIM_AF_PT_39_MAX] =
    { "(none)", "C6 (Center)", "B6", "A2",
	"D6", "E2", "C7", "B7", "A3", "D7", "E3", "C5", "B5", "A1", "D5",
	"E1", "C8", "B8",
	"D8", "C9", "B9", "D9", "C10", "B10", "D10", "C11", "B11", "D11",
	"C4", "B4", "D4",
	"C3", "B3", "D3", "C2", "B2", "D2", "C1", "B1", "D1"
};


#define EXN_PIC_CTRL_ADJ_MAX 3
char *EXN_Pic_Ctrl_Adj[EXN_PIC_CTRL_ADJ_MAX] = { "Default Settings",
	"Quick Adjust",
	"Full Control"
};



static void exn_get_prim_af_pt(unsigned int phasedetectaf,
			       unsigned int primafpt,
			       char *buffer, unsigned int maxsize);
static void exn_get_flash_output(unsigned int flashoutput, char *buffer,
				 unsigned int maxsize);
static void exn_get_mnote_nikon_18(ExifData * ed, char *buffer,
				   unsigned int maxsize);
static void exn_get_mnote_nikon_34(ExifData * ed, char *buffer,
				   unsigned int maxsize);
static void exn_get_mnote_nikon_35(ExifData * ed, char *buffer,
				   unsigned int maxsize);
static void exn_get_mnote_nikon_168(ExifData * ed, char *buffer,
				    unsigned int maxsize);
static void exn_get_mnote_nikon_183(ExifData * ed, char *buffer,
				    unsigned int maxsize);



/* get primary AF point */
static void exn_get_prim_af_pt(unsigned int phasedetectaf,
			       unsigned int primafpt,
			       char *buffer, unsigned int maxsize)
{

	switch (phasedetectaf) {
	case 0:
		{
			/* phasedetect not used. should not happen */
			snprintf(buffer, maxsize, "FAIL");
			return;
		}
		break;
	case 1:
		{
			/* 51 pt */
			if (primafpt < EXN_PRIM_AF_PT_51_MAX) {
				snprintf(buffer, maxsize, "%s",
					 EXN_Prim_AF_Pt_51[primafpt]);
			}
			return;
		}
		break;
	case 2:
		{
			/* 11 pt */
			if (primafpt < EXN_PRIM_AF_PT_11_MAX) {
				snprintf(buffer, maxsize, "%s",
					 EXN_Prim_AF_Pt_11[primafpt]);
			}
			return;
		}
		break;
	case 3:
		{
			/* 39 pt */
			if (primafpt < EXN_PRIM_AF_PT_39_MAX) {
				snprintf(buffer, maxsize, "%s",
					 EXN_Prim_AF_Pt_39[primafpt]);
			}
			return;
		}
		break;
	default:
		{
			snprintf(buffer, maxsize, "?");
			return;
		}
		break;

	}

}



/* get flash output power (for FlashInfo010x) */
static void exn_get_flash_output(unsigned int flashoutput, char *buffer,
				 unsigned int maxsize)
{

	if (flashoutput == 0) {
		/* full power */
		snprintf(buffer, maxsize, "Full");
	} else {
		if ((flashoutput % 6) == 0) {
			/* value is a power of 2 */
			snprintf(buffer, maxsize, "1/%d",
				 1 << (flashoutput / 6));
		} else {
			/* something uneven...ugly. maybe introduce pow() function from libm later */
			snprintf(buffer, maxsize, "1/2^(%f)",
				 ((float) flashoutput) / 6.0);
		}
	}
}



/* get ActiveD-Lighting (18) info */
static void exn_get_mnote_nikon_18(ExifData * ed, char *buffer,
				   unsigned int maxsize)
{

	char buf[EXIF_STD_BUF_LEN];
	float data = 0;

	buf[0] = '\0';
	exif_get_mnote_tag(ed, 18, buf, sizeof(buf));

	sscanf(buf, "Flash Exposure Compensation: %f", &data);	/* libexif buggy here. fix conversion */

	snprintf(buffer, maxsize, "FlashExposureCompensation: %+.1f EV\n",
		 ((float) ((signed char) round(data * 6.0))) / 6.0);
}



/* get ActiveD-Lighting (34) info */
static void exn_get_mnote_nikon_34(ExifData * ed, char *buffer,
				   unsigned int maxsize)
{
	char buf[EXIF_STD_BUF_LEN];
	unsigned int data = 0;
	char *answer;

	buf[0] = '\0';
	exif_get_mnote_tag(ed, 34, buf, sizeof(buf));
	sscanf(buf, "(null): %u", &data);	/* not directly supported by libexif yet */

	switch (data) {
	case 0:
		{
			answer = "Off";
		}
		break;
	case 1:
		{
			answer = "Low";
		}
		break;
	case 3:
		{
			answer = "Normal";
		}
		break;
	case 5:
		{
			answer = "High";
		}
		break;
	case 7:
		{
			answer = "Extra High";
		}
		break;
	case 65535:
		{
			answer = "Auto";
		}
		break;
	default:
		{
			answer = "N/A";	/* this is not a nikon value */
		}

	}

	snprintf(buffer, maxsize, "Active D-Lightning: %s\n", answer);

}



/* get nikon PictureControlData (35) info */
static void exn_get_mnote_nikon_35(ExifData * ed, char *buffer,
				   unsigned int maxsize)
{
	char buf[EXIF_STD_BUF_LEN];
	char picturecontrolname[EXIF_STD_BUF_LEN];
	char picturecontrolbase[EXIF_STD_BUF_LEN];
	unsigned int version = 0;
	unsigned int length = 0;
	unsigned int piccontroladj = 0;
	unsigned int piccontrolquickadj = 0;
	unsigned int sharpness = 0;
	unsigned int contrast = 0;
	unsigned int brightness = 0;
	unsigned int saturation = 0;
	unsigned int hueadjustment = 0;
	unsigned int i, j;

	/* libexif does not support PictureControlData 35 yet. so we have to parse the debug data :-( */
	buf[0] = '\0';
	exif_get_mnote_tag(ed, 35, buf, sizeof(buf));

	sscanf(buf,
	       "(null): %u bytes unknown data: 303130%02X%40s%40s%*8s%02X%02X%02X%02X%02X%02X%02X",
	       &length, &version, &picturecontrolname[0],
	       &picturecontrolbase[0], &piccontroladj, &piccontrolquickadj,
	       &sharpness, &contrast, &brightness, &saturation,
	       &hueadjustment);

	/* printf("--%s %d-%d-\n", buf, version, piccontroladj); */

	for (i = 0; i < 40; i++) {
		sscanf(&picturecontrolname[2 * i], "%2X", &j);
		picturecontrolname[i] = j;
		sscanf(&picturecontrolbase[2 * i], "%2X", &j);
		picturecontrolbase[i] = j;

	}
	exif_trim_spaces(picturecontrolname);
	exif_trim_spaces(picturecontrolbase);

	if (((length == 58) && (version == '0'))
	    && (piccontroladj < EXN_PIC_CTRL_ADJ_MAX)
	    ) {
		snprintf(buffer, maxsize,
			 "PictCtrlData: Name: %s; Base: %s; CtrlAdj: %s; Quick: %d; Shrp: %d; Contr: %d; Brght: %d; Sat: %d; Hue: %d\n",
			 picturecontrolname, picturecontrolbase,
			 EXN_Pic_Ctrl_Adj[piccontroladj],
			 piccontrolquickadj, sharpness, contrast,
			 brightness, saturation, hueadjustment);
	}

}




/* get nikon Flash info: control mode (168) info */
static void exn_get_mnote_nikon_168(ExifData * ed, char *buffer,
				    unsigned int maxsize)
{
	char buf[EXIF_STD_BUF_LEN];
	unsigned int version = 0;
	unsigned int length = 0;
	unsigned int exn_fcm = (EXN_FLASH_CONTROL_MODES_MAX - 1);	/* default to N/A */
	unsigned int flashoutput = 0;
	unsigned int externalflashflags = 0;
	unsigned int flashcompensation = 0;

	/* libexif does not support flash info 168 yet. so we have to parse the debug data :-( */
	buf[0] = '\0';
	exif_get_mnote_tag(ed, 168, buf, sizeof(buf));
	sscanf(buf,
	       "(null): %u bytes unknown data: 303130%02X%*8s%02X%02X%02X%02X",
	       &length, &version, &externalflashflags, &exn_fcm,
	       &flashoutput, &flashcompensation);
	exn_fcm = exn_fcm & EXN_FLASH_CONTROL_MODE_MASK;

	/* printf("%s - %d %d %d %d\n", buf, externalflashflags, exn_fcm, flashoutput, (signed char)flashcompensation); */

	if ((exn_fcm < EXN_FLASH_CONTROL_MODES_MAX)
	    && (((length == 22) && (version == '3'))	/* Nikon FlashInfo0103 */
		||((length == 22) && (version == '4'))	/* Nikon FlashInfo0104 */
		||((length == 21) && (version == '2'))	/* Nikon FlashInfo0102 */
		||((length == 19) && (version == '0'))	/* Nikon FlashInfo0100 */
	    )
	    ) {

		buf[0] = '\0';
		exn_get_flash_output(flashoutput, buf, EXIF_STD_BUF_LEN);
		snprintf(buffer, maxsize,
			 "NikonFlashControlMode: %s (Power: %s)\n",
			 EXN_NikonFlashControlModeValues[exn_fcm], buf);

		/* External Flash Flags. Not as useful as expected. Not used (yet). */
		/* if ( (externalflashflags & (1<<2)) ) -> Bounce Flash */
		/* if ( (externalflashflags & (1<<4)) ) -> Wide Flash Adapter */
		/* if ( (externalflashflags & (1<<5)) ) -> Dome Diffusor */

	}

}



/* get nikon AFInfo2 (183) info */
static void exn_get_mnote_nikon_183(ExifData * ed, char *buffer,
				    unsigned int maxsize)
{
	char buf[EXIF_STD_BUF_LEN];
	unsigned int contrastdetectaf = 0;
	unsigned int afareamode = 0;
	unsigned int phasedetectaf = 0;
	unsigned int primaryafpoint = 0;
	unsigned int version = 0;
	unsigned int length = 0;

	/* AFInfo2 */
	/* libexif does not support AFInfo2 183 yet. so we have to parse the debug data :-( */
	buf[0] = '\0';
	exif_get_mnote_tag(ed, 183, buf, sizeof(buf));
	sscanf(buf,
	       "(null): %u bytes unknown data: 303130%02X%02X%02X%02X%02X",
	       &length, &version, &contrastdetectaf, &afareamode,
	       &phasedetectaf, &primaryafpoint);


	if (((length == 30) && (version == '0'))
	    && (contrastdetectaf < EXN_CONTRAST_DETECT_AF_MAX)
	    && (phasedetectaf < EXN_PHASE_DETECT_AF_MAX)
	    ) {
		if ((contrastdetectaf != 0)
		    && (afareamode < EXN_AF_AREA_MODE_C_MAX)) {
			/* Contrast AF (live view) */
			snprintf(buffer, maxsize,
				 "ContrastDetectAF: %s; AFAreaMode: %s\n",
				 EXN_NikonContrastDetectAF
				 [contrastdetectaf],
				 EXN_NikonAFAreaModeContr[afareamode]);

		} else if ((phasedetectaf != 0)
			   && (afareamode < EXN_AF_AREA_MODE_P_MAX)) {
			/* Phase AF */
			buf[0] = '\0';
			exn_get_prim_af_pt(phasedetectaf, primaryafpoint,
					   buf, EXIF_STD_BUF_LEN);

			snprintf(buffer, maxsize,
				 "PhaseDetectAF: %s; AreaMode: %s; PrimaryAFPoint: %s\n",
				 EXN_NikonPhaseDetectAF[phasedetectaf],
				 EXN_NikonAFAreaModePhase[afareamode],
				 buf);
		}

	}
}



/* get interesting nikon maker note tags in readable form */
void exn_get_mnote_nikon_tags(ExifData * ed, unsigned int tag,
			      char *buffer, unsigned int maxsize)
{
	char buf[EXIF_STD_BUF_LEN];

	buf[0] = '\0';
	exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_FLASH, buf, sizeof(buf));
	exif_trim_spaces(buf);

	switch (tag) {
		/* show only if flash was used */
	case 8:		/* Flash Setting */
	case 9:		/* Flash Mode */
	case 135:		/* Flash used */
		{
			if (!
			    (strcmp("Flash: Flash did not fire\n", buf) ==
			     0)) {
				/* show extended flash info only if flash was fired */
				exif_get_mnote_tag(ed, tag, buffer,
						   maxsize);
			}
		}
		break;

	case 18:		/* FlashExposureComp */
		{
			if (!
			    (strcmp("Flash: Flash did not fire\n", buf) ==
			     0)) {
				/* show only if flash was fired */
				exn_get_mnote_nikon_18(ed, buffer,
						       maxsize);
			}
		}
		break;

	case 34:
		{
			/* ActiveD-Lighting */
			exn_get_mnote_nikon_34(ed, buffer, maxsize);
		}
		break;

	case 35:
		{
			/* PictureControlData */
			exn_get_mnote_nikon_35(ed, buffer, maxsize);
		}
		break;

	case 168:
		{
			/* Flash info: control mode */
			if (!
			    (strcmp("Flash: Flash did not fire\n", buf) ==
			     0)) {
				/* show extended flash info only if flash was fired */
				exn_get_mnote_nikon_168(ed, buffer,
							maxsize);
			}
		}
		break;

	case 183:
		{
			/* AFInfo 2 */
			exn_get_mnote_nikon_183(ed, buffer, maxsize);
		}
		break;

	default:
		{
			/* normal makernote tags without special treatment */
			exif_get_mnote_tag(ed, tag, buffer, maxsize);
		}
		break;
	}


	return;
}

#endif
