/*
 * GQmpeg
 * (C) 2002 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#include "gqmpeg.h"
#include "japanese_tag.h"

#ifdef ENABLE_JAPANESE_TAGS

/*
  If Japanese tags extension has any troubles,
  please contact: Yuuki NINOMIYA <gm@debian.or.jp>

  These codes are from libjcode-2.00i which is licenced
  under the LGPL2/GPL2.

  libjcode.c Ver 1.0
   (C) Kuramitsu Kimio, Tokyo Univ. 1996-97
*/

#define ASCII       0
#define JIS         1
#define EUC         2
#define SJIS        3
#define NEW         4
#define OLD         5
#define NEC         6
#define EUCORSJIS   7
#define LF          10
#define FF          12
#define CR          13
#define ESC         27

#define CHAROUT(ch) *str2 = (unsigned char)(ch); str2++;
#define SJIS1(A)    ((A >= 129 && A <= 159) || (A >= 224 && A <= 239))
#define SJIS2(A)    (A >= 64 && A <= 252)

static void _sjis_shift(int *p1, int *p2);
static unsigned char *_skip_esc(unsigned char *str, int *esc_in);
static void _shift2euc(unsigned char *str, unsigned char *str2);
static void _seven2euc(unsigned char *str, unsigned char *str2);

static int _detect(unsigned char *str, int expected);
static int _detect_euc_or_sjis(unsigned char *str);
static int detect_kanji_code(char *str);

static void _sjis_shift(int *p1, int *p2)
{
	unsigned char c1 = *p1;
	unsigned char c2 = *p2;
	int adjust = c2 < 159;
	int rowOffset = c1 < 160 ? 112 : 176;
	int cellOffset = adjust ? (c2 > 127 ? 32 : 31) : 126;

	*p1 = ((c1 - rowOffset) << 1) - adjust;
	*p2 -= cellOffset;
}

static unsigned char *_skip_esc(unsigned char *str, int *esc_in)
{
	int c;
  
	c = (int)*(++str);
	if ((c == '$') || (c == '(')) str++;
	if ((c == 'K') || (c == '$')) *esc_in = TRUE;
	else *esc_in = FALSE;

	if(*str != '\0') str++;
	return str;
}

static void _shift2euc(unsigned char *str, unsigned char *str2)
{
	int p1,p2;
  
	while ((p1 = (int)*str) != '\0') {
		if (SJIS1(p1)) {
			if((p2 = (int)*(++str)) == '\0') break;
			if (SJIS2(p2)) {
				_sjis_shift(&p1,&p2);
				p1 += 128;
				p2 += 128;
			}
			CHAROUT(p1);
			CHAROUT(p2);
			str++;
			continue;
		}

		/* put SB-SJIS as is, with SS2 */
		if ((p1 >= 161) && (p1 <= 223))
		{
			CHAROUT(0x8e);
			CHAROUT(p1);
			str++;
			continue;
		}
		CHAROUT(p1);
		str++;
	}
	*str2='\0';
}

static void _seven2euc(unsigned char *str, unsigned char *str2)
{
	int p1, esc_in = FALSE;

	while ((p1 = (int)*str) != '\0') {
		/* skip escape sequence */
		if (p1 == ESC) {
			str = _skip_esc(str, &esc_in);
			continue;
		}

		if ((p1 == LF) || (p1 == CR) || (p1 <= 32)){
			if (esc_in) esc_in = FALSE;
		}

		if(esc_in) { /* ISO-2022-JP */
			CHAROUT(p1 + 128); 

			if((p1 = (int)*(++str)) == '\0') break;
			if(p1 <= 32) {
				CHAROUT(p1);
			} else CHAROUT(p1 + 128);
		}else{ /* ASCII */
			CHAROUT(p1);
		}
		str++;
	}
	*str2 = '\0';
}

static int _detect(unsigned char *str, int expected)
{
	register int c;

	while((c = (int)*str) != '\0') {

		/* JIS */
		if(c == ESC) {
			if((c = (int)*(++str)) == '\0') return expected; 
			if (c == '$') {
				if((c = (int)*(++str)) == '\0') return expected; 
			        /* ESC $ B --> new JIS 
				   ESC $ @ --> old JIS */
				if (c == 'B' || c == '@') return JIS;
			}
			if (c == 'K')
				return NEC; /* ESC K --> NEC JIS */
			str++;
			continue;
		}

		/* SJIS */
		if ((c >= 129 && c <= 141) || (c >= 143 && c <= 159))
			return SJIS;

		/* SS2 */
		if (c == 142) {
			if((c = (int)*(++str)) == '\0') return expected; 
			if ((c >= 64 && c <= 126) || (c >= 128 && c <= 160) || 
			    (c >= 224 && c <= 252)) return SJIS;
			if (c >= 161 && c <= 223) expected = EUCORSJIS;
			str++;
			continue;
		}

		/* SJIS or EUC */
		/* if euc or 1byte katakana sjis */
		if (c >= 161 && c <= 223) {            /* next is euc 2nd or other 1st */
			expected = EUCORSJIS;
			while (c >=161 && c <= 252) {
				if((c = (int)*(++str)) == '\0') return SJIS; /* not euc 2nd */
				if (c <= 159) return SJIS;              /* ascii, not euc 2nd */
				if (c == 160) return SJIS;              /* Not euc */
				if (c >= 240 && c <= 254) return EUC;   /* must be euc 2nd */
				if (c >= 161 && c <= 223)               /* euc 2nd or HANKATA */
					break;                          /* cannot determin, goto top loop */
				if (c >= 224 && c <= 239) {             /* euc 2nd or MBSJIS 1st */
					if ((c = (int)*(++str)) == '\0') return EUC;   /* not sjis 2nd */
					if (c <= 63) return EUC;        /* ascii, not sjis 2nd */
					if (c == 127) return EUC;
					if (c == 142) {                 /* SS2 or MBSJIS 2nd */
						if ((c = (unsigned int)*(++str)) <= 0xa0)
							return SJIS;    /* not SS2 */
						else if (c <= 0xdf) {   /* SS2(2) or SBSJIS */
							break;
						}
						else if (c <= 0xef)     /* MBSJIS(1) not SS2 */
							return SJIS;
						else                    /* 0xf0 - 0xff, can't happen */
							return expected;
					}
					if ((c >= 64 && c <= 126) || (c == 128)) {
						/* sjis 2nd or ascii */
						break;
					}
					if ((c >= 129 && c<= 141) || (c >= 143 && c <= 160)) return SJIS;
					/* cannot be euc 1st */
					if (c >= 253 && c <= 254) return EUC; /* not sjis 2nd */
					/* if (c >= 161 && c <= 252)  can be euc 1st or sjis 2nd */
					/* retry this loop */
				}
				if (c == 255) return expected;
			}
		}

		if (c >= 224 && c <= 239) {                         /* MBSJIS or euc 1st */
			if ((c = (int)*(++str)) == '\0') return expected; /* illegal */
			if ((c >= 64 && c <= 126) || (c >= 128 && c <= 160))
				return SJIS;
			if (c >= 253 && c <= 254) return EUC;
			if (c >= 161 && c <= 252) {
				expected = EUCORSJIS;
				str++;
				continue;
			}
		}

		if (c >= 240 && c <= 254) {
			return EUC;
		}

		/* Pass ASCII, 0xff, 0xa0 */
		str++;
	}
	return expected;
}

static int _detect_euc_or_sjis(unsigned char *str)
{
	int c1, c2;
	int euc_c = 0, sjis_c = 0;
	unsigned char *ptr;
	static int expected = EUCORSJIS;

	ptr = str, c2 = 0;
	while ((c1 = (int)*ptr++) != '\0') {
		if (((c2 >  0x80 && c2 < 0xa0) || (c2 >= 0xe0 && c2 < 0xfd)) &&
		    ((c1 >= 0x40 && c1 < 0x7f) || (c1 >= 0x80 && c1 < 0xfd)))
			sjis_c++, c1 = *ptr++;
		c2 = c1;
	}
	if (sjis_c == 0)
		expected = EUC;
	else {
		ptr = str, c2 = 0;
		while ((c1 = (int)*ptr++) != '\0') {
			if ((c2 > 0xa0  && c2 < 0xff) &&
			    (c1 > 0xa0  && c1 < 0xff))
				euc_c++, c1 = *ptr++;
			c2 = c1;
		}
		if (sjis_c > euc_c)
			expected = SJIS;
		else
			expected = EUC;
	}
	return expected;
}

static int detect_kanji_code(char *str)
{
	static int detected = ASCII;

	if(!str) return (0);

	detected = _detect((unsigned char *)str, ASCII);

	if(detected == NEW || detected == OLD || detected == NEC)
		return JIS;

	if(detected == EUCORSJIS)
		detected = _detect_euc_or_sjis((unsigned char *)str);

	return detected;
}

char *to_string_euc(char *str)
{
	int detected;
	char *buf, *ret;

	if(!str) return (NULL);
	detected = detect_kanji_code(str);
	if(detected == ASCII || detected == EUC) return strdup(str);

	buf = (char*)g_malloc(strlen(str)*2);
	if (!buf) return NULL;

	switch(detected) {
	case SJIS :
		_shift2euc((unsigned char *)str, (unsigned char *)buf);
		break;
	case JIS :
	case NEW : case OLD : case NEC :
		_seven2euc((unsigned char *)str, (unsigned char *)buf);
		break;
	default:
		g_free(buf);
		return g_strdup(str);
		break;
	}

	ret = g_strdup(buf);
	g_free(buf);
	return ret;
}

#endif /* ENABLE_JAPANESE_TAGS */
