/****************************************************************************

                                 RiffFile.cpp

						Copyright 2000, John J. Bolton
	----------------------------------------------------------------------

	$Header: //depot/Libraries/RiffFile/RiffFile.cpp#2 $

	$NoKeywords: $

****************************************************************************/

#include "RiffFile.h"

#include <cassert>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include "../Misc/Exceptions.h"


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

RiffFile::RiffFile( char const * filename, char const * type )
{
	if ( !Open( filename, type ) )
	{
		throw ConstructorFailedException();
	}
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

RiffFile::~RiffFile()
{
	Close();
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

_off_t RiffFile::GetCurrentPosition() const
{
	return IsOpen() ? mmioSeek( m_hMmio, 0L, SEEK_CUR ) : -1;
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

_off_t RiffFile::GetChunkSize() const
{
	return IsOpen() ? GetCkInfo().cksize : 0;
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

bool RiffFile::Open( char const * filename, char const * type )
{
	assert( filename );
	assert( type );

		// Open the file. If the file did not open, then abort

	m_hMmio = mmioOpen( const_cast< char * >( filename ), NULL, MMIO_ALLOCBUF ); 
	if ( m_hMmio == NULL )
		return false;

		// Find the RIFF chunk. If that fails, close the file and abort

	if ( !FindRiff( type ) )
	{
		Close();
		return false;
	}

	return true;
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

void RiffFile::Close()
{
	if ( IsOpen() )
	{
		// Close the file
		mmioClose( m_hMmio, 0 );
		m_hMmio = NULL;				// Mark as closed

		// Purge the stack

		while ( !m_CkInfoStack.empty() )
			m_CkInfoStack.pop();
	}
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

bool RiffFile::FindRiff( char const * type )
{
	assert( type );

	MMRESULT	result;

	// Save the current position in case of error

	_off_t const	savedPosition	= GetCurrentPosition();
	if ( savedPosition < 0 )
		return false;

	// Find the RIFF and descend into it

	MMCKINFO	riffCkInfo;
	riffCkInfo.fccType = mmioStringToFOURCC( type, 0 );

	MMCKINFO const * const	parent	= !m_CkInfoStack.empty() ? &GetCkInfo() : 0;

	result = mmioDescend( m_hMmio, &riffCkInfo, parent, MMIO_FINDRIFF );

	// If there is an error then go back to the saved position and abort

	if ( result != MMSYSERR_NOERROR )
	{
		mmioSeek( m_hMmio, savedPosition, SEEK_SET );
		return false;
	}

	// Push the new chunk info on the stack

	m_CkInfoStack.push( riffCkInfo );

	return true;
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

bool RiffFile::FindList( char const * type )
{
	assert( type );

	MMRESULT	result;

	// Make sure the file is open and we have found the RIFF

	if ( !IsOpen() || m_CkInfoStack.empty() )
		return false;

	// Save the current position in case of error

	_off_t const	savedPosition	= GetCurrentPosition();
	if ( savedPosition < 0 )
		return false;

	// Find the LIST and descend into it

	MMCKINFO listCkInfo;
	listCkInfo.fccType = mmioStringToFOURCC( type, 0 ); 

	result = mmioDescend( m_hMmio, &listCkInfo, &GetCkInfo(), MMIO_FINDLIST );

	// If there is an error then go back to the saved position and abort

	if ( result != MMSYSERR_NOERROR )
	{
		mmioSeek( m_hMmio, savedPosition, SEEK_SET );
		return false;
	}

	// Push the new chunk on the stack

	m_CkInfoStack.push( listCkInfo );

	return true;
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

bool RiffFile::FindChunk( char const * id )
{
	assert( id );

	MMRESULT	result;

	// Make sure the file is open and we have found the RIFF

	if ( !IsOpen() || m_CkInfoStack.empty() )
		return false;

	// Save the current position in case of error

	_off_t const savedPosition = GetCurrentPosition();
	if ( savedPosition < 0 )
		return false;

	// Find the chunk and descend into it

	MMCKINFO chunkCkInfo;
    chunkCkInfo.ckid = mmioStringToFOURCC( id, 0 );

	result = mmioDescend( m_hMmio, &chunkCkInfo, &GetCkInfo(), MMIO_FINDCHUNK );

	// If there is an error then go back to the saved position and abort

	if ( result != MMSYSERR_NOERROR )
	{
		mmioSeek( m_hMmio, savedPosition, SEEK_SET );
		return false;
	}

	// Push the new chunk on the stack

	m_CkInfoStack.push( chunkCkInfo );

	return true;
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

bool RiffFile::Descend()
{

	MMRESULT	result;

	// Make sure the file is open and we have found the RIFF

	if ( !IsOpen() || m_CkInfoStack.empty() )
		return false;

		// Descend

	MMCKINFO newCkInfo;

	result = mmioDescend( m_hMmio, &newCkInfo, &GetCkInfo(), 0 );

		// If there is an error then abort

	if ( result != MMSYSERR_NOERROR )
		return false;

		// Push the new chunk on the stack

	m_CkInfoStack.push( newCkInfo );

	return true;
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

bool RiffFile::Ascend()
{
	MMRESULT	result;

	// Make sure the file is open and we have found the RIFF

	if ( !IsOpen() || m_CkInfoStack.empty() )
		return false;

		// Ascend

    result = mmioAscend( m_hMmio, &GetCkInfo(), 0 );

		// If there is an error then abort

	if ( result != MMSYSERR_NOERROR )
		return false;

		// Pop the old chunk off the stack

	m_CkInfoStack.pop();

	return true;
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

long RiffFile::Read( void * buffer, size_t n )
{
	assert( buffer );
	assert( n > 0 );

	if ( !IsOpen() )
		return -1;

	_off_t const savedPosition = GetCurrentPosition();
	if ( savedPosition < 0 )
		return -1;

	// If we are reading past the end of the chunk, read to the end of the chunk.

	_off_t const maxRead = GetCkInfo().dwDataOffset + GetCkInfo().cksize - savedPosition;
	if ( n > (unsigned)maxRead )
		n = maxRead;

	return mmioRead( m_hMmio, ( HPSTR )buffer, n );
}


/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

_off_t RiffFile::Skip( _off_t n )
{
	assert( n >= 0 );

	if ( !IsOpen() )
		return -1;

	_off_t const	savedPosition	= GetCurrentPosition();
	if ( savedPosition < 0 )
		return -1;

	// If we are skippng past the end of the chunk, skip to the end of the chunk.

	_off_t const	maxSkip	= GetCkInfo().dwDataOffset + GetCkInfo().cksize - savedPosition;
	if ( n > maxSkip )
		n = maxSkip;

		// Seek

	_off_t const newPosition = mmioSeek( m_hMmio, n, SEEK_CUR );

		// If error, return error

	if ( newPosition < 0 )
		return newPosition;

		// Return the number of of bytes skipped

	return ( newPosition - savedPosition );
}



