/***********************************************************************
**
**  SUBPACKET.C -- Subfield packet handling
**
**  Author: Bj”rn Stenberg (bjorn.stenberg@sth.frontec.se)
**
***********************************************************************/
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "jam.h"

/***********************************************************************
**
**  JAM_NewSubPacket - Create a new subfield packet
**
***********************************************************************/
s_JamSubPacket* JAM_NewSubPacket( void )
{
    s_JamSubPacket* Sub_PS;

    /* allocate packet struct */
    Sub_PS = (s_JamSubPacket*) malloc( sizeof( s_JamSubPacket ) );
    if ( !Sub_PS )
	return NULL;

    Sub_PS->NumAlloc  = 20;
    Sub_PS->NumFields = 0;

    /* allocate pointer array */
    Sub_PS->Fields    = (s_JamSubfield**) malloc( Sub_PS->NumAlloc *
						  sizeof( s_JamSubfield* ) );
    if ( !Sub_PS->Fields )
	return NULL;

    /* clear pointer array */
    memset( Sub_PS->Fields, 0, Sub_PS->NumAlloc * sizeof( s_JamSubfield* ) );

    return Sub_PS;
}

/***********************************************************************
**
**  JAM_FreeSubPacket - Free the data associated with a subfield packet
**
***********************************************************************/
int JAM_DelSubPacket( s_JamSubPacket* SubPack_PS )
{
    int i;

    if (!SubPack_PS)
	return JAM_BAD_PARAM;
    
    for ( i=0; i < SubPack_PS->NumFields; i++ ) {
	s_JamSubfield* Field_PS = SubPack_PS->Fields[i];
	
	if ( Field_PS->Buffer )
	    free( Field_PS->Buffer );
	free( Field_PS );
    }
    free( SubPack_PS->Fields );
    free( SubPack_PS );

    return 0;
}

/***********************************************************************
**
**  JAM_GetSubfield -- Get first/next subfield from a subfield packet
**
***********************************************************************/
s_JamSubfield* JAM_GetSubfield( s_JamSubPacket* SubPack_PS )
{
    static s_JamSubPacket* LastPack_PS = NULL;
    static int             NextIndex_I = 0;

    if ( SubPack_PS ) {
	LastPack_PS = SubPack_PS;
	NextIndex_I = 0;
    }

    if ( NextIndex_I < LastPack_PS->NumFields )
	return LastPack_PS->Fields[ NextIndex_I++ ];

    return NULL;
}

/***********************************************************************
**
**  JAM_PutSubfield -- Add a subfield to a subfield packet
**
***********************************************************************/
int JAM_PutSubfield( s_JamSubPacket* SubPack_PS, s_JamSubfield* Field_PS )
{
    s_JamSubfield* 	NewField_PS;
    uchar*		NewBuf_PC;

    /* do we have to expand the array? */
    if ( SubPack_PS->NumFields == SubPack_PS->NumAlloc ) {
	s_JamSubfield** Fields_PPS;

	SubPack_PS->NumAlloc *= 2;
	Fields_PPS = (s_JamSubfield**) realloc( SubPack_PS->Fields,
					        SubPack_PS->NumAlloc *
					        sizeof( s_JamSubfield* ) );
	if ( !Fields_PPS )
	    return JAM_NO_MEMORY;

        SubPack_PS->Fields=Fields_PPS;
    }

    /*
    **  Copy the passed subfield
    */

    /* allocate a new subfield */
    NewField_PS = (s_JamSubfield*) malloc( sizeof( s_JamSubfield ) );
    if ( !NewField_PS )
	return JAM_NO_MEMORY;

    /* allocate a new buffer */
    if ( Field_PS->DatLen ) {
	NewBuf_PC = (uchar*) malloc( Field_PS->DatLen );
	if ( !NewBuf_PC )
	    return JAM_NO_MEMORY;
	memcpy( NewBuf_PC, Field_PS->Buffer, Field_PS->DatLen );
    }
    else
	NewBuf_PC = NULL;

    /* copy field struct */
    NewField_PS->LoID   = Field_PS->LoID;
    NewField_PS->HiID   = Field_PS->HiID;
    NewField_PS->DatLen = Field_PS->DatLen;
    NewField_PS->Buffer = NewBuf_PC;


    /*
    **  Update subfield packet
    */

    SubPack_PS->Fields[ SubPack_PS->NumFields ] = NewField_PS;
    SubPack_PS->NumFields++;

    return 0;
}
