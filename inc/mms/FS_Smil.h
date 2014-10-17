#ifndef _FS_SMIL_H_
#define _FS_SMIL_H_

#include "inc/FS_Config.h"
#include "inc/util/FS_List.h"

typedef struct FS_SmilRegion_Tag
{
	FS_List			list;
	
	FS_CHAR *		id;
	FS_SINT4		left;
	FS_SINT4		top;
	FS_SINT4		width;
	FS_SINT4		height;
}FS_SmilRegion;

typedef struct FS_SmilObject_Tag
{
	FS_CHAR *		src;
	FS_CHAR *		region;
}FS_SmilObject;

typedef struct FS_SmilPar_Tag
{
	FS_List			list;

	FS_SINT4		dur;
	FS_SmilObject *	text;
	FS_SmilObject *	image;
	FS_SmilObject *	audio;
	FS_SmilObject *	video;
	FS_SmilObject *	ref;
}FS_SmilPar;

typedef struct FS_MmsSmil_Tag
{
	/* head */
	FS_SINT4		root_width;
	FS_SINT4		root_height;
	FS_List 		region_list;
	/* body */
	FS_List 		par_list;
}FS_MmsSmil;

FS_MmsSmil * FS_SmilDecodeFile( FS_CHAR *absFile );

void FS_SmilEncodeFile( FS_CHAR *absFile, FS_MmsSmil *pSmil );

FS_MmsSmil *FS_CreateDefaultMmsSmil( void );

void FS_SmilAddMmsFrame( FS_MmsSmil *pSmil, FS_SINT4 index );

void FS_SmilDelMmsFrame( FS_MmsSmil *pSmil, FS_SINT4 frameNum );

void FS_FreeMmsSmil( FS_MmsSmil *pSmil );

/* frameNum begin from 1 */
void FS_SmilAddFrameImage( FS_MmsSmil *pSmil, FS_SINT4 frameNum, FS_CHAR *cid );

/* frameNum begin from 1 */
void FS_SmilAddFrameVideo( FS_MmsSmil *pSmil, FS_SINT4 frameNum, FS_CHAR *cid );

/* frameNum begin from 1 */
void FS_SmilAddFrameAudio( FS_MmsSmil *pSmil, FS_SINT4 frameNum, FS_CHAR *cid );

/* frameNum begin from 1 */
void FS_SmilAddFrameText( FS_MmsSmil *pSmil, FS_SINT4 frameNum, FS_CHAR *cid );

void FS_SmilDelFrameImage( FS_MmsSmil *pSmil, FS_SINT4 frameNum );

void FS_SmilDelFrameAudio( FS_MmsSmil *pSmil, FS_SINT4 frameNum );

void FS_SmilDelFrameVideo( FS_MmsSmil *pSmil, FS_SINT4 frameNum );

void FS_SmilDelFrameText( FS_MmsSmil *pSmil, FS_SINT4 frameNum );

/* frameNum begin from 1 */
FS_CHAR *FS_SmilGetImageCid( FS_MmsSmil *pSmil, FS_SINT4 frameNum );

/* frameNum begin from 1 */
FS_CHAR *FS_SmilGetAudioCid( FS_MmsSmil *pSmil, FS_SINT4 frameNum );

/* frameNum begin from 1 */
FS_CHAR *FS_SmilGetVideoCid( FS_MmsSmil *pSmil, FS_SINT4 frameNum );

/* frameNum begin from 1 */
FS_CHAR *FS_SmilGetTextCid( FS_MmsSmil *pSmil, FS_SINT4 frameNum );

FS_SINT4 FS_SmilGetFrameCount( FS_MmsSmil *pSmil );

void FS_SmilSetFrameDur( FS_MmsSmil *pSmil, FS_SINT4 frameNum, FS_SINT4 dur );

FS_SINT4 FS_SmilGetFrameDur( FS_MmsSmil *pSmil, FS_SINT4 frameNum );

void FS_SmilLayoutSetImageFirst( FS_MmsSmil *pSmil, FS_BOOL bImgFirst );

FS_BOOL FS_SmilLayoutImageFirst( FS_MmsSmil *pSmil );

#endif
