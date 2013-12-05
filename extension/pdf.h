/*
 * pdf.h - Subset of libharu PDF Library
 *
 *  Author: Hiroshi Saito (hiroshi@winpg.jp)
 *  Create: 2013.10.20
 */

/*
 * Copyright (C) 1986, 1988, 1989, 1991-2004 the Free Software Foundation, Inc.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#ifndef _PDF_H
#define _PDF_H 1

#define HAVE_LIBPNG 1

#include <hpdf.h>
// #include <time.h>

/* Libharu defines */
#define DEFAULT_HPDF_TRUE HPDF_TRUE
#define DEFAULT_HPDF_FALSE HPDF_FALSE

/* defalt page-size */
#define DEFAULT_HPDF_DEF_PAGE_WIDTH HPDF_DEF_PAGE_WIDTH
#define DEFAULT_HPDF_DEF_PAGE_HEIGHT HPDF_DEF_PAGE_HEIGHT

/* compression mode */
#define DEFAULT_HPDF_COMP_NONE HPDF_COMP_NONE
#define DEFAULT_HPDF_COMP_TEXT HPDF_COMP_TEXT
#define DEFAULT_HPDF_COMP_IMAGE HPDF_COMP_IMAGE
#define DEFAULT_HPDF_COMP_METADATA HPDF_COMP_METADATA
#define DEFAULT_HPDF_COMP_ALL HPDF_COMP_ALL

/* InfoType */
#define DEFAULT_HPDF_INFO_CREATION_DATE  HPDF_INFO_CREATION_DATE
#define DEFAULT_HPDF_INFO_MOD_DATE HPDF_INFO_MOD_DATE 
#define DEFAULT_HPDF_INFO_AUTHOR HPDF_INFO_AUTHOR 
#define DEFAULT_HPDF_INFO_CREATOR HPDF_INFO_CREATOR
#define DEFAULT_HPDF_INFO_PRODUCER HPDF_INFO_PRODUCER 
#define DEFAULT_HPDF_INFO_TITLE HPDF_INFO_TITLE
#define DEFAULT_HPDF_INFO_SUBJECT HPDF_INFO_SUBJECT
#define DEFAULT_HPDF_INFO_KEYWORDS HPDF_INFO_KEYWORDS 

/* PdfVer */
#define DEFAULT_HPDF_VER_12 HPDF_VER_12
#define DEFAULT_HPDF_VER_13 HPDF_VER_13
#define DEFAULT_HPDF_VER_14 HPDF_VER_14 
#define DEFAULT_HPDF_VER_15 HPDF_VER_15
#define DEFAULT_HPDF_VER_16 HPDF_VER_16
#define DEFAULT_HPDF_VER_17 HPDF_VER_17

/* EncryptMode */
#define DEFAULT_HPDF_ENCRYPT_R2 HPDF_ENCRYPT_R2
#define DEFAULT_HPDF_ENCRYPT_R3 HPDF_ENCRYPT_R3 

/* PageSizes */
#define DEFAULT_HPDF_PAGE_SIZE_LETTER HPDF_PAGE_SIZE_LETTER
#define DEFAULT_HPDF_PAGE_SIZE_LEGAL HPDF_PAGE_SIZE_LEGAL
#define DEFAULT_HPDF_PAGE_SIZE_A3 HPDF_PAGE_SIZE_A3
#define DEFAULT_HPDF_PAGE_SIZE_A4 HPDF_PAGE_SIZE_A4
#define DEFAULT_HPDF_PAGE_SIZE_A5 HPDF_PAGE_SIZE_A5
#define DEFAULT_HPDF_PAGE_SIZE_B4 HPDF_PAGE_SIZE_B4
#define DEFAULT_HPDF_PAGE_SIZE_B5 HPDF_PAGE_SIZE_B5
#define DEFAULT_HPDF_PAGE_SIZE_EXECUTIVE HPDF_PAGE_SIZE_EXECUTIVE
#define DEFAULT_HPDF_PAGE_SIZE_US4x6 HPDF_PAGE_SIZE_US4x6
#define DEFAULT_HPDF_PAGE_SIZE_US4x8 HPDF_PAGE_SIZE_US4x8
#define DEFAULT_HPDF_PAGE_SIZE_US5x7 HPDF_PAGE_SIZE_US5x7
#define DEFAULT_HPDF_PAGE_SIZE_COMM10 HPDF_PAGE_SIZE_COMM10

#define DEFAULT_HPDF_CS_DEVICE_GRAY HPDF_CS_DEVICE_GRAY
#define DEFAULT_HPDF_CS_DEVICE_RGB HPDF_CS_DEVICE_RGB
#define DEFAULT_HPDF_CS_DEVICE_CMYK HPDF_CS_DEVICE_CMYK
#define DEFAULT_HPDF_CS_CAL_GRAY HPDF_CS_CAL_GRAY
#define DEFAULT_HPDF_CS_CAL_RGB HPDF_CS_CAL_RGB
#define DEFAULT_HPDF_CS_LAB HPDF_CS_LAB
#define DEFAULT_HPDF_CS_ICC_BASED HPDF_CS_ICC_BASED
#define DEFAULT_HPDF_CS_SEPARATION HPDF_CS_SEPARATION
#define DEFAULT_HPDF_CS_DEVICE_N HPDF_CS_DEVICE_N
#define DEFAULT_HPDF_CS_INDEXED HPDF_CS_INDEXED
#define DEFAULT_HPDF_CS_PATTERN HPDF_CS_PATTERN

#define DEFAULT_HPDF_BUTT_END HPDF_BUTT_END
#define DEFAULT_HPDF_ROUND_END HPDF_ROUND_END
#define DEFAULT_HPDF_PROJECTING_SCUARE_END HPDF_PROJECTING_SCUARE_END

/*  _LineJoin */
#define DEFAULT_HPDF_MITER_JOIN HPDF_MITER_JOIN
#define DEFAULT_HPDF_ROUND_JOIN HPDF_ROUND_JOIN
#define DEFAULT_HPDF_BEVEL_JOIN HPDF_BEVEL_JOIN

/* TextRenderingMode */
#define DEFAULT_HPDF_FILL HPDF_FILL
#define DEFAULT_HPDF_STROKE HPDF_STROKE
#define DEFAULT_HPDF_FILL_THEN_STROKE HPDF_FILL_THEN_STROKE
#define DEFAULT_HPDF_INVISIBLE HPDF_INVISIBLE
#define DEFAULT_HPDF_FILL_CLIPPING HPDF_FILL_CLIPPING
#define DEFAULT_HPDF_STROKE_CLIPPING HPDF_STROKE_CLIPPING
#define DEFAULT_HPDF_FILL_STROKE_CLIPPING HPDF_FILL_STROKE_CLIPPING
#define DEFAULT_HPDF_CLIPPING HPDF_CLIPPING

/*  WritingMode */
#define DEFAULT_HPDF_WMODE_HORIZONTAL HPDF_WMODE_HORIZONTAL
#define DEFAULT_HPDF_WMODE_VERTICAL HPDF_WMODE_VERTICAL 

/*  PageLayout */
#define DEFAULT_HPDF_PAGE_LAYOUT_SINGLE HPDF_PAGE_LAYOUT_SINGLE
#define DEFAULT_HPDF_PAGE_LAYOUT_ONE_COLUMN HPDF_PAGE_LAYOUT_ONE_COLUMN
#define DEFAULT_HPDF_PAGE_LAYOUT_TWO_COLUMN_LEFT HPDF_PAGE_LAYOUT_TWO_COLUMN_LEFT
#define DEFAULT_HPDF_PAGE_LAYOUT_TWO_COLUMN_RIGHT HPDF_PAGE_LAYOUT_TWO_COLUMN_RIGHT

/* PageMode */
#define DEFAULT_HPDF_PAGE_MODE_USE_NONE HPDF_PAGE_MODE_USE_NONE 
#define DEFAULT_HPDF_PAGE_MODE_USE_OUTLINE HPDF_PAGE_MODE_USE_OUTLINE
#define DEFAULT_HPDF_PAGE_MODE_USE_THUMBS HPDF_PAGE_MODE_USE_THUMBS 
#define DEFAULT_HPDF_PAGE_MODE_FULL_SCREEN HPDF_PAGE_MODE_FULL_SCREEN 

/* PageNumStyle */
#define DEFAULT_HPDF_PAGE_NUM_STYLE_DECIMAL HPDF_PAGE_NUM_STYLE_DECIMAL 
#define DEFAULT_HPDF_PAGE_NUM_STYLE_UPPER_ROMAN HPDF_PAGE_NUM_STYLE_UPPER_ROMAN 
#define DEFAULT_HPDF_PAGE_NUM_STYLE_LOWER_ROMAN HPDF_PAGE_NUM_STYLE_LOWER_ROMAN 
#define DEFAULT_HPDF_PAGE_NUM_STYLE_UPPER_LETTERS HPDF_PAGE_NUM_STYLE_UPPER_LETTERS
#define DEFAULT_HPDF_PAGE_NUM_STYLE_LOWER_LETTERS HPDF_PAGE_NUM_STYLE_LOWER_LETTERS

/*  DestinationType */
#define DEFAULT_HPDF_XYZ HPDF_XYZ 
#define DEFAULT_HPDF_FIT HPDF_FIT 
#define DEFAULT_HPDF_FIT_H HPDF_FIT_H
#define DEFAULT_HPDF_FIT_V HPDF_FIT_V 
#define DEFAULT_HPDF_FIT_R HPDF_FIT_R
#define DEFAULT_HPDF_FIT_B HPDF_FIT_B
#define DEFAULT_HPDF_FIT_BH HPDF_FIT_BH
#define DEFAULT_HPDF_FIT_BV HPDF_FIT_BV 

/* AnnotType */
#define DEFAULT_HPDF_ANNOT_TEXT_NOTES HPDF_ANNOT_TEXT_NOTES 
#define DEFAULT_HPDF_ANNOT_LINK HPDF_ANNOT_LINK
#define DEFAULT_HPDF_ANNOT_SOUND HPDF_ANNOT_SOUND
#define DEFAULT_HPDF_ANNOT_FREE_TEXT HPDF_ANNOT_FREE_TEXT 
#define DEFAULT_HPDF_ANNOT_STAMP HPDF_ANNOT_STAMP
#define DEFAULT_HPDF_ANNOT_SQUARE HPDF_ANNOT_SQUARE
#define DEFAULT_HPDF_ANNOT_CIRCLE HPDF_ANNOT_CIRCLE
#define DEFAULT_HPDF_ANNOT_STRIKE_OUT HPDF_ANNOT_STRIKE_OUT
#define DEFAULT_HPDF_ANNOT_HIGHTLIGHT HPDF_ANNOT_HIGHTLIGHT
#define DEFAULT_HPDF_ANNOT_UNDERLINE HPDF_ANNOT_UNDERLINE
#define DEFAULT_HPDF_ANNOT_INK HPDF_ANNOT_INK
#define DEFAULT_HPDF_ANNOT_FILE_ATTACHMENT HPDF_ANNOT_FILE_ATTACHMENT
#define DEFAULT_HPDF_ANNOT_POPUP HPDF_ANNOT_POPUP
#define DEFAULT_HPDF_ANNOT_3D HPDF_ANNOT_3D 

/*  AnnotFlgs  */
#define DEFAULT_HPDF_ANNOT_INVISIBLE HPDF_ANNOT_INVISIBLE
#define DEFAULT_HPDF_ANNOT_HIDDEN HPDF_ANNOT_HIDDEN 
#define DEFAULT_HPDF_ANNOT_PRINT HPDF_ANNOT_PRINT
#define DEFAULT_HPDF_ANNOT_NOZOOM HPDF_ANNOT_NOZOOM
#define DEFAULT_HPDF_ANNOT_NOROTATE HPDF_ANNOT_NOROTATE
#define DEFAULT_HPDF_ANNOT_NOVIEW HPDF_ANNOT_NOVIEW
#define DEFAULT_HPDF_ANNOT_READONLY HPDF_ANNOT_READONLY

/* AnnotHighlightMode */
#define DEFAULT_HPDF_ANNOT_NO_HIGHTLIGHT HPDF_ANNOT_NO_HIGHTLIGHT 
#define DEFAULT_HPDF_ANNOT_INVERT_BOX HPDF_ANNOT_INVERT_BOX 
#define DEFAULT_HPDF_ANNOT_INVERT_BORDER HPDF_ANNOT_INVERT_BORDER 
#define DEFAULT_HPDF_ANNOT_DOWN_APPEARANCE HPDF_ANNOT_DOWN_APPEARANCE 

/* AnnotIcon */
#define DEFAULT_HPDF_ANNOT_ICON_COMMENT HPDF_ANNOT_ICON_COMMENT
#define DEFAULT_HPDF_ANNOT_ICON_KEY HPDF_ANNOT_ICON_KEY
#define DEFAULT_HPDF_ANNOT_ICON_NOTE HPDF_ANNOT_ICON_NOTE
#define DEFAULT_HPDF_ANNOT_ICON_HELP HPDF_ANNOT_ICON_HELP 
#define DEFAULT_HPDF_ANNOT_ICON_NEW_PARAGRAPH HPDF_ANNOT_ICON_NEW_PARAGRAPH 
#define DEFAULT_HPDF_ANNOT_ICON_PARAGRAPH HPDF_ANNOT_ICON_PARAGRAPH
#define DEFAULT_HPDF_ANNOT_ICON_INSERT HPDF_ANNOT_ICON_INSERT

/* BSSubtype */
#define DEFAULT_HPDF_BS_SOLID HPDF_BS_SOLID 
#define DEFAULT_HPDF_BS_DASHED HPDF_BS_DASHED
#define DEFAULT_HPDF_BS_BEVELED HPDF_BS_BEVELED
#define DEFAULT_HPDF_BS_INSET HPDF_BS_INSET 
#define DEFAULT_HPDF_BS_UNDERLINED HPDF_BS_UNDERLINED

/* BlendMode */
#define DEFAULT_HPDF_BM_NORMAL HPDF_BM_NORMAL 
#define DEFAULT_HPDF_BM_MULTIPLY HPDF_BM_MULTIPLY 
#define DEFAULT_HPDF_BM_SCREEN HPDF_BM_SCREEN 
#define DEFAULT_HPDF_BM_OVERLAY HPDF_BM_OVERLAY
#define DEFAULT_HPDF_BM_DARKEN HPDF_BM_DARKEN
#define DEFAULT_HPDF_BM_LIGHTEN HPDF_BM_LIGHTEN 
#define DEFAULT_HPDF_BM_COLOR_DODGE HPDF_BM_COLOR_DODGE
#define DEFAULT_HPDF_BM_COLOR_BUM HPDF_BM_COLOR_BUM
#define DEFAULT_HPDF_BM_HARD_LIGHT HPDF_BM_HARD_LIGHT 
#define DEFAULT_HPDF_BM_SOFT_LIGHT HPDF_BM_SOFT_LIGHT
#define DEFAULT_HPDF_BM_DIFFERENCE HPDF_BM_DIFFERENCE
#define DEFAULT_HPDF_BM_EXCLUSHON HPDF_BM_EXCLUSHON 

/* _TransitionStyle */
#define DEFAULT_HPDF_TS_WIPE_RIGHT HPDF_TS_WIPE_RIGHT
#define DEFAULT_HPDF_TS_WIPE_UP HPDF_TS_WIPE_UP 
#define DEFAULT_HPDF_TS_WIPE_LEFT HPDF_TS_WIPE_LEFT 
#define DEFAULT_HPDF_TS_WIPE_DOWN HPDF_TS_WIPE_DOWN
#define DEFAULT_HPDF_TS_BARN_DOORS_HORIZONTAL_OUT HPDF_TS_BARN_DOORS_HORIZONTAL_OUT
#define DEFAULT_HPDF_TS_BARN_DOORS_HORIZONTAL_IN HPDF_TS_BARN_DOORS_HORIZONTAL_IN
#define DEFAULT_HPDF_TS_BARN_DOORS_VERTICAL_OUT HPDF_TS_BARN_DOORS_VERTICAL_OUT 
#define DEFAULT_HPDF_TS_BARN_DOORS_VERTICAL_IN HPDF_TS_BARN_DOORS_VERTICAL_IN 
#define DEFAULT_HPDF_TS_BOX_OUT HPDF_TS_BOX_OUT 
#define DEFAULT_HPDF_TS_BOX_IN HPDF_TS_BOX_IN
#define DEFAULT_HPDF_TS_BLINDS_HORIZONTAL HPDF_TS_BLINDS_HORIZONTAL 
#define DEFAULT_HPDF_TS_BLINDS_VERTICAL HPDF_TS_BLINDS_VERTICAL 
#define DEFAULT_HPDF_TS_DISSOLVE HPDF_TS_DISSOLVE 
#define DEFAULT_HPDF_TS_GLITTER_RIGHT HPDF_TS_GLITTER_RIGHT 
#define DEFAULT_HPDF_TS_GLITTER_DOWN HPDF_TS_GLITTER_DOWN
#define DEFAULT_HPDF_TS_GLITTER_TOP_LEFT_TO_BOTTOM_RIGHT HPDF_TS_GLITTER_TOP_LEFT_TO_BOTTOM_RIGHT
#define DEFAULT_HPDF_TS_REPLACE HPDF_TS_REPLACE

/*  PageDirection */
#define DEFAULT_HPDF_PAGE_PORTRAIT HPDF_PAGE_PORTRAIT
#define DEFAULT_HPDF_PAGE_LANDSCAPE HPDF_PAGE_LANDSCAPE

/* EncoderType */
#define DEFAULT_HPDF_ENCODER_TYPE_SINGLE_BYTE HPDF_ENCODER_TYPE_SINGLE_BYTE 
#define DEFAULT_HPDF_ENCODER_TYPE_DOUBLE_BYTE HPDF_ENCODER_TYPE_DOUBLE_BYTE 
#define DEFAULT_HPDF_ENCODER_TYPE_UNINITIALIZED HPDF_ENCODER_TYPE_UNINITIALIZED 
#define DEFAULT_HPDF_ENCODER_UNKNOWN HPDF_ENCODER_UNKNOWN 

/* ByteType */
#define DEFAULT_HPDF_BYTE_TYPE_SINGLE HPDF_BYTE_TYPE_SINGLE
#define DEFAULT_HPDF_BYTE_TYPE_LEAD HPDF_BYTE_TYPE_LEAD
#define DEFAULT_HPDF_BYTE_TYPE_TRIAL HPDF_BYTE_TYPE_TRIAL 
#define DEFAULT_HPDF_BYTE_TYPE_UNKNOWN HPDF_BYTE_TYPE_UNKNOWN 

/* TextAlignment */
#define DEFAULT_HPDF_TALIGN_LEFT HPDF_TALIGN_LEFT 
#define DEFAULT_HPDF_TALIGN_RIGHT HPDF_TALIGN_RIGHT 
#define DEFAULT_HPDF_TALIGN_CENTER HPDF_TALIGN_CENTER 
#define DEFAULT_HPDF_TALIGN_JUSTIFY HPDF_TALIGN_JUSTIFY 


struct varinit {
	awk_scalar_t *spec;
	const char *name;
	int dfltval;
	int read_only;
};

#define ENTRY(VAR, CONST) { &VAR##_node, #VAR, DEFAULT_##VAR, CONST },

#define NUM_SCALARS     (sizeof(varinit)/sizeof(varinit[0]))
#define NUM_RESET       (NUM_SCALARS-1)
#define HANDLE_SIZE 32

/*  hpdf_consts.h
#define  HPDF_OK	 0
#define  HPDF_NOERROR	 0
*/
#define PDF_NOK (-1)
#define PDF_OK  (0)

#endif /* _PDF_H */

