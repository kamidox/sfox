#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_File.h"
#include "inc/util/FS_TrueType.h"

#define FS_TT_OK									0
#define FS_TT_ERR_ENCODING_FORMAT_NOT_SUPPORT		-1
#define FS_TT_ERR_ENCODING_NOT_SUPPORT				-2
#define FS_TT_ERR_INVALID_FONT_FILE					-3
#define FS_TT_ERR_INVALID_GLYPH_DATA				-4
#define FS_TT_ERR_COMPOSITE_GLYPH_NOT_SUPPORT		-5

#define FS_TAB_DIR_CMAP	0x636D6170	/* cmap */
#define FS_TAB_DIR_HEAD	0x68656164	/* head */
#define FS_TAB_DIR_MAXP	0x6D617870	/* maxp */
#define FS_TAB_DIR_GLYF	0x676C7966	/* glyf */
#define FS_TAB_DIR_LOCA	0x6C6F6361	/* loca */

/* glyph data flags */
#define FS_TT_G_ON_CURVE	0x01
#define FS_TT_G_REPEAT		0x08
#define FS_TT_G_XMASK		0x12
#define FS_TT_G_YMASK		0x24

#define FS_TT_G_X_POS_BYTE		0x12
#define FS_TT_G_X_NAV_BYTE		0x02
#define FS_TT_G_X_THE_SAME		0x10
#define FS_TT_G_X_SHORT			0x00

#define FS_TT_G_Y_POS_BYTE		0x24
#define FS_TT_G_Y_NAV_BYTE		0x04
#define FS_TT_G_Y_THE_SAME		0x20
#define FS_TT_G_Y_SHORT			0x00

/* font bitmap */
#define FS_TT_MAX_FONT_W		64
#define FS_TT_MAX_FONT_H		64

/* font color */
#define FS_TT_FONT_COLOR		0xFF

#define FS_TT_READ_4(buf, val) (val) = (buf)[0] << 24 | (buf)[1] << 16 | (buf)[2] << 8 | (buf)[3] 
#define FS_TT_READ_2(buf, val) (val) = (buf)[0] << 8 | (buf)[1] 

#define FS_TT_CHECK_GLYPHDATA(offset, len) if( (offset) > (len) ) return FS_TT_ERR_INVALID_GLYPH_DATA

static void FS_TT_DRAW_PIXEL( FS_Bitmap *bmp, FS_SINT2 x, FS_SINT2 y )
{
	FS_BYTE * bits;
	
	if( x < 0 || y < 0 || x > bmp->width || y > bmp->height )
	{
		return;
	}
	bits = bmp->bits + ((bmp->height - y - 1) * bmp->pitch) + x * bmp->bpp;
	if( bmp->bpp == 1 )
	{
		*bits = FS_TT_FONT_COLOR;
	}
	else if( bmp->bpp == 2 )
	{
		*(FS_UINT2 *)(bits) = FS_TT_FONT_COLOR; 
	}
	else if( bmp->bpp == 3 )
	{
		*(FS_UINT2 *)(bits) = FS_TT_FONT_COLOR;
		*((bits) + 2) = FS_TT_FONT_COLOR >> 16;
	}
	else if( bmp->bpp == 4 )
	{
		*(FS_UINT4 *)(bits) = FS_TT_FONT_COLOR; 
	}
}

#if 0

#define FS_TTSetPixel(pDstPixel, clr) 						\
do {														\
	if( GFS_DC.bytes_per_pixel == 1 )						\
	{														\
		*(pDstPixel) = clr; 								\
	}														\
	else if( GFS_DC.bytes_per_pixel == 2 )					\
	{														\
		*(FS_UINT2 *)(pDstPixel) = clr; 					\
	}														\
	else if( GFS_DC.bytes_per_pixel == 3 )					\
	{														\
		*(FS_UINT2 *)(pDstPixel) = clr; 					\
		*((pDstPixel) + 2) = clr >> 16; 					\
	}														\
	else if( GFS_DC.bytes_per_pixel == 4 )					\
	{														\
		*(FS_UINT4 *)(pDstPixel) = clr; 					\
	}														\
} while( 0 )	

#endif

typedef struct FS_TTDirEntry_Tag
{
	FS_UINT4	tag;
	FS_UINT4	check_sum;
	FS_UINT4	offset;
	FS_UINT4	len;
}FS_TTDirEntry;

typedef struct FS_TTCmapCode_Tag
{
	FS_UINT2	platform_id;		/* must be 3 */
	FS_UINT2	encoding_id;	/* must be 1 this means unicode */
	FS_UINT4	offset;

	FS_UINT2	format;			/* only support format 4 now. This is the Microsoft standard character to glyph index mapping table */
	FS_UINT2	len;
	FS_UINT2	version;
	FS_UINT2	seg_count;
	FS_UINT2	search_range;
	FS_UINT2	entry_selector;
	FS_UINT2	range_shift;
	FS_UINT2 *	end_code;			/* seg_count length */
	FS_UINT2	reserve_pad;
	FS_UINT2 *	start_code;			/* seg_count length */
	FS_UINT2 *	id_delta;			/* seg_count length */
	FS_UINT2 *	id_range_offset;		/* seg_count length */
	
	FS_UINT2	glyph_id_size;		/* calc. (len - 16 - 4 * seg_countX2) >> 1 */
	FS_UINT2 *	glyph_id_array; 	/* calc. reference to id_range_offset. */
}FS_TTCmapCode;

typedef struct FS_TTCmap_Tag
{
	FS_UINT2	version;
	FS_UINT2	tab_num;
	
	FS_TTCmapCode *encode_tabs;
}FS_TTCmap;

typedef struct FS_TTHead_Tag
{
	FS_UINT4	version;
	FS_UINT4	font_revision;
	FS_UINT4	check_sum_adj;
	FS_UINT4	magic;
	FS_UINT2	flags;
	FS_UINT2	units_per_em;
	FS_BYTE		created[8];
	FS_BYTE		modified[8];
	FS_SINT2	xmin;
	FS_SINT2	ymin;
	FS_SINT2	xmax;
	FS_SINT2	ymax;
	FS_UINT2	mac_style;
	FS_UINT2	lowest_rec_ppem;
	FS_SINT2	direction;
	FS_SINT2	index_to_loc;
	FS_SINT2	glyph_data_format;
}FS_TTHead;

typedef struct FS_TTMaxp_Tag
{
	FS_UINT4	version;
	FS_UINT2	glyph_num;
	FS_UINT2	max_points;
	FS_UINT2	max_contours;
	FS_UINT2	max_composite_points;
	FS_UINT2	max_composite_contours;

	FS_UINT2	max_component_depth;	/* we only support 1 now */
	/* other field we ignore */
}FS_TTMaxp;

typedef struct FS_TTLoca_Tag
{
	FS_UINT4	num;
	FS_UINT4 *	offsets;
}FS_TTLoca;

typedef struct FS_TTGlyphDataSimple_Tag
{
	FS_UINT2 *	contours_end_pts;
	FS_UINT2	ins_len;
	FS_BYTE *	ins;
	FS_BYTE *	flags;
	FS_BYTE *	xcoord;
	FS_BYTE *	ycoord;
}FS_TTGlyphDataSimple;

typedef struct FS_TTGlyphData_Tag
{
	FS_SINT2	contours_num;
	FS_SINT2	xmin;
	FS_SINT2	ymin;
	FS_SINT2	xmax;
	FS_SINT2	ymax;

	FS_TTGlyphDataSimple data;
}FS_TTGlyphData;

typedef struct FS_TTTabDir_tag
{
	FS_CHAR *	font_file;
	
	FS_UINT4	version;
	FS_UINT2	tab_num;
	FS_UINT2	search_range;
	FS_UINT2	entry_selector;
	FS_UINT2	range_shift;

	FS_TTDirEntry *tab_entrys;
	
	FS_TTCmap		cmap;
	FS_TTHead		head;
	FS_TTMaxp		maxp;
	FS_TTLoca		loca;

	FS_TTGlyphData	glyph;
	FS_Bitmap		bmp;
}FS_TTFont;

static void FS_TTSwapByte2( void *buf, FS_SINT4 len )
{
	FS_BOOL isbigendian;
	FS_BYTE *p;
	FS_BYTE c;
	FS_SINT4 i;
	union {
		FS_SINT4 i;
		FS_CHAR c[4];
	} u;
	u.i = 0x11223344;

	isbigendian = (u.c[0]==0x11);

	if ( ! isbigendian) {
		p = (FS_BYTE *)buf;
		for( i = 0; i < len; i ++ )
		{
			c = p[i * 2];
			p[i * 2] = p[i * 2 + 1];
			p[i * 2 + 1] = c;
		}
	}
}

static void FS_TTSwapByte4( void *buf, FS_SINT4 len )
{
	FS_BOOL isbigendian;
	FS_BYTE *p;
	FS_BYTE c;
	FS_SINT4 i;
	union {
		FS_SINT4 i;
		FS_CHAR c[4];
	} u;
	u.i = 0x11223344;

	isbigendian = (u.c[0]==0x11);

	if ( ! isbigendian) {
		p = (FS_BYTE *)buf;
		for( i = 0; i < len; i ++ )
		{
			c = p[i * 4];
			p[i * 4] = p[i * 4 + 3];
			p[i * 4 + 3] = c;

			c = p[i * 4 + 1];
			p[i * 4 + 1] = p[i * 4 + 2];
			p[i * 4 + 2] = c;
		}
	}
}

static FS_TTDirEntry *FS_TTFindDirEntry( FS_TTFont *ttfont, FS_UINT4 tag )
{
	FS_UINT2 i;
	for( i = 0; i < ttfont->tab_num; i ++ )
	{
		if( ttfont->tab_entrys[i].tag == tag )
		{
			return ttfont->tab_entrys + i;
		}
	}
	return FS_NULL;
}

static FS_UINT2 FS_TTFindCodeSegment( FS_TTFont *ttfont, FS_UINT2 wchar, FS_UINT2 *tabindex )
{
	FS_UINT2 index, i;
	FS_TTCmapCode *codetab;
	for( i = 0; i < ttfont->cmap.tab_num; i ++ )
	{
		codetab = ttfont->cmap.encode_tabs + i;
		for( index = 0; index < codetab->seg_count; index ++ )
		{
			if( wchar <= codetab->end_code[index] )
			{
				*tabindex = i;
				return index;
			}
		}
	}
	*tabindex = 0xFFFF;
	return 0xFFFF;
}

static FS_SINT4 FS_TTReadGlyphData( FS_TTFont *ttfont, FS_UINT4 glyphoff, FS_UINT4 glyphlen )
{
	FS_BYTE *glyphdata, *flags, *px, *py;
	FS_TTGlyphData *glyph = &ttfont->glyph;
	FS_TTDirEntry *entry;
	FS_UINT4 offset;
	FS_UINT2 i, npts, xlen, ylen, flen;
	FS_CHAR xunit, yunit;
	
	entry = FS_TTFindDirEntry(ttfont, FS_TAB_DIR_GLYF);
	if( entry == FS_NULL )
	{
		return FS_TT_ERR_INVALID_FONT_FILE;
	}
	glyphdata = IFS_Malloc( glyphlen );
	FS_FileRead(FS_DIR_ROOT, ttfont->font_file, entry->offset + glyphoff, glyphdata, glyphlen);
	FS_TT_READ_2(glyphdata, glyph->contours_num);
	FS_TT_READ_2(glyphdata + 2, glyph->xmin);
	FS_TT_READ_2(glyphdata + 4, glyph->ymin);
	FS_TT_READ_2(glyphdata + 6, glyph->xmax);
	FS_TT_READ_2(glyphdata + 8, glyph->ymax);

	offset = 10;
	/*
	 * If  the number of contours is greater than or equal to zero, this is a single glyph; 
	 * if negative, this is a composite glyph.
	 */
	if( glyph->contours_num < 0 )
	{
		return FS_TT_ERR_COMPOSITE_GLYPH_NOT_SUPPORT;
	}
	if( glyph->contours_num > ttfont->maxp.max_contours )
	{
		return FS_TT_ERR_INVALID_GLYPH_DATA;
	}
	/* endPtsOfContours */
	glyph->data.contours_end_pts = IFS_Malloc(glyph->contours_num * 2);
	IFS_Memcpy(glyph->data.contours_end_pts, glyphdata + offset, glyph->contours_num * 2);
	FS_TTSwapByte2(glyph->data.contours_end_pts, glyph->contours_num);
	offset += glyph->contours_num * 2;
	FS_TT_CHECK_GLYPHDATA(offset, glyphlen);
	/* instructionLength */
	FS_TT_READ_2(glyphdata + offset, glyph->data.ins_len);
	offset += 2;
	FS_TT_CHECK_GLYPHDATA(offset, glyphlen);
	/* instructions */
	glyph->data.ins = IFS_Malloc(glyph->data.ins_len);
	IFS_Memcpy(glyph->data.ins, glyphdata + offset, glyph->data.ins_len);
	offset += glyph->data.ins_len;
	FS_TT_CHECK_GLYPHDATA(offset, glyphlen);
	/* flags */
	npts = glyph->data.contours_end_pts[glyph->contours_num - 1];
	if( npts > ttfont->maxp.max_points )
	{
		return FS_TT_ERR_INVALID_GLYPH_DATA;
	}
	/* now, we calc flags len, xcoord len, ycoord len */
	flags = glyphdata + offset;
	px = flags;
	xlen = 0;
	ylen = 0;
	for( i = 0; i < npts; i ++, px ++ )
	{
		switch(px[0] & FS_TT_G_XMASK)
		{
			case FS_TT_G_X_NAV_BYTE:
			case FS_TT_G_X_POS_BYTE:	
				xunit = 1;
				break;
			case FS_TT_G_X_SHORT:
				xunit = 2;
				break;
			default:
				xunit = 0;
				break;
		}
		
		switch(px[0] & FS_TT_G_YMASK)
		{
			case FS_TT_G_Y_NAV_BYTE:
			case FS_TT_G_Y_POS_BYTE:	
				yunit = 1;
				break;
			case FS_TT_G_Y_SHORT:
				yunit = 2;
				break;
			default:
				yunit = 0;
				break;
		}
		
		if(px[0] & FS_TT_G_REPEAT)
		{
			xlen += xunit * (px[1] + 1);
			ylen += yunit * (px[1] + 1);
			i += px[1];
			px ++;
		}
		else
		{
			xlen += xunit;
			ylen += yunit;
		}
	}

	flen = px - flags;
	py = px + xlen;
	FS_TT_CHECK_GLYPHDATA(flen + xlen + ylen + offset, glyphlen);
	glyph->data.flags = IFS_Malloc(flen);
	glyph->data.xcoord = IFS_Malloc(xlen);
	glyph->data.ycoord = IFS_Malloc(ylen);
	IFS_Memcpy(glyph->data.flags, flags, flen);
	IFS_Memcpy(glyph->data.xcoord, px, xlen);
	IFS_Memcpy(glyph->data.ycoord, py, ylen);
	return FS_TT_OK;
}

static FS_SINT4 FS_TTDrawGlyphOutline( FS_TTFont *ttfont, FS_SINT4 size )
{
	FS_TTGlyphData *glyph = &ttfont->glyph;
	FS_BYTE *bits = ttfont->bmp.bits;
	FS_UINT2 i, k, npts;
	FS_CHAR xunit = 0, yunit = 0;
	FS_SINT2 x, y, dx, dy;
	FS_BYTE *px, *py, *pf;
	FS_Bitmap *bmp = &ttfont->bmp;
	FS_SINT2 xorg = -(size >> 2), yorg = size >> 2;
	
	ttfont->bmp.width = size;
	ttfont->bmp.height = size;
	ttfont->bmp.bpp = (IFS_GetBitsPerPixel() + 7) >> 3;
	ttfont->bmp.pitch = size * ttfont->bmp.bpp;
	IFS_Memset(ttfont->bmp.bits, 0x00, FS_TT_MAX_FONT_W * FS_TT_MAX_FONT_H * 2);

	npts = glyph->data.contours_end_pts[glyph->contours_num - 1];
	px = glyph->data.xcoord;
	py = glyph->data.ycoord;
	pf = glyph->data.flags;

	x = y = 0;
	for( i = 0; i < npts; i ++, pf ++ )
	{
		switch(pf[0] & FS_TT_G_XMASK)
		{
			case FS_TT_G_X_NAV_BYTE:
				xunit = -1;
				break;
			case FS_TT_G_X_POS_BYTE:	
				xunit = 1;
				break;
			case FS_TT_G_X_SHORT:
				xunit = 2;
				break;
			default:	/*  FS_TT_G_X_THE_SAME*/
				break;
		}
		
		switch(pf[0] & FS_TT_G_YMASK)
		{
			case FS_TT_G_Y_NAV_BYTE:
				yunit = -1;
				break;
			case FS_TT_G_Y_POS_BYTE:	
				yunit = 1;
				break;
			case FS_TT_G_Y_SHORT:
				yunit = 2;
				break;
			default:	/*  FS_TT_G_Y_THE_SAME*/
				break;
		}
		
		if(pf[0] & FS_TT_G_REPEAT)
		{
			for( k = 0; k < pf[1] + 1; k ++ )
			{
				if( xunit < 2 )
				{
					dx = px[0];
					x += xunit * dx * size / ttfont->head.units_per_em; 
					if( (pf[0] & FS_TT_G_XMASK) != FS_TT_G_X_THE_SAME )
					{
						px ++;
					}
				}
				else
				{
					FS_TT_READ_2(px, dx);
					x += dx * size / ttfont->head.units_per_em; 
					if( (pf[0] & FS_TT_G_XMASK) != FS_TT_G_X_THE_SAME )
					{
						px += 2;
					}
				}
				
				if( yunit < 2 )
				{
					dy = py[0];
					y += yunit * dy * size / ttfont->head.units_per_em;
					if( (pf[0] & FS_TT_G_YMASK) != FS_TT_G_Y_THE_SAME )
					{
						py ++;
					}
				}
				else
				{
					FS_TT_READ_2(py, dy);
					y += dy * size / ttfont->head.units_per_em;
					if( (pf[0] & FS_TT_G_YMASK) != FS_TT_G_Y_THE_SAME )
					{
						py += 2;
					}
				}

				FS_TT_DRAW_PIXEL( bmp, x + xorg, y + yorg );
			}
			i += pf[1];
			pf ++;
		}
		else
		{
			if( xunit < 2 )
			{
				dx = px[0];
				x += xunit * dx * size / ttfont->head.units_per_em;	
				if( (pf[0] & FS_TT_G_XMASK) != FS_TT_G_X_THE_SAME )
				{
					px ++;
				}
			}
			else
			{
				FS_TT_READ_2(px, dx);
				x += dx * size / ttfont->head.units_per_em;	
				if( (pf[0] & FS_TT_G_XMASK) != FS_TT_G_X_THE_SAME )
				{
					px += 2;
				}
			}

			if( yunit < 2 )
			{
				dy = py[0];
				y += yunit * dy * size / ttfont->head.units_per_em;
				if( (pf[0] & FS_TT_G_YMASK) != FS_TT_G_Y_THE_SAME )
				{
					py ++;
				}
			}
			else
			{
				FS_TT_READ_2(py, dy);
				y += dy * size / ttfont->head.units_per_em;
				if( (pf[0] & FS_TT_G_YMASK) != FS_TT_G_Y_THE_SAME )
				{
					py += 2;
				}
			}
			FS_TT_DRAW_PIXEL( bmp, x + xorg, y + yorg );
		}
	}
	return FS_TT_OK;
}

static FS_SINT4 FS_TTReadLoca( FS_TTFont *ttfont )
{
	FS_UINT2 i;
	FS_BYTE *buf;
	FS_TTLoca *loca = &ttfont->loca;
	FS_TTDirEntry *entry;
	
	entry = FS_TTFindDirEntry(ttfont, FS_TAB_DIR_LOCA);
	if( entry == FS_NULL )
	{
		return FS_TT_ERR_INVALID_FONT_FILE;
	}
	
	loca->num = ttfont->maxp.glyph_num + 1;
	loca->offsets = IFS_Malloc( loca->num * 4 );
	IFS_Memset(loca->offsets, 0, loca->num * 4 );
	if( ttfont->head.index_to_loc == 0 )
	{
		buf = IFS_Malloc( loca->num * 2 );
		FS_FileRead(FS_DIR_ROOT, ttfont->font_file, entry->offset, buf, loca->num * 2);
		for( i = 0; i < loca->num; i ++ )
		{
			FS_TT_READ_2(buf + i * 2, loca->offsets[i]);
			loca->offsets[i] <<= 1;
		}
		IFS_Free( buf );
		/*
		 * value for numGlyphs is found in the 'maxp' table. But the value in the 'loca' table should agree.
		 */
		if( entry->len != loca->num << 1 )
		{
			return FS_TT_ERR_INVALID_FONT_FILE;
		}
	}
	else if(ttfont->head.index_to_loc == 1)
	{
		FS_FileRead(FS_DIR_ROOT, ttfont->font_file, entry->offset, loca->offsets, loca->num * 4);
		FS_TTSwapByte4(loca->offsets, loca->num);
		/*
		 * value for numGlyphs is found in the 'maxp' table. But the value in the 'loca' table should agree.
		 */		
		if( entry->len != loca->num << 2 )
		{
			return FS_TT_ERR_INVALID_FONT_FILE;
		}
	}
	else
	{
		return FS_TT_ERR_INVALID_FONT_FILE;
	}
	return FS_TT_OK;
}

static FS_SINT4 FS_TTReadMaxp( FS_TTFont *ttfont )
{
	FS_BYTE head[32];
	FS_TTMaxp *maxp = &ttfont->maxp;
	FS_TTDirEntry *entry;
	
	entry = FS_TTFindDirEntry(ttfont, FS_TAB_DIR_MAXP);
	if( entry == FS_NULL )
	{
		return FS_TT_ERR_INVALID_FONT_FILE;
	}

	FS_FileRead( FS_DIR_ROOT, ttfont->font_file, entry->offset, head, 32 );
	FS_TT_READ_4(head, maxp->version);
	FS_TT_READ_2(head + 4, maxp->glyph_num);
	FS_TT_READ_2(head + 6, maxp->max_points);
	FS_TT_READ_2(head + 8, maxp->max_contours);
	FS_TT_READ_2(head + 10, maxp->max_composite_points);
	FS_TT_READ_2(head + 12, maxp->max_composite_contours);

	return FS_TT_OK;
}

static FS_SINT4 FS_TTReadHead( FS_TTFont *ttfont )
{
	FS_BYTE hbuf[54];
	FS_TTHead *head = &ttfont->head;
	FS_TTDirEntry *entry;
	
	entry = FS_TTFindDirEntry(ttfont, FS_TAB_DIR_HEAD);
	if( entry == FS_NULL )
	{
		return FS_TT_ERR_INVALID_FONT_FILE;
	}

	FS_FileRead( FS_DIR_ROOT, ttfont->font_file, entry->offset, hbuf, 54 );

	FS_TT_READ_4(hbuf, head->version);
	FS_TT_READ_4(hbuf + 4, head->font_revision);
	FS_TT_READ_4(hbuf + 8, head->check_sum_adj);
	FS_TT_READ_4(hbuf + 12, head->magic);
	if(head->magic != 0x5F0F3CF5){
		return FS_TT_ERR_INVALID_FONT_FILE;
	}
	FS_TT_READ_2(hbuf + 16, head->flags);
	FS_TT_READ_2(hbuf + 18, head->units_per_em);
	if(head->units_per_em < 16 || head->units_per_em > 16384){
		return FS_TT_ERR_INVALID_FONT_FILE;
	}
	/* skip created and modified time 16 bytes */
	FS_TT_READ_2(hbuf + 36, head->xmin);
	FS_TT_READ_2(hbuf + 38, head->ymin);
	FS_TT_READ_2(hbuf + 40, head->xmax);
	FS_TT_READ_2(hbuf + 42, head->ymax);

	FS_TT_READ_2(hbuf + 44, head->mac_style);
	FS_TT_READ_2(hbuf + 46, head->lowest_rec_ppem);
	FS_TT_READ_2(hbuf + 48, head->direction);
	FS_TT_READ_2(hbuf + 50, head->index_to_loc);
	FS_TT_READ_2(hbuf + 52, head->glyph_data_format);
	if(head->glyph_data_format != 0){
		return FS_TT_ERR_INVALID_FONT_FILE;
	}
	return FS_TT_OK;	
}

static FS_SINT4 FS_TTReadCmap( FS_TTFont *ttfont )
{
	FS_BYTE head[22];
	FS_TTCmap *cmap = &ttfont->cmap;
	FS_TTCmapCode *encode_tab;
	FS_UINT2 i;
	FS_TTDirEntry *entry;
	FS_UINT4 offset;
	
	entry = FS_TTFindDirEntry(ttfont, FS_TAB_DIR_CMAP);
	if( entry == FS_NULL )
	{
		return FS_TT_ERR_INVALID_FONT_FILE;
	}
	
	FS_FileRead(FS_DIR_ROOT, ttfont->font_file, entry->offset, head, 4);
	FS_TT_READ_2(head, cmap->version);
	FS_TT_READ_2(head + 2, cmap->tab_num);
	cmap->encode_tabs = IFS_Malloc( cmap->tab_num * sizeof(FS_TTCmapCode) );
	IFS_Memset(cmap->encode_tabs, 0, cmap->tab_num * sizeof(FS_TTCmapCode) );
	offset = entry->offset + 4;
	for( i = 0; i < cmap->tab_num; i ++ )
	{
		encode_tab = cmap->encode_tabs + i;
		FS_FileRead(FS_DIR_ROOT, ttfont->font_file, offset, head, 22);
		FS_TT_READ_2(head, encode_tab->platform_id);
		FS_TT_READ_2(head + 2, encode_tab->encoding_id);
		FS_TT_READ_4(head + 4, encode_tab->offset);
		if( encode_tab->platform_id != 3 || encode_tab->encoding_id != 1 )
		{
			return FS_TT_ERR_ENCODING_NOT_SUPPORT;
		}
		FS_TT_READ_2(head + 8, encode_tab->format);
		if( encode_tab->format != 4 )
		{
			return FS_TT_ERR_ENCODING_FORMAT_NOT_SUPPORT;
		}
		FS_TT_READ_2(head + 10, encode_tab->len);
		FS_TT_READ_2(head + 12, encode_tab->version);
		FS_TT_READ_2(head + 14, encode_tab->seg_count);
		encode_tab->seg_count >>= 1;
		FS_TT_READ_2(head + 16, encode_tab->search_range);
		FS_TT_READ_2(head + 18, encode_tab->entry_selector);
		FS_TT_READ_2(head + 20, encode_tab->range_shift);

		/* end code table */
		encode_tab->end_code = IFS_Malloc(encode_tab->seg_count * 2);
		FS_FileRead(FS_DIR_ROOT, ttfont->font_file, 
			entry->offset + encode_tab->offset + 14, 
			encode_tab->end_code, encode_tab->seg_count * 2);
		FS_TTSwapByte2(encode_tab->end_code, encode_tab->seg_count);

		/* start code table */
		encode_tab->start_code = IFS_Malloc(encode_tab->seg_count * 2);
		FS_FileRead(FS_DIR_ROOT, ttfont->font_file, 
			entry->offset + encode_tab->offset + 14 + encode_tab->seg_count * 2 + 2, 
			encode_tab->start_code, encode_tab->seg_count * 2);
		FS_TTSwapByte2(encode_tab->start_code, encode_tab->seg_count);

		/* id delta table */
		encode_tab->id_delta = IFS_Malloc(encode_tab->seg_count * 2);
		FS_FileRead(FS_DIR_ROOT, ttfont->font_file, 
			entry->offset + encode_tab->offset + 14 + encode_tab->seg_count * 4 + 2, 
			encode_tab->id_delta, encode_tab->seg_count * 2);
		FS_TTSwapByte2(encode_tab->id_delta, encode_tab->seg_count);

		if( encode_tab->len < 16 + encode_tab->seg_count * 8 )
		{
			return FS_TT_ERR_INVALID_FONT_FILE;
		}
		/* id range offset table and glyph id array table. this two table is connected */
		encode_tab->id_range_offset = IFS_Malloc(encode_tab->len - 16 - encode_tab->seg_count * 6);
		FS_FileRead(FS_DIR_ROOT, ttfont->font_file, 
			entry->offset + encode_tab->offset + 16 + encode_tab->seg_count * 6, 
			encode_tab->id_range_offset, encode_tab->len - 16 - encode_tab->seg_count * 6);
		FS_TTSwapByte2(encode_tab->id_range_offset, (encode_tab->len - 16 - encode_tab->seg_count * 6) >> 1);
		
		/* glyph id array table. this table is calc. reference to id range offset table. did not malloc memory here */
		encode_tab->glyph_id_size = (encode_tab->len - 16 - encode_tab->seg_count * 8) >> 1;
		encode_tab->glyph_id_array = encode_tab->id_range_offset + encode_tab->seg_count * 2;
		offset = entry->offset + encode_tab->offset + encode_tab->len;
	}
	return FS_TT_OK;
}

FS_TTFontHandle FS_TTFontCreate(FS_CHAR *fontfile)
{
	FS_BYTE head[12];
	FS_TTFont *ttfont;
	FS_UINT2 i;
	
	if(FS_FileRead(FS_DIR_ROOT, fontfile, 0, head, 12) != 12){
		return FS_NULL;
	}
	ttfont = IFS_Malloc( sizeof(FS_TTFont) );
	IFS_Memset( ttfont, 0, sizeof(FS_TTFont) );
	ttfont->font_file = IFS_Strdup(fontfile);
	/* tab dir head */
	FS_TT_READ_4(head, ttfont->version);
	FS_TT_READ_2(head + 4, ttfont->tab_num);
	FS_TT_READ_2(head + 6, ttfont->search_range);
	FS_TT_READ_2(head + 8, ttfont->entry_selector);
	FS_TT_READ_2(head + 10, ttfont->range_shift);

	/* read tab dir */
	ttfont->tab_entrys = IFS_Malloc( ttfont->tab_num * sizeof(FS_TTDirEntry) );
	FS_FileRead( FS_DIR_ROOT, fontfile, 12, ttfont->tab_entrys, sizeof(FS_TTDirEntry) * ttfont->tab_num );
	for( i = 0; i < ttfont->tab_num; i ++ ){
		FS_TTSwapByte4(ttfont->tab_entrys + i, 4);
	}

	/* analize tab dir */
	if( FS_TTReadCmap( ttfont ) != FS_TT_OK){
		goto ERR_RET;
	}
	if( FS_TTReadHead( ttfont ) != FS_TT_OK ){
		goto ERR_RET;
	}
	if( FS_TTReadMaxp( ttfont ) != FS_TT_OK ){
		goto ERR_RET;
	}	
	if( FS_TTReadLoca( ttfont ) != FS_TT_OK ){
		goto ERR_RET;
	}

	ttfont->bmp.bpp = (IFS_GetBitsPerPixel() + 7) >> 3;
	ttfont->bmp.bits = IFS_Malloc(FS_TT_MAX_FONT_W * FS_TT_MAX_FONT_H * ttfont->bmp.bpp );
	IFS_Memset(ttfont->bmp.bits, 0xFF, FS_TT_MAX_FONT_W * FS_TT_MAX_FONT_H * ttfont->bmp.bpp );
	return ttfont;

ERR_RET:
	FS_TTFontDestroy(ttfont);
	ttfont = FS_NULL;
	return ttfont;
}

void FS_TTFontDestroy(FS_TTFontHandle hTTFont)
{
	FS_UINT2 i;
	FS_TTFont *ttfont = (FS_TTFont *)hTTFont;

	if( ttfont == FS_NULL ) return;
	FS_SAFE_FREE(ttfont->font_file);
	FS_SAFE_FREE(ttfont->tab_entrys);

	for( i = 0; i < ttfont->cmap.tab_num; i ++ )
	{
		FS_SAFE_FREE(ttfont->cmap.encode_tabs[i].end_code);
		FS_SAFE_FREE(ttfont->cmap.encode_tabs[i].start_code);
		FS_SAFE_FREE(ttfont->cmap.encode_tabs[i].id_delta);
		FS_SAFE_FREE(ttfont->cmap.encode_tabs[i].id_range_offset);
	}
	FS_SAFE_FREE(ttfont->cmap.encode_tabs);
	FS_SAFE_FREE(ttfont->loca.offsets);
	FS_SAFE_FREE(ttfont->glyph.data.contours_end_pts);
	FS_SAFE_FREE(ttfont->glyph.data.ins);
	FS_SAFE_FREE(ttfont->glyph.data.flags);
	FS_SAFE_FREE(ttfont->glyph.data.xcoord);
	FS_SAFE_FREE(ttfont->glyph.data.ycoord);

	FS_SAFE_FREE(ttfont->bmp.bits);
}

FS_Bitmap *FS_TTFontGetBitmap(FS_TTFontHandle hTTFont, FS_UINT2 wchar, FS_SINT4 size)
{
	FS_TTFont *ttfont = (FS_TTFont *)hTTFont;
	FS_TTCmapCode *codetab;
	FS_UINT2 tabindex, index, glyphid;
	FS_UINT4 glyphoff, glyphlen;

	if( ttfont == FS_NULL ) return FS_NULL;
	if( size > FS_TT_MAX_FONT_W ) return FS_NULL;
	index = FS_TTFindCodeSegment(ttfont, wchar, &tabindex);
	if( index == 0xFFFF || tabindex == 0xFFFF ) return FS_NULL;
	codetab = ttfont->cmap.encode_tabs + tabindex;
	if(codetab->start_code[index] > wchar) return FS_NULL;

	/*
	 * If the idRangeOffset value for the segment is not 0, the mapping of character codes relies on glyphIdArray. 
	 * The character code offset from startCode is added to the idRangeOffset value. 
	 * This sum is used as an offset from the current location within idRangeOffset itself to index out the correct 
	 * glyphIdArray value.
	 *
	 * If the idRangeOffset is 0, the idDelta value is added directly to the character code offset (i.e. idDelta[i] + c) 
	 * to get the corresponding glyph index.
	 */
	if(codetab->id_range_offset[index] == 0)
	{
		glyphid = codetab->id_delta[index] + wchar;
		glyphid = codetab->glyph_id_array[glyphid];
	}
	else
	{
		/*
		 * glyphid = *(idRangeOffset[i]/2 + (c - startCount[i]) + &idRangeOffset[i])
		 */
		glyphid = *(codetab->id_range_offset[index] / 2 + (wchar - codetab->start_code[index]) + &codetab->id_range_offset[index]);
		if( glyphid != 0 )
		{
			glyphid += codetab->id_delta[index];
		}
		else
		{
			return FS_NULL;		/* missing glyph */
		}
	}

	if(glyphid >= ttfont->loca.num )
	{
		return FS_NULL;
	}
	glyphoff = ttfont->loca.offsets[glyphid];
	glyphlen = ttfont->loca.offsets[glyphid + 1] - glyphoff;
	/* 
	 * now, we got offsets to the locations of the glyphs in the font, 
	 * relative to the beginning of the glyphData table. 
	 */
	if( FS_TTReadGlyphData( ttfont, glyphoff, glyphlen ) != FS_TT_OK ) return FS_NULL;
	if( FS_TTDrawGlyphOutline( ttfont, size ) != FS_TT_OK ) return FS_NULL;
	
	return &ttfont->bmp;
}

