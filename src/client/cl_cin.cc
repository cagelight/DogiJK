/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

/*****************************************************************************
 * name:		cl_cin.c
 *
 * desc:		video and cinematic playback
 *
 * $Archive: /MissionPack/code/client/cl_cin.c $
 * $Author: osman $
 * $Revision: 1.4 $
 * $Modtime: 6/12/01 10:36a $
 * $Date: 2003/03/15 23:43:59 $
 *
 * cl_glconfig.hwtype trtypes 3dfx/ragepro need 256x256
 *
 *****************************************************************************/

#include "qcommon/q_math2.hh"
#include "client.hh"
#include "cl_uiapi.hh"
#include "snd_local.hh"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

//-----------------------------------------------------------------------------
// RllDecodeMonoToMono
//
// Decode mono source data into a mono buffer.
//
// Parameters:	from -> buffer holding encoded data
//				to ->	buffer to hold decoded data
//				size =	number of bytes of input (= # of shorts of output)
//				signedOutput = 0 for unsigned output, non-zero for signed output
//				flag = flags from asset header
//
// Returns:		Number of samples placed in output buffer
//-----------------------------------------------------------------------------
/*
static long RllDecodeMonoToMono(unsigned char *from,short *to,unsigned int size,char signedOutput ,unsigned short flag)
{
	unsigned int z;
	int prev;

	if (signedOutput)
		prev =  flag - 0x8000;
	else
		prev = flag;

	for (z=0;z<size;z++) {
		prev = to[z] = (short)(prev + cin.sqrTable[from[z]]);
	}
	return size;	// *sizeof(short));
}
*/


//-----------------------------------------------------------------------------
// RllDecodeMonoToStereo
//
// Decode mono source data into a stereo buffer. Output is 4 times the number
// of bytes in the input.
//
// Parameters:	from -> buffer holding encoded data
//				to ->	buffer to hold decoded data
//				size =	number of bytes of input (= 1/4 # of bytes of output)
//				signedOutput = 0 for unsigned output, non-zero for signed output
//				flag = flags from asset header
//
// Returns:		Number of samples placed in output buffer
//-----------------------------------------------------------------------------
/*
static long RllDecodeMonoToStereo(unsigned char *from,short *to,unsigned int size,char signedOutput,unsigned short flag)
{
	unsigned int z;
	int prev;

	if (signedOutput)
		prev =  flag - 0x8000;
	else
		prev = flag;

	for (z = 0; z < size; z++) {
		prev = (short)(prev + cin.sqrTable[from[z]]);
		to[z*2+0] = to[z*2+1] = (short)(prev);
	}

	return size;	// * 2 * sizeof(short));
}
*/

//-----------------------------------------------------------------------------
// RllDecodeStereoToStereo
//
// Decode stereo source data into a stereo buffer.
//
// Parameters:	from -> buffer holding encoded data
//				to ->	buffer to hold decoded data
//				size =	number of bytes of input (= 1/2 # of bytes of output)
//				signedOutput = 0 for unsigned output, non-zero for signed output
//				flag = flags from asset header
//
// Returns:		Number of samples placed in output buffer
//-----------------------------------------------------------------------------
/*
static long RllDecodeStereoToStereo(unsigned char *from,short *to,unsigned int size,char signedOutput, unsigned short flag)
{
	unsigned int z;
	unsigned char *zz = from;
	int	prevL, prevR;

	if (signedOutput) {
		prevL = (flag & 0xff00) - 0x8000;
		prevR = ((flag & 0x00ff) << 8) - 0x8000;
	} else {
		prevL = flag & 0xff00;
		prevR = (flag & 0x00ff) << 8;
	}

	for (z=0;z<size;z+=2) {
                prevL = (short)(prevL + cin.sqrTable[*zz++]);
                prevR = (short)(prevR + cin.sqrTable[*zz++]);
                to[z+0] = (short)(prevL);
                to[z+1] = (short)(prevR);
	}

	return (size>>1);	// *sizeof(short));
}
*/

//-----------------------------------------------------------------------------
// RllDecodeStereoToMono
//
// Decode stereo source data into a mono buffer.
//
// Parameters:	from -> buffer holding encoded data
//				to ->	buffer to hold decoded data
//				size =	number of bytes of input (= # of bytes of output)
//				signedOutput = 0 for unsigned output, non-zero for signed output
//				flag = flags from asset header
//
// Returns:		Number of samples placed in output buffer
//-----------------------------------------------------------------------------
/*
static long RllDecodeStereoToMono(unsigned char *from,short *to,unsigned int size,char signedOutput, unsigned short flag)
{
	unsigned int z;
	int prevL,prevR;

	if (signedOutput) {
		prevL = (flag & 0xff00) - 0x8000;
		prevR = ((flag & 0x00ff) << 8) -0x8000;
	} else {
		prevL = flag & 0xff00;
		prevR = (flag & 0x00ff) << 8;
	}

	for (z=0;z<size;z+=1) {
		prevL= prevL + cin.sqrTable[from[z*2]];
		prevR = prevR + cin.sqrTable[from[z*2+1]];
		to[z] = (short)((prevL + prevR)/2);
	}

	return size;
}
*/
/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void move8_32( byte *src, byte *dst, int spl )
{
	int i;

	for(i = 0; i < 8; ++i)
	{
		memcpy(dst, src, 32);
		src += spl;
		dst += spl;
	}
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void move4_32( byte *src, byte *dst, int spl  )
{
	int i;

	for(i = 0; i < 4; ++i)
	{
		memcpy(dst, src, 16);
		src += spl;
		dst += spl;
	}
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void blit8_32( byte *src, byte *dst, int spl  )
{
	int i;

	for(i = 0; i < 8; ++i)
	{
		memcpy(dst, src, 32);
		src += 32;
		dst += spl;
	}
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void blit4_32( byte *src, byte *dst, int spl  )
{
	int i;

	for(i = 0; i < 4; ++i)
	{
		memmove(dst, src, 16);
		src += 16;
		dst += spl;
	}
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void blit2_32( byte *src, byte *dst, int spl  )
{
	memcpy(dst, src, 8);
	memcpy(dst+spl, src+8, 8);
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void blitVQQuad32fs( byte **status, unsigned char *data )
{
unsigned short	newd, celdata, code;
unsigned int	index, i;
int		spl;

	newd	= 0;
	celdata = 0;
	index	= 0;

        spl = cinTable[currentHandle].samplesPerLine;

	do {
		if (!newd) {
			newd = 7;
			celdata = data[0] + data[1]*256;
			data += 2;
		} else {
			newd--;
		}

		code = (unsigned short)(celdata&0xc000);
		celdata <<= 2;

		switch (code) {
			case	0x8000:													// vq code
				blit8_32( (byte *)&vq8[(*data)*128], status[index], spl );
				data++;
				index += 5;
				break;
			case	0xc000:													// drop
				index++;													// skip 8x8
				for(i=0;i<4;i++) {
					if (!newd) {
						newd = 7;
						celdata = data[0] + data[1]*256;
						data += 2;
					} else {
						newd--;
					}

					code = (unsigned short)(celdata&0xc000); celdata <<= 2;

					switch (code) {											// code in top two bits of code
						case	0x8000:										// 4x4 vq code
							blit4_32( (byte *)&vq4[(*data)*32], status[index], spl );
							data++;
							break;
						case	0xc000:										// 2x2 vq code
							blit2_32( (byte *)&vq2[(*data)*8], status[index], spl );
							data++;
							blit2_32( (byte *)&vq2[(*data)*8], status[index]+8, spl );
							data++;
							blit2_32( (byte *)&vq2[(*data)*8], status[index]+spl*2, spl );
							data++;
							blit2_32( (byte *)&vq2[(*data)*8], status[index]+spl*2+8, spl );
							data++;
							break;
						case	0x4000:										// motion compensation
							move4_32( status[index] + cin.mcomp[(*data)], status[index], spl );
							data++;
							break;
					}
					index++;
				}
				break;
			case	0x4000:													// motion compensation
				move8_32( status[index] + cin.mcomp[(*data)], status[index], spl );
				data++;
				index += 5;
				break;
			case	0x0000:
				index += 5;
				break;
		}
	} while ( status[index] != NULL );
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void ROQ_GenYUVTables( void )
{
	float t_ub,t_vr,t_ug,t_vg;
	long i;

	t_ub = (1.77200f/2.0f) * (float)(1<<6) + 0.5f;
	t_vr = (1.40200f/2.0f) * (float)(1<<6) + 0.5f;
	t_ug = (0.34414f/2.0f) * (float)(1<<6) + 0.5f;
	t_vg = (0.71414f/2.0f) * (float)(1<<6) + 0.5f;
	for(i=0;i<256;i++) {
		float x = (float)(2 * i - 255);

		ROQ_UB_tab[i] = (long)( ( t_ub * x) + (1<<5));
		ROQ_VR_tab[i] = (long)( ( t_vr * x) + (1<<5));
		ROQ_UG_tab[i] = (long)( (-t_ug * x)		 );
		ROQ_VG_tab[i] = (long)( (-t_vg * x) + (1<<5));
		ROQ_YY_tab[i] = (long)( (i << 6) | (i >> 2) );
	}
}

#define VQ2TO4(a,b,c,d) { \
    	*c++ = a[0];	\
	*d++ = a[0];	\
	*d++ = a[0];	\
	*c++ = a[1];	\
	*d++ = a[1];	\
	*d++ = a[1];	\
	*c++ = b[0];	\
	*d++ = b[0];	\
	*d++ = b[0];	\
	*c++ = b[1];	\
	*d++ = b[1];	\
	*d++ = b[1];	\
	*d++ = a[0];	\
	*d++ = a[0];	\
	*d++ = a[1];	\
	*d++ = a[1];	\
	*d++ = b[0];	\
	*d++ = b[0];	\
	*d++ = b[1];	\
	*d++ = b[1];	\
	a += 2; b += 2; }

#define VQ2TO2(a,b,c,d) { \
	*c++ = *a;	\
	*d++ = *a;	\
	*d++ = *a;	\
	*c++ = *b;	\
	*d++ = *b;	\
	*d++ = *b;	\
	*d++ = *a;	\
	*d++ = *a;	\
	*d++ = *b;	\
	*d++ = *b;	\
	a++; b++; }
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static unsigned short yuv_to_rgb( long y, long u, long v )
{
	long r,g,b,YY = (long)(ROQ_YY_tab[(y)]);

	r = (YY + ROQ_VR_tab[v]) >> 9;
	g = (YY + ROQ_UG_tab[u] + ROQ_VG_tab[v]) >> 8;
	b = (YY + ROQ_UB_tab[u]) >> 9;

	if (r<0)
		r = 0;
	if (g<0)
		g = 0;
	if (b<0)
		b = 0;
	if (r > 31)
		r = 31;
	if (g > 63)
		g = 63;
	if (b > 31)
		b = 31;

	return (unsigned short)((r<<11)+(g<<5)+(b));
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/
/*
static unsigned int yuv_to_rgb24( long y, long u, long v )
{
	long r,g,b,YY = (long)(ROQ_YY_tab[(y)]);

	r = (YY + ROQ_VR_tab[v]) >> 6;
	g = (YY + ROQ_UG_tab[u] + ROQ_VG_tab[v]) >> 6;
	b = (YY + ROQ_UB_tab[u]) >> 6;

	if (r<0)
		r = 0;
	if (g<0)
		g = 0;
	if (b<0)
		b = 0;
	if (r > 255)
		r = 255;
	if (g > 255)
		g = 255;
	if (b > 255)
		b = 255;

	return LittleLong ((r)|(g<<8)|(b<<16)|(255<<24));
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void decodeCodeBook( byte *input, unsigned short roq_flags )
{
	long	i, j, two, four;
	unsigned short	*aptr, *bptr, *cptr, *dptr;
	long	y0,y1,y2,y3,cr,cb;
	byte	*bbptr, *baptr, *bcptr, *bdptr;
	union {
		unsigned int *i;
		unsigned short *s;
	} iaptr, ibptr, icptr, idptr;

	if (!roq_flags) {
		two = four = 256;
	} else {
		two  = roq_flags>>8;
		if (!two) two = 256;
		four = roq_flags&0xff;
	}

	four *= 2;

	bptr = (unsigned short *)vq2;

	if (!cinTable[currentHandle].half) {
		if (!cinTable[currentHandle].smootheddouble) {
//
// normal height
//
			if (cinTable[currentHandle].samplesPerPixel==2) {
				for(i=0;i<two;i++) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input++;
					cr = (long)*input++;
					cb = (long)*input++;
					*bptr++ = yuv_to_rgb( y0, cr, cb );
					*bptr++ = yuv_to_rgb( y1, cr, cb );
					*bptr++ = yuv_to_rgb( y2, cr, cb );
					*bptr++ = yuv_to_rgb( y3, cr, cb );
				}

				cptr = (unsigned short *)vq4;
				dptr = (unsigned short *)vq8;

				for(i=0;i<four;i++) {
					aptr = (unsigned short *)vq2 + (*input++)*4;
					bptr = (unsigned short *)vq2 + (*input++)*4;
					for(j=0;j<2;j++)
						VQ2TO4(aptr,bptr,cptr,dptr);
				}
			} else if (cinTable[currentHandle].samplesPerPixel==4) {
				ibptr.s = bptr;
				for(i=0;i<two;i++) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input++;
					cr = (long)*input++;
					cb = (long)*input++;
					*ibptr.i++ = yuv_to_rgb24( y0, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y1, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y2, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y3, cr, cb );
				}

				icptr.s = vq4;
				idptr.s = vq8;

				for(i=0;i<four;i++) {
					iaptr.s = vq2;
					iaptr.i += (*input++)*4;
					ibptr.s = vq2;
					ibptr.i += (*input++)*4;
					for(j=0;j<2;j++)
						VQ2TO4(iaptr.i, ibptr.i, icptr.i, idptr.i);
				}
			} else if (cinTable[currentHandle].samplesPerPixel==1) {
				bbptr = (byte *)bptr;
				for(i=0;i<two;i++) {
					*bbptr++ = cinTable[currentHandle].gray[*input++];
					*bbptr++ = cinTable[currentHandle].gray[*input++];
					*bbptr++ = cinTable[currentHandle].gray[*input++];
					*bbptr++ = cinTable[currentHandle].gray[*input]; input +=3;
				}

				bcptr = (byte *)vq4;
				bdptr = (byte *)vq8;

				for(i=0;i<four;i++) {
					baptr = (byte *)vq2 + (*input++)*4;
					bbptr = (byte *)vq2 + (*input++)*4;
					for(j=0;j<2;j++)
						VQ2TO4(baptr,bbptr,bcptr,bdptr);
				}
			}
		} else {
//
// double height, smoothed
//
			if (cinTable[currentHandle].samplesPerPixel==2) {
				for(i=0;i<two;i++) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input++;
					cr = (long)*input++;
					cb = (long)*input++;
					*bptr++ = yuv_to_rgb( y0, cr, cb );
					*bptr++ = yuv_to_rgb( y1, cr, cb );
					*bptr++ = yuv_to_rgb( ((y0*3)+y2)/4, cr, cb );
					*bptr++ = yuv_to_rgb( ((y1*3)+y3)/4, cr, cb );
					*bptr++ = yuv_to_rgb( (y0+(y2*3))/4, cr, cb );
					*bptr++ = yuv_to_rgb( (y1+(y3*3))/4, cr, cb );
					*bptr++ = yuv_to_rgb( y2, cr, cb );
					*bptr++ = yuv_to_rgb( y3, cr, cb );
				}

				cptr = (unsigned short *)vq4;
				dptr = (unsigned short *)vq8;

				for(i=0;i<four;i++) {
					aptr = (unsigned short *)vq2 + (*input++)*8;
					bptr = (unsigned short *)vq2 + (*input++)*8;
					for(j=0;j<2;j++) {
						VQ2TO4(aptr,bptr,cptr,dptr);
						VQ2TO4(aptr,bptr,cptr,dptr);
					}
				}
			} else if (cinTable[currentHandle].samplesPerPixel==4) {
				ibptr.s = bptr;
				for(i=0;i<two;i++) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input++;
					cr = (long)*input++;
					cb = (long)*input++;
					*ibptr.i++ = yuv_to_rgb24( y0, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y1, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( ((y0*3)+y2)/4, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( ((y1*3)+y3)/4, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( (y0+(y2*3))/4, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( (y1+(y3*3))/4, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y2, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y3, cr, cb );
				}

				icptr.s = vq4;
				idptr.s = vq8;

				for(i=0;i<four;i++) {
					iaptr.s = vq2;
					iaptr.i += (*input++)*8;
					ibptr.s = vq2;
					ibptr.i += (*input++)*8;
					for(j=0;j<2;j++) {
						VQ2TO4(iaptr.i, ibptr.i, icptr.i, idptr.i);
						VQ2TO4(iaptr.i, ibptr.i, icptr.i, idptr.i);
					}
				}
			} else if (cinTable[currentHandle].samplesPerPixel==1) {
				bbptr = (byte *)bptr;
				for(i=0;i<two;i++) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input; input+= 3;
					*bbptr++ = cinTable[currentHandle].gray[y0];
					*bbptr++ = cinTable[currentHandle].gray[y1];
					*bbptr++ = cinTable[currentHandle].gray[((y0*3)+y2)/4];
					*bbptr++ = cinTable[currentHandle].gray[((y1*3)+y3)/4];
					*bbptr++ = cinTable[currentHandle].gray[(y0+(y2*3))/4];
					*bbptr++ = cinTable[currentHandle].gray[(y1+(y3*3))/4];
					*bbptr++ = cinTable[currentHandle].gray[y2];
					*bbptr++ = cinTable[currentHandle].gray[y3];
				}

				bcptr = (byte *)vq4;
				bdptr = (byte *)vq8;

				for(i=0;i<four;i++) {
					baptr = (byte *)vq2 + (*input++)*8;
					bbptr = (byte *)vq2 + (*input++)*8;
					for(j=0;j<2;j++) {
						VQ2TO4(baptr,bbptr,bcptr,bdptr);
						VQ2TO4(baptr,bbptr,bcptr,bdptr);
					}
				}
			}
		}
	} else {
//
// 1/4 screen
//
		if (cinTable[currentHandle].samplesPerPixel==2) {
			for(i=0;i<two;i++) {
				y0 = (long)*input; input+=2;
				y2 = (long)*input; input+=2;
				cr = (long)*input++;
				cb = (long)*input++;
				*bptr++ = yuv_to_rgb( y0, cr, cb );
				*bptr++ = yuv_to_rgb( y2, cr, cb );
			}

			cptr = (unsigned short *)vq4;
			dptr = (unsigned short *)vq8;

			for(i=0;i<four;i++) {
				aptr = (unsigned short *)vq2 + (*input++)*2;
				bptr = (unsigned short *)vq2 + (*input++)*2;
				for(j=0;j<2;j++) {
					VQ2TO2(aptr,bptr,cptr,dptr);
				}
			}
		} else if (cinTable[currentHandle].samplesPerPixel == 1) {
			bbptr = (byte *)bptr;

			for(i=0;i<two;i++) {
				*bbptr++ = cinTable[currentHandle].gray[*input]; input+=2;
				*bbptr++ = cinTable[currentHandle].gray[*input]; input+=4;
			}

			bcptr = (byte *)vq4;
			bdptr = (byte *)vq8;

			for(i=0;i<four;i++) {
				baptr = (byte *)vq2 + (*input++)*2;
				bbptr = (byte *)vq2 + (*input++)*2;
				for(j=0;j<2;j++) {
					VQ2TO2(baptr,bbptr,bcptr,bdptr);
				}
			}
		} else if (cinTable[currentHandle].samplesPerPixel == 4) {
			ibptr.s = bptr;
			for(i=0;i<two;i++) {
				y0 = (long)*input; input+=2;
				y2 = (long)*input; input+=2;
				cr = (long)*input++;
				cb = (long)*input++;
				*ibptr.i++ = yuv_to_rgb24( y0, cr, cb );
				*ibptr.i++ = yuv_to_rgb24( y2, cr, cb );
			}

			icptr.s = vq4;
			idptr.s = vq8;

			for(i=0;i<four;i++) {
				iaptr.s = vq2;
				iaptr.i += (*input++)*2;
				ibptr.s = vq2 + (*input++)*2;
				ibptr.i += (*input++)*2;
				for(j=0;j<2;j++) {
					VQ2TO2(iaptr.i,ibptr.i,icptr.i,idptr.i);
				}
			}
		}
	}
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void recurseQuad( long startX, long startY, long quadSize, long xOff, long yOff )
{
	byte *scroff;
	long bigx, bigy, lowx, lowy, useY;
	long offset;

	offset = cinTable[currentHandle].screenDelta;

	lowx = lowy = 0;
	bigx = cinTable[currentHandle].xsize;
	bigy = cinTable[currentHandle].ysize;

	if (bigx > cinTable[currentHandle].CIN_WIDTH) bigx = cinTable[currentHandle].CIN_WIDTH;
	if (bigy > cinTable[currentHandle].CIN_HEIGHT) bigy = cinTable[currentHandle].CIN_HEIGHT;

	if ( (startX >= lowx) && (startX+quadSize) <= (bigx) && (startY+quadSize) <= (bigy) && (startY >= lowy) && quadSize <= MAXSIZE) {
		useY = startY;
		scroff = cin.linbuf + (useY+((cinTable[currentHandle].CIN_HEIGHT-bigy)>>1)+yOff)*(cinTable[currentHandle].samplesPerLine) + (((startX+xOff))*cinTable[currentHandle].samplesPerPixel);

		cin.qStatus[0][cinTable[currentHandle].onQuad  ] = scroff;
		cin.qStatus[1][cinTable[currentHandle].onQuad++] = scroff+offset;
	}

	if ( quadSize != MINSIZE ) {
		quadSize >>= 1;
		recurseQuad( startX,		  startY		  , quadSize, xOff, yOff );
		recurseQuad( startX+quadSize, startY		  , quadSize, xOff, yOff );
		recurseQuad( startX,		  startY+quadSize , quadSize, xOff, yOff );
		recurseQuad( startX+quadSize, startY+quadSize , quadSize, xOff, yOff );
	}
}
*/


/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void setupQuad( long xOff, long yOff )
{
	long numQuadCels, i,x,y;
	byte *temp;

	if (xOff == cin.oldXOff && yOff == cin.oldYOff && cinTable[currentHandle].ysize == (unsigned)cin.oldysize && cinTable[currentHandle].xsize == (unsigned)cin.oldxsize) {
		return;
	}

	cin.oldXOff = xOff;
	cin.oldYOff = yOff;
	cin.oldysize = cinTable[currentHandle].ysize;
	cin.oldxsize = cinTable[currentHandle].xsize;
	// Enisform: Not in q3 source
	// numQuadCels  = (cinTable[currentHandle].CIN_WIDTH*cinTable[currentHandle].CIN_HEIGHT) / (16);
	// numQuadCels += numQuadCels/4 + numQuadCels/16;
	// numQuadCels += 64;							  // for overflow

	numQuadCels  = (cinTable[currentHandle].xsize*cinTable[currentHandle].ysize) / (16);
	numQuadCels += numQuadCels/4;
	numQuadCels += 64;							  // for overflow

	cinTable[currentHandle].onQuad = 0;

	for(y=0;y<(long)cinTable[currentHandle].ysize;y+=16)
		for(x=0;x<(long)cinTable[currentHandle].xsize;x+=16)
			recurseQuad( x, y, 16, xOff, yOff );

	temp = NULL;

	for(i=(numQuadCels-64);i<numQuadCels;i++) {
		cin.qStatus[0][i] = temp;			  // eoq
		cin.qStatus[1][i] = temp;			  // eoq
	}
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void readQuadInfo( byte *qData )
{
	if (currentHandle < 0) return;

	cinTable[currentHandle].xsize    = qData[0]+qData[1]*256;
	cinTable[currentHandle].ysize    = qData[2]+qData[3]*256;
	cinTable[currentHandle].maxsize  = qData[4]+qData[5]*256;
	cinTable[currentHandle].minsize  = qData[6]+qData[7]*256;

	cinTable[currentHandle].CIN_HEIGHT = cinTable[currentHandle].ysize;
	cinTable[currentHandle].CIN_WIDTH  = cinTable[currentHandle].xsize;

	cinTable[currentHandle].samplesPerLine = cinTable[currentHandle].CIN_WIDTH*cinTable[currentHandle].samplesPerPixel;
	cinTable[currentHandle].screenDelta = cinTable[currentHandle].CIN_HEIGHT*cinTable[currentHandle].samplesPerLine;

	cinTable[currentHandle].half = qfalse;
	cinTable[currentHandle].smootheddouble = qfalse;

	cinTable[currentHandle].VQ0 = cinTable[currentHandle].VQNormal;
	cinTable[currentHandle].VQ1 = cinTable[currentHandle].VQBuffer;

	cinTable[currentHandle].t[0] = cinTable[currentHandle].screenDelta;
	cinTable[currentHandle].t[1] = -cinTable[currentHandle].screenDelta;

	cinTable[currentHandle].drawX = cinTable[currentHandle].CIN_WIDTH;
	cinTable[currentHandle].drawY = cinTable[currentHandle].CIN_HEIGHT;
	// jic the card sucks
	if ( cls.vidconfig.maxTextureSize <= 256) {
        if (cinTable[currentHandle].drawX>256) {
            cinTable[currentHandle].drawX = 256;
        }
        if (cinTable[currentHandle].drawY>256) {
            cinTable[currentHandle].drawY = 256;
        }
		if (cinTable[currentHandle].CIN_WIDTH != 256 || cinTable[currentHandle].CIN_HEIGHT != 256) {
			Com_Printf("HACK: approxmimating cinematic for Rage Pro or Voodoo\n");
		}
	}
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void RoQPrepMcomp( long xoff, long yoff )
{
	long i, j, x, y, temp, temp2;

	i=cinTable[currentHandle].samplesPerLine; j=cinTable[currentHandle].samplesPerPixel;
	if ( cinTable[currentHandle].xsize == (cinTable[currentHandle].ysize*4) && !cinTable[currentHandle].half ) { j = j+j; i = i+i; }

	for(y=0;y<16;y++) {
		temp2 = (y+yoff-8)*i;
		for(x=0;x<16;x++) {
			temp = (x+xoff-8)*j;
			cin.mcomp[(x*16)+y] = cinTable[currentHandle].normalBuffer0-(temp2+temp);
		}
	}
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void initRoQ( void )
{
	if (currentHandle < 0) return;

	cinTable[currentHandle].VQNormal = (void (*)(byte *, void *))blitVQQuad32fs;
	cinTable[currentHandle].VQBuffer = (void (*)(byte *, void *))blitVQQuad32fs;
	cinTable[currentHandle].samplesPerPixel = 4;
	ROQ_GenYUVTables();
	RllSetupTable();
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/
/*
static byte* RoQFetchInterlaced( byte *source ) {
	int x, *src, *dst;

	if (currentHandle < 0) return NULL;

	src = (int *)source;
	dst = (int *)cinTable[currentHandle].buf2;

	for(x=0;x<256*256;x++) {
		*dst = *src;
		dst++; src += 2;
	}
	return cinTable[currentHandle].buf2;
}
*/
/*
static void RoQReset( void ) {

	if (currentHandle < 0) return;

	FS_FCloseFile( cinTable[currentHandle].iFile );
	FS_FOpenFileRead (cinTable[currentHandle].fileName, &cinTable[currentHandle].iFile, qtrue);
	// let the background thread start reading ahead
	FS_Read (cin.file, 16, cinTable[currentHandle].iFile);
	RoQ_init();
	cinTable[currentHandle].status = FMV_LOOPED;
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void RoQInterrupt(void)
{
	byte				*framedata;
        short		sbuf[32768];
        int		ssize;

	if (currentHandle < 0) return;

	FS_Read( cin.file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile );
	if ( cinTable[currentHandle].RoQPlayed >= cinTable[currentHandle].ROQSize ) {
		if (cinTable[currentHandle].holdAtEnd==qfalse) {
			if (cinTable[currentHandle].looping) {
				RoQReset();
			} else {
				cinTable[currentHandle].status = FMV_EOF;
			}
		} else {
			cinTable[currentHandle].status = FMV_IDLE;
		}
		return;
	}

	framedata = cin.file;
//
// new frame is ready
//
redump:
	switch(cinTable[currentHandle].roq_id)
	{
		case	ROQ_QUAD_VQ:
			if ((cinTable[currentHandle].numQuads&1)) {
				cinTable[currentHandle].normalBuffer0 = cinTable[currentHandle].t[1];
				RoQPrepMcomp( cinTable[currentHandle].roqF0, cinTable[currentHandle].roqF1 );
				cinTable[currentHandle].VQ1( (byte *)cin.qStatus[1], framedata);
				cinTable[currentHandle].buf = 	cin.linbuf + cinTable[currentHandle].screenDelta;
			} else {
				cinTable[currentHandle].normalBuffer0 = cinTable[currentHandle].t[0];
				RoQPrepMcomp( cinTable[currentHandle].roqF0, cinTable[currentHandle].roqF1 );
				cinTable[currentHandle].VQ0( (byte *)cin.qStatus[0], framedata );
				cinTable[currentHandle].buf = 	cin.linbuf;
			}
			if (cinTable[currentHandle].numQuads == 0) {		// first frame
				Com_Memcpy(cin.linbuf+cinTable[currentHandle].screenDelta, cin.linbuf, cinTable[currentHandle].samplesPerLine*cinTable[currentHandle].ysize);
			}
			cinTable[currentHandle].numQuads++;
			cinTable[currentHandle].dirty = qtrue;
			break;
		case	ROQ_CODEBOOK:
			decodeCodeBook( framedata, (unsigned short)cinTable[currentHandle].roq_flags );
			break;
		case	ZA_SOUND_MONO:
			if (!cinTable[currentHandle].silent) {
				ssize = RllDecodeMonoToStereo( framedata, sbuf, cinTable[currentHandle].RoQFrameSize, 0, (unsigned short)cinTable[currentHandle].roq_flags);
                S_RawSamples( ssize, 22050, 2, 1, (byte *)sbuf, s_volume->value, 1 );
			}
			break;
		case	ZA_SOUND_STEREO:
			if (!cinTable[currentHandle].silent) {
				if (cinTable[currentHandle].numQuads == -1) {
					S_Update();
					s_rawend = s_soundtime;
				}
				ssize = RllDecodeStereoToStereo( framedata, sbuf, cinTable[currentHandle].RoQFrameSize, 0, (unsigned short)cinTable[currentHandle].roq_flags);
                S_RawSamples( ssize, 22050, 2, 2, (byte *)sbuf, s_volume->value, 1 );
			}
			break;
		case	ROQ_QUAD_INFO:
			if (cinTable[currentHandle].numQuads == -1) {
				readQuadInfo( framedata );
				setupQuad( 0, 0 );
				cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime = Sys_Milliseconds()*com_timescale->value;
			}
			if (cinTable[currentHandle].numQuads != 1) cinTable[currentHandle].numQuads = 0;
			break;
		case	ROQ_PACKET:
			cinTable[currentHandle].inMemory = (qboolean)cinTable[currentHandle].roq_flags;
			cinTable[currentHandle].RoQFrameSize = 0;           // for header
			break;
		case	ROQ_QUAD_HANG:
			cinTable[currentHandle].RoQFrameSize = 0;
			break;
		case	ROQ_QUAD_JPEG:
			break;
		default:
			cinTable[currentHandle].status = FMV_EOF;
			break;
	}
//
// read in next frame data
//
	if ( cinTable[currentHandle].RoQPlayed >= cinTable[currentHandle].ROQSize ) {
		if (cinTable[currentHandle].holdAtEnd==qfalse) {
			if (cinTable[currentHandle].looping) {
				RoQReset();
			} else {
				cinTable[currentHandle].status = FMV_EOF;
			}
		} else {
			cinTable[currentHandle].status = FMV_IDLE;
		}
		return;
	}

	framedata		 += cinTable[currentHandle].RoQFrameSize;
	cinTable[currentHandle].roq_id		 = framedata[0] + framedata[1]*256;
	cinTable[currentHandle].RoQFrameSize = framedata[2] + framedata[3]*256 + framedata[4]*65536;
	cinTable[currentHandle].roq_flags	 = framedata[6] + framedata[7]*256;
	cinTable[currentHandle].roqF0		 = (signed char)framedata[7];
	cinTable[currentHandle].roqF1		 = (signed char)framedata[6];

	if (cinTable[currentHandle].RoQFrameSize>65536||cinTable[currentHandle].roq_id==0x1084) {
		Com_DPrintf("roq_size>65536||roq_id==0x1084\n");
		cinTable[currentHandle].status = FMV_EOF;
		if (cinTable[currentHandle].looping) {
			RoQReset();
		}
		return;
	}
	if (cinTable[currentHandle].inMemory && (cinTable[currentHandle].status != FMV_EOF))
	{
		cinTable[currentHandle].inMemory = (qboolean)(((int)cinTable[currentHandle].inMemory)-1);
		framedata += 8;
		goto redump;
	}
//
// one more frame hits the dust
//
//	assert(cinTable[currentHandle].RoQFrameSize <= 65536);
//	r = FS_Read( cin.file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile );
	cinTable[currentHandle].RoQPlayed	+= cinTable[currentHandle].RoQFrameSize+8;
}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void RoQ_init( void )
{
	cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime = Sys_Milliseconds()*com_timescale->value;

	cinTable[currentHandle].RoQPlayed = 24;

	// get frame rate
	cinTable[currentHandle].roqFPS	 = cin.file[ 6] + cin.file[ 7]*256;

	if (!cinTable[currentHandle].roqFPS) cinTable[currentHandle].roqFPS = 30;

	cinTable[currentHandle].numQuads = -1;

	cinTable[currentHandle].roq_id		= cin.file[ 8] + cin.file[ 9]*256;
	cinTable[currentHandle].RoQFrameSize	= cin.file[10] + cin.file[11]*256 + cin.file[12]*65536;
	cinTable[currentHandle].roq_flags	= cin.file[14] + cin.file[15]*256;

	if (cinTable[currentHandle].RoQFrameSize > 65536 || !cinTable[currentHandle].RoQFrameSize) {
		return;
	}

}
*/

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static void RoQShutdown( void ) {
	const char *s;

	if (!cinTable[currentHandle].buf) {
		return;
	}

	if ( cinTable[currentHandle].status == FMV_IDLE ) {
		return;
	}
	Com_DPrintf("finished cinematic\n");
	cinTable[currentHandle].status = FMV_IDLE;

	if (cinTable[currentHandle].iFile) {
		FS_FCloseFile( cinTable[currentHandle].iFile );
		cinTable[currentHandle].iFile = 0;
	}

	if (cinTable[currentHandle].alterGameState) {
		cls.state = CA_DISCONNECTED;
		// we can't just do a vstr nextmap, because
		// if we are aborting the intro cinematic with
		// a devmap command, nextmap would be valid by
		// the time it was referenced
		s = Cvar_VariableString( "nextmap" );
		if ( s[0] ) {
			Cbuf_ExecuteText( EXEC_APPEND, va("%s\n", s) );
			Cvar_Set( "nextmap", "" );
		}
		CL_handle = -1;
	}
	cinTable[currentHandle].fileName[0] = 0;
	currentHandle = -1;
}
*/

/*
==================
CIN_StopCinematic
==================
*/
/*
e_status CIN_StopCinematic(int handle) {

	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return FMV_EOF;
	currentHandle = handle;

	Com_DPrintf("trFMV::stop(), closing %s\n", cinTable[currentHandle].fileName);

	if (!cinTable[currentHandle].buf) {
		return FMV_EOF;
	}

	if (cinTable[currentHandle].alterGameState) {
		if ( cls.state != CA_CINEMATIC ) {
			return cinTable[currentHandle].status;
		}
	}
	cinTable[currentHandle].status = FMV_EOF;
	RoQShutdown();

	return FMV_EOF;
}
*/

/*
==================
CIN_RunCinematic

Fetch and decompress the pending frame
==================
*/

/*
e_status CIN_RunCinematic (int handle)
{
	int	start = 0;
	int     thisTime = 0;

	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return FMV_EOF;

	if (cin.currentHandle != handle) {
		currentHandle = handle;
		cin.currentHandle = currentHandle;
		cinTable[currentHandle].status = FMV_EOF;
		RoQReset();
	}

	if (cinTable[handle].playonwalls < -1)
	{
		return cinTable[handle].status;
	}

	currentHandle = handle;

	if (cinTable[currentHandle].alterGameState) {
		if ( cls.state != CA_CINEMATIC ) {
			return cinTable[currentHandle].status;
		}
	}

	if (cinTable[currentHandle].status == FMV_IDLE) {
		return cinTable[currentHandle].status;
	}

	thisTime = Sys_Milliseconds()*com_timescale->value;
	if (cinTable[currentHandle].shader && (abs(thisTime - (double)cinTable[currentHandle].lastTime))>100) {
		cinTable[currentHandle].startTime += thisTime - cinTable[currentHandle].lastTime;
	}
	cinTable[currentHandle].tfps = ((((Sys_Milliseconds()*com_timescale->value) - cinTable[currentHandle].startTime)*cinTable[currentHandle].roqFPS)/1000);

	start = cinTable[currentHandle].startTime;
	while(  (cinTable[currentHandle].tfps != cinTable[currentHandle].numQuads)
		&& (cinTable[currentHandle].status == FMV_PLAY) )
	{
		RoQInterrupt();
		if ((unsigned)start != cinTable[currentHandle].startTime) {
		  cinTable[currentHandle].tfps = ((((Sys_Milliseconds()*com_timescale->value)
							  - cinTable[currentHandle].startTime)*cinTable[currentHandle].roqFPS)/1000);
			start = cinTable[currentHandle].startTime;
		}
	}

	cinTable[currentHandle].lastTime = thisTime;

	if (cinTable[currentHandle].status == FMV_LOOPED) {
		cinTable[currentHandle].status = FMV_PLAY;
	}

	if (cinTable[currentHandle].status == FMV_EOF) {
	  if (cinTable[currentHandle].looping) {
		RoQReset();
	  } else {
		RoQShutdown();
	  }
	}

	return cinTable[currentHandle].status;
}
*/

/*
==================
CIN_PlayCinematic
==================
*/
/*
int CIN_PlayCinematic( const char *arg, int x, int y, int w, int h, int systemBits ) {
	unsigned short RoQID;
	char	name[MAX_OSPATH];
	int		i;

	if (strstr(arg, "/") == NULL && strstr(arg, "\\") == NULL) {
		Com_sprintf (name, sizeof(name), "video/%s", arg);
	} else {
		Com_sprintf (name, sizeof(name), "%s", arg);
	}
	COM_DefaultExtension(name,sizeof(name),".roq");

	if (!(systemBits & CIN_system)) {
		for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
			if (!strcmp(cinTable[i].fileName, name) ) {
				return i;
			}
		}
	}

	Com_DPrintf("CIN_PlayCinematic( %s )\n", arg);

	Com_Memset(&cin, 0, sizeof(cinematics_t) );
	currentHandle = CIN_HandleForVideo();

	cin.currentHandle = currentHandle;

	strcpy(cinTable[currentHandle].fileName, name);

	cinTable[currentHandle].ROQSize = 0;
	cinTable[currentHandle].ROQSize = FS_FOpenFileRead (cinTable[currentHandle].fileName, &cinTable[currentHandle].iFile, qtrue);

	if (cinTable[currentHandle].ROQSize<=0) {
		Com_DPrintf("cinematic failed to open %s\n", arg);
		cinTable[currentHandle].fileName[0] = 0;
		return -1;
	}

	CIN_SetExtents(currentHandle, x, y, w, h);
	CIN_SetLooping(currentHandle, (qboolean)((systemBits & CIN_loop)!=0));

	cinTable[currentHandle].CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
	cinTable[currentHandle].CIN_WIDTH  =  DEFAULT_CIN_WIDTH;
	cinTable[currentHandle].holdAtEnd = (qboolean)((systemBits & CIN_hold) != 0);
	cinTable[currentHandle].alterGameState = (qboolean)((systemBits & CIN_system) != 0);
	cinTable[currentHandle].playonwalls = 1;
	cinTable[currentHandle].silent = (qboolean)((systemBits & CIN_silent) != 0);
	cinTable[currentHandle].shader = (qboolean)((systemBits & CIN_shader) != 0);

	if (cinTable[currentHandle].alterGameState) {
		// close the menu
		if ( cls.uiStarted ) {
			UIVM_SetActiveMenu( UIMENU_NONE );
		}
	} else {
		cinTable[currentHandle].playonwalls = cl_inGameVideo->integer;
	}

	initRoQ();

	FS_Read (cin.file, 16, cinTable[currentHandle].iFile);

	RoQID = (unsigned short)(cin.file[0]) + (unsigned short)(cin.file[1])*256;
	if (RoQID == 0x1084)
	{
		RoQ_init();
//		FS_Read (cin.file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile);

		cinTable[currentHandle].status = FMV_PLAY;
		Com_DPrintf("trFMV::play(), playing %s\n", arg);

		if (cinTable[currentHandle].alterGameState) {
			cls.state = CA_CINEMATIC;
		}

		Con_Close();

		if ( !cinTable[currentHandle].silent )
			s_rawend = s_soundtime;

		return currentHandle;
	}
	Com_DPrintf("trFMV::play(), invalid RoQ ID\n");

	RoQShutdown();
	return -1;
}
*/

/*
void CIN_SetExtents (int handle, int x, int y, int w, int h) {
	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return;
	cinTable[handle].xpos = x;
	cinTable[handle].ypos = y;
	cinTable[handle].width = w;
	cinTable[handle].height = h;
	cinTable[handle].dirty = qtrue;
}
*/

/*
void CIN_SetLooping(int handle, qboolean loop) {
	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return;
	cinTable[handle].looping = loop;
}
*/

/*
==================
CIN_ResampleCinematic

Resample cinematic to 256x256 and store in buf2
==================
*/

/*
void CIN_ResampleCinematic(int handle, int *buf2) {
	int ix, iy, *buf3, xm, ym, ll;
	byte	*buf;

	buf = cinTable[handle].buf;

	xm = cinTable[handle].CIN_WIDTH/256;
	ym = cinTable[handle].CIN_HEIGHT/256;
	ll = 8;
	if (cinTable[handle].CIN_WIDTH==512) {
		ll = 9;
	}

	buf3 = (int*)buf;
	if (xm==2 && ym==2) {
		byte *bc2, *bc3;
		int	ic, iiy;

		bc2 = (byte *)buf2;
		bc3 = (byte *)buf3;
		for (iy = 0; iy<256; iy++) {
			iiy = iy<<12;
			for (ix = 0; ix<2048; ix+=8) {
				for(ic = ix;ic<(ix+4);ic++) {
					*bc2=(bc3[iiy+ic]+bc3[iiy+4+ic]+bc3[iiy+2048+ic]+bc3[iiy+2048+4+ic])>>2;
					bc2++;
				}
			}
		}
	} else if (xm==2 && ym==1) {
		byte *bc2, *bc3;
		int	ic, iiy;

		bc2 = (byte *)buf2;
		bc3 = (byte *)buf3;
		for (iy = 0; iy<256; iy++) {
			iiy = iy<<11;
			for (ix = 0; ix<2048; ix+=8) {
				for(ic = ix;ic<(ix+4);ic++) {
					*bc2=(bc3[iiy+ic]+bc3[iiy+4+ic])>>1;
					bc2++;
				}
			}
		}
	} else {
		for (iy = 0; iy<256; iy++) {
			for (ix = 0; ix<256; ix++) {
					buf2[(iy<<8)+ix] = buf3[((iy*ym)<<ll) + (ix*xm)];
			}
		}
	}
}
*/

/*
==================
CIN_DrawCinematic
==================
*/

/*
void CIN_DrawCinematic (int handle) {
	float	x, y, w, h;
	byte	*buf;

	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return;

	if (!cinTable[handle].buf) {
		return;
	}

	x = cinTable[handle].xpos;
	y = cinTable[handle].ypos;
	w = cinTable[handle].width;
	h = cinTable[handle].height;
	buf = cinTable[handle].buf;

	if (cinTable[handle].dirty && (cinTable[handle].CIN_WIDTH != cinTable[handle].drawX || cinTable[handle].CIN_HEIGHT != cinTable[handle].drawY)) {
		int *buf2;

		buf2 = (int *)Hunk_AllocateTempMemory( 256*256*4 );

		CIN_ResampleCinematic(handle, buf2);

		re->DrawStretchRaw( x, y, w, h, 256, 256, (byte *)buf2, handle, qtrue);
		cinTable[handle].dirty = qfalse;
		Hunk_FreeTempMemory(buf2);
		return;
	}

	re->DrawStretchRaw( x, y, w, h, cinTable[handle].drawX, cinTable[handle].drawY, buf, handle, cinTable[handle].dirty);
	cinTable[handle].dirty = qfalse;
}
*/

/*
void CL_PlayCinematic_f(void) {
	Com_DPrintf("CL_PlayCinematic_f\n");
	if (cls.state == CA_CINEMATIC) {
		SCR_StopCinematic();
	}

	const char *arg = Cmd_Argv(1);
	const char *s = Cmd_Argv(2);

	int bits = CIN_system;
	if ((s && s[0] == '1') || Q_stricmp(arg,"demoend.roq")==0 || Q_stricmp(arg,"end.roq")==0) {
		bits |= CIN_hold;
	}
	if (s && s[0] == '2') {
		bits |= CIN_loop;
	}

	S_StopAllSounds ();

	CL_handle = CIN_PlayCinematic( arg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bits );
	if (CL_handle >= 0) {
		do {
			SCR_RunCinematic();
		} while (cinTable[currentHandle].buf == NULL && cinTable[currentHandle].status == FMV_PLAY);		// wait for first frame (load codebook and sound)
	}
	else
	{
		Com_Printf(S_COLOR_RED "PlayCinematic(): Failed to open \"%s\"\n", arg);
	}
}


void SCR_DrawCinematic (void) {
	if (CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES) {
		CIN_DrawCinematic(CL_handle);
	}
}

void SCR_RunCinematic (void)
{
	if (CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES) {
		CIN_RunCinematic(CL_handle);
	}
}

void SCR_StopCinematic(void) {
	if (CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES) {
		CIN_StopCinematic(CL_handle);
		S_StopAllSounds ();
		CL_handle = -1;
	}
}

void CIN_UploadCinematic(int handle) {
	if (handle >= 0 && handle < MAX_VIDEO_HANDLES) {
		if (!cinTable[handle].buf) {
			return;
		}
		if (cinTable[handle].playonwalls <= 0 && cinTable[handle].dirty) {
			if (cinTable[handle].playonwalls == 0) {
				cinTable[handle].playonwalls = -1;
			} else {
				if (cinTable[handle].playonwalls == -1) {
					cinTable[handle].playonwalls = -2;
				} else {
					cinTable[handle].dirty = qfalse;
				}
			}
		}

		// Resample the video if needed
		if (cinTable[handle].dirty && (cinTable[handle].CIN_WIDTH != cinTable[handle].drawX || cinTable[handle].CIN_HEIGHT != cinTable[handle].drawY))  {
			int *buf2;

			buf2 = (int *)Hunk_AllocateTempMemory( 256*256*4 );

			CIN_ResampleCinematic(handle, buf2);

			re->UploadCinematic( 256, 256, (byte *)buf2, handle, qtrue);
			cinTable[handle].dirty = qfalse;
			Hunk_FreeTempMemory(buf2);
		} else {
			// Upload video at normal resolution
			re->UploadCinematic( cinTable[handle].drawX, cinTable[handle].drawY,
					cinTable[handle].buf, handle, cinTable[handle].dirty);
			cinTable[handle].dirty = qfalse;
		}

		if (cl_inGameVideo->integer == 0 && cinTable[handle].playonwalls == 1) {
			cinTable[handle].playonwalls--;
		}
		else if (cl_inGameVideo->integer != 0 && cinTable[handle].playonwalls != 1) {
			cinTable[handle].playonwalls = 1;
		}
	}
}
*/

// =========================================================================================================================================================











// =========================================================================================================================================================

static bool CIN_do_print = false;
static void CIN_Set_Print_Callback() {
	av_log_set_callback(*[](void *, int lev, char const * fmt, va_list val) {
		if (com_developer->integer || CIN_do_print) {
			if (lev == AV_LOG_QUIET) return;
			if (lev < AV_LOG_ERROR && lev > 0)
				Com_VPrintf(fmt, val);
			else if (lev == AV_LOG_WARNING) {
				Com_VPrintf(fmt, val);
			} else if (lev == AV_LOG_INFO) {
				Com_VPrintf(fmt, val);
			}
		}
	});
}

static int CIN_Time() {
	return std::round(Sys_Milliseconds() * com_timescale->value);
}

// ================================
// EXCEPTION
// ================================

struct CIN_Exception : public std::exception {
	
	enum struct CODE : int {
		UNKNOWN = 0,
		FILE_NOT_FOUND = 1,
		AVBASE = 1000
	};
	
	CIN_Exception(std::string_view msg, CODE err = CODE::UNKNOWN) : m_code(err) {
		message = va("CIN EXCEPTION [%i]: %s\n", err,  msg.data());
	}
	
	char const * what() const noexcept override {
		return message.data();
	}
	
	CODE code() const { return m_code; }
	
private:
	CODE m_code;
	std::string message;
};

// ================================
// PERS
// ================================

struct CIN_Share {
	std::string path;
};

using CIN_SharePtr = std::shared_ptr<CIN_Share>;

// ================================
// FILE
// ================================

struct CIN_File {
	
	CIN_File(CIN_SharePtr share) : m_share { share } {
		m_size = FS_FOpenFileRead(m_share->path.data(), &m_fh, qtrue);
		if (m_size <= 0)
			throw CIN_Exception { va("could not open file [%s] for reading", m_share->path.data()), CIN_Exception::CODE::FILE_NOT_FOUND };
		m_io = avio_alloc_context(reinterpret_cast<uint8_t *>(av_malloc(BUFFER_SIZE)), BUFFER_SIZE, 0, this, &read_static, nullptr, &seek_static);
	}
	
	~CIN_File() {
		if (m_io->buffer) av_free(m_io->buffer);
		if (m_io) avio_context_free(&m_io);
		if (m_fh >= 0) FS_FCloseFile(m_fh);
	}
	
	inline AVIOContext * io() { return m_io; }
	
	void reset() {
		avio_seek(m_io, 0, SEEK_SET);
	}
	
private:
	
	static constexpr size_t BUFFER_SIZE = 1024 * 1024 * 64; // 64 MiB
	
	CIN_SharePtr m_share;
	
	int m_size = 0;
	
	// --------
	fileHandle_t m_fh = -1;
	AVIOContext * m_io = nullptr;
	// --------
	
	int read(uint8_t * buf, int len) {
		return FS_Read(buf, len, m_fh);
	}
	
	int64_t seek(int64_t offset, int whence) {
		switch(whence) {
		default:
		case SEEK_CUR: return FS_Seek(m_fh, offset, FS_SEEK_CUR);
		case SEEK_SET: return FS_Seek(m_fh, offset, FS_SEEK_SET);
		case SEEK_END: return FS_Seek(m_fh, offset, FS_SEEK_END);
		}
	}
	
	static inline int read_static(void * self, uint8_t * buf, int len) {
		return reinterpret_cast<CIN_File *>(self)->read(buf, len);
	}
	
	static inline int64_t seek_static(void * self, int64_t offset, int whence) {
		return reinterpret_cast<CIN_File *>(self)->seek(offset, whence);
	}
};

using CIN_FilePtr = std::unique_ptr<CIN_File>;

// ================================
// FORMAT
// ================================

struct CIN_Container {
	
	CIN_Container(CIN_SharePtr share, AVIOContext * io) : m_share { share } {
		m_fctx = avformat_alloc_context();
		m_fctx->pb = io;
		
		int err;
		
		err = avformat_open_input(&m_fctx, "", nullptr, nullptr);
		if (err < 0) 
			throw CIN_Exception { va("unknown IO error in [%s]\n", m_share->path.data()) };
		
		err = avformat_find_stream_info(m_fctx, nullptr);
		if (err < 0) 
			throw CIN_Exception { va("could not find stream info in [%s]\n", m_share->path.data()) };
	}
	
	~CIN_Container() {
		if (m_fctx) avformat_close_input(&m_fctx);
	}
	
	inline int find_video() {
		for (size_t i = 0; i < m_fctx->nb_streams; i++) {
			if (m_fctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
				return i;
			}
		}
		return -1;
	}
	
	inline AVStream * stream_for_idx(int idx) { return m_fctx->streams[idx]; }
	
	inline int frametime_msec(int stream_idx) {
		auto framerate = av_guess_frame_rate(m_fctx, m_fctx->streams[stream_idx], nullptr);
		if (framerate.num == 0 && framerate.den == 1)
			throw CIN_Exception { va("could not determine framerate of video stream in [%s]\n", m_share->path.data()) };
		return std::round(av_q2d(av_inv_q(framerate)) * 1000);
	}
	
	inline int read_packet(AVPacket * pkt) {
		return av_read_frame(m_fctx, pkt);
	}
	
	inline void print_info() {
		CIN_do_print = true;
		av_dump_format(m_fctx, 0, m_share->path.data(), 0);
		CIN_do_print = false;

	}
	
private:
	
	CIN_SharePtr m_share;
	
	// --------
	AVFormatContext * m_fctx = nullptr;
	// --------
};

using CIN_ContainerPtr = std::shared_ptr<CIN_Container>;

// ================================
// CODEC
// ================================

struct CIN_Decoder {
	
	CIN_Decoder(CIN_SharePtr share, CIN_ContainerPtr cont, int stream_idx, int width_dst, int height_dst) : m_share { share }, m_cont { cont } {
		
		m_stream = m_cont->stream_for_idx(m_stream_idx = stream_idx);
		
		m_codec = avcodec_find_decoder(m_stream->codecpar->codec_id);
		char const * codec_name = avcodec_get_name(m_stream->codecpar->codec_id);
		
		if (!m_codec)
			throw CIN_Exception { va("could not find decoder for codec [%s] in video [%s]\n", codec_name, m_share->path.data()) };
		
		m_cctx = avcodec_alloc_context3(m_codec);
		avcodec_parameters_to_context(m_cctx, m_stream->codecpar);
		avcodec_open2(m_cctx, m_codec, nullptr);
		
		m_sws = sws_getContext(m_cctx->width, m_cctx->height, m_cctx->pix_fmt, width_dst, height_dst, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
	}
	
	~CIN_Decoder() {
		if (m_cctx) avcodec_free_context(&m_cctx);
		if (m_sws) sws_freeContext(m_sws);
	}
	
	enum struct DecodeStatus {
		OK,
		LOOP,
		FAIL
	};
	
	DecodeStatus retrieve_frame(AVFrame * dst) {
		
		std::unique_ptr<AVFrame, void(*)(AVFrame *)> src { av_frame_alloc(), *[](AVFrame * ptr){ av_frame_free(&ptr); } };
		int err;
		
		while (true) {
			err = avcodec_receive_frame(m_cctx, src.get());
			if (!err) break;
			if (err != AVERROR(EAGAIN))
				throw CIN_Exception { "unknown error occurred when decoding stream", static_cast<CIN_Exception::CODE>(err + 1000) };
			AVPacket pkt;
			do {
				err = m_cont->read_packet(&pkt);
				if (err < 0)
					return DecodeStatus::LOOP;
			} while (pkt.stream_index != m_stream_idx);
			assert(avcodec_send_packet(m_cctx, &pkt) == 0);
			av_packet_unref(&pkt);
		}
		
		sws_scale(m_sws, src->data, src->linesize, 0, m_cctx->height, dst->data, dst->linesize);
		
		return DecodeStatus::OK;
	}
	
private:
	
	CIN_SharePtr m_share;
	CIN_ContainerPtr m_cont;
	AVCodec const * m_codec = nullptr;
	AVStream * m_stream;
	int m_stream_idx;
	
	// --------
	AVCodecContext * m_cctx = nullptr;
	SwsContext * m_sws = nullptr;
	// --------
};

using CIN_DecoderPtr = std::unique_ptr<CIN_Decoder>;

// ================================
// CONTEXT
// ================================

static int nearest_square(int in) {
	return 2 << static_cast<int>(std::floor(std::log2(in - 1.0)));
}

struct CIN_Context {
	
	CIN_Context(char const * path, int handle, int max_width, int max_height) 
		: m_handle { handle }
	{	
		m_share = std::make_shared<CIN_Share>();
		m_share->path = path;
		
		CIN_Set_Print_Callback();
		
		m_file = std::make_unique<CIN_File>(m_share);
		m_cont = std::make_shared<CIN_Container>(m_share, m_file->io());
		
		m_stream_idx = m_cont->find_video();
		if (m_stream_idx < 0)
			throw CIN_Exception { va("could not find a video stream in [%s]\n", m_share->path.data()) };
		
		auto frame = m_cont->stream_for_idx(m_stream_idx);
		m_width = qm::min(max_width, nearest_square(frame->codecpar->width));
		m_height = qm::min(max_height, nearest_square(frame->codecpar->height));
		
		m_dec = std::make_unique<CIN_Decoder>(m_share, m_cont, m_stream_idx, m_width, m_height);
		m_frametime_msec = m_cont->frametime_msec(m_stream_idx);
		
		m_frame = av_frame_alloc();
		
		m_framebuffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, m_width, m_height, 32);
		m_framebuffer = (uint8_t *)av_malloc(m_framebuffer_size);
		av_image_fill_arrays(m_frame->data, m_frame->linesize, m_framebuffer, AV_PIX_FMT_RGBA, m_width, m_height, 32);
		
		m_starttime = CIN_Time();
		m_framenum = 0;
		
		if (com_developer->integer) {
			Com_Printf("\n\nCIN: New Video Loaded\n================\n");
			m_cont->print_info();
			Com_Printf("================\n");
			Com_Printf("using video stream: %zu, output size: %ix%i\n", m_stream_idx, m_width, m_height);
			Com_Printf("================\n\n");
		}
	}
	
	~CIN_Context() {
		if (m_framebuffer) av_free(m_framebuffer);
		if (m_frame) av_frame_free(&m_frame);
	}
	
	void run() {
		
		bool loop_breaker = false;
		try_again:
		if (loop_breaker) return;
		//loop_breaker = true;
		
		int frame_target = (CIN_Time() - m_starttime) / m_frametime_msec + 1;
		if (frame_target == m_framenum) return;
		
		static constexpr int TIME_SKIP_THRESHOLD = 4;
		
		if (frame_target < m_framenum) { // moved backwards, probably a timescale change, reset the timing
			m_framenum = 0; // not a video reset, but a time tracking reset
			frame_target = 1;
		}
		
		if (frame_target - m_framenum > TIME_SKIP_THRESHOLD) {
			m_framenum = 0; // not a video reset, but a time tracking reset
			frame_target = 1;
		}
		
		if (!m_framenum) m_starttime = CIN_Time();
		
		while (m_framenum < frame_target) {
			if (m_dec->retrieve_frame(m_frame) != CIN_Decoder::DecodeStatus::OK) {
				reset();
				goto try_again;
				return;
			}
			m_framenum++;
			m_dirty = true;
		}
	}
	
	void upload() {
		re->UploadCinematic(m_width, m_height, m_framebuffer, m_handle, m_dirty);
		m_dirty = false;
	}
	
	void upload2d() {
		re->DrawStretchRaw(0, 0, m_width, m_height, m_width, m_height, m_framebuffer, m_handle, m_dirty);
		m_dirty = false;
	}
	
	void close() {
		m_file.reset();
		m_cont.reset();
		m_dec.reset();
	}
	
	void reset() {
		m_file->reset();
		m_cont = std::make_shared<CIN_Container>(m_share, m_file->io());
		m_dec = std::make_unique<CIN_Decoder>(m_share, m_cont, m_stream_idx, m_width, m_height);
	}
	
	inline bool is_active() const {
		return m_dec && m_cont && m_file;
	}
	
private:
	
	CIN_SharePtr m_share;
	CIN_FilePtr m_file;
	CIN_ContainerPtr m_cont;
	CIN_DecoderPtr m_dec;
	
	int m_handle;
	int m_width, m_height;
	int m_stream_idx;
	int m_framebuffer_size;
	int m_frametime_msec;
	int m_starttime;
	int m_framenum;
	bool m_dirty = true;
	
	// --------
	AVFrame * m_frame = nullptr;
	uint8_t * m_framebuffer = nullptr;
	// --------
};

// =========================================================================================================================================================

static std::vector<std::unique_ptr<CIN_Context>> cin_contexts;
std::unordered_map<istring, int> cin_handle_lookup;

void CIN_CloseAllVideos(void) {
	cin_contexts.clear();
	cin_handle_lookup.clear();
}

// =========================================================================================================================================================

e_status CIN_RunCinematic (int handle) {
	if (handle < 0 || handle >= (ssize_t)cin_contexts.size()) return FMV_EOF;
	if (!cin_contexts[handle]->is_active()) return FMV_EOF;
	cin_contexts[handle]->run();
	return FMV_PLAY;
}

int CIN_PlayCinematic( char const * path, int /*x*/, int /*y*/, int max_width, int max_height, int /*flags*/ ) {
	
	std::array<char, MAX_QPATH> name, name_mkv, name_roq;
	if (!strstr(path, "/") && !strstr(path, "\\"))
		Com_sprintf(name.data(), MAX_QPATH, "video/%s", path);
	else
		strncpy(name.data(), path, MAX_QPATH);
	
	name_mkv = name;
	name_roq = name;
	COM_DefaultExtension(name_mkv.data(), MAX_QPATH, ".mkv");
	COM_DefaultExtension(name_roq.data(), MAX_QPATH, ".roq"); // for compatibility reasons
	
	auto iter = cin_handle_lookup.find(name.data());
	if (iter != cin_handle_lookup.end()) {
		auto & ptr = cin_contexts[iter->second];
		if (!ptr->is_active()) ptr->reset();
		return iter->second;
	}
	
	iter = cin_handle_lookup.find(name_mkv.data());
	if (iter != cin_handle_lookup.end()) {
		auto & ptr = cin_contexts[iter->second];
		if (!ptr->is_active()) ptr->reset();
		return iter->second;
	}
	
	iter = cin_handle_lookup.find(name_roq.data());
	if (iter != cin_handle_lookup.end()) {
		auto & ptr = cin_contexts[iter->second];
		if (!ptr->is_active()) ptr->reset();
		return iter->second;
	}
	
	int handle = cin_contexts.size();
	std::unique_ptr<CIN_Context> & ctx = cin_contexts.emplace_back();
	
	try {
		ctx = std::make_unique<CIN_Context>(name.data(), handle, max_width, max_height);
		cin_handle_lookup[name.data()] = handle;
		return handle;
	} catch (CIN_Exception & ex) {
		if (ex.code() != CIN_Exception::CODE::FILE_NOT_FOUND) {
			Com_Printf("%s\n", ex.what());
			ctx.reset();
			return -1;
		}
	}
	
	try {
		ctx = std::make_unique<CIN_Context>(name_mkv.data(), handle, max_width, max_height);
		cin_handle_lookup[name_mkv.data()] = handle;
		return handle;
	} catch (CIN_Exception & ex) {
		if (ex.code() != CIN_Exception::CODE::FILE_NOT_FOUND) {
			Com_Printf("%s\n", ex.what());
			ctx.reset();
			return -1;
		}
	}
	
	try {
		ctx = std::make_unique<CIN_Context>(name_roq.data(), handle, max_width, max_height);
		cin_handle_lookup[name_roq.data()] = handle;
		return handle;
	} catch (CIN_Exception & ex) {
		if (ex.code() != CIN_Exception::CODE::FILE_NOT_FOUND) {
			Com_Printf("%s\n", ex.what());
			ctx.reset();
			return -1;
		}
	}
	
	Com_Printf("CIN EXCEPTION [%s]: could not find video!\n", name.data());
	ctx.reset();
	return -1;
}

void CIN_SetExtents (int /*handle*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/) {
	// TODO -- ?????
}

void CIN_DrawCinematic (int handle) {
	if (handle < 0 || handle >= (ssize_t)cin_contexts.size()) return;
	if (!cin_contexts[handle]->is_active()) return;
	cin_contexts[handle]->upload2d();
}

e_status CIN_StopCinematic(int handle) {
	if (handle < 0 || handle >= (ssize_t)cin_contexts.size()) return FMV_EOF;
	cin_contexts[handle]->close();
	return FMV_EOF;
}

void CIN_UploadCinematic(int handle) {
	if (handle < 0 || handle >= (ssize_t)cin_contexts.size()) return;
	if (!cin_contexts[handle]->is_active()) return;
	cin_contexts[handle]->upload();
}

// =========================================================================================================================================================

static int cl_ctx = -1;

void SCR_RunCinematic () {
	CIN_RunCinematic(cl_ctx);
}

void SCR_StopCinematic() {
	CIN_StopCinematic(cl_ctx);
	cl_ctx = -1;
}

void CL_PlayCinematic_f() {
	if (cls.state == CA_CINEMATIC || cl_ctx >= 0) {
		SCR_StopCinematic();
	}
	
	char const * path = Cmd_Argv(1);

	S_StopAllSounds();
	
	cl_ctx = CIN_PlayCinematic(path, 0, 0, 512, 512, 0);
	if (cl_ctx < 0) return;
	
	cls.state = CA_CINEMATIC;
	UIVM_SetActiveMenu( UIMENU_NONE );
}

void SCR_DrawCinematic () {
	CIN_DrawCinematic(cl_ctx);
}
