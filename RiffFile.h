#if !defined( RIFFFILE_H_INCLUDED )
#define RIFFFILE_H_INCLUDED

#pragma once

/****************************************************************************

                                  RiffFile.h

						Copyright 2000, John J. Bolton
	----------------------------------------------------------------------

	$Header: //depot/Libraries/RiffFile/RiffFile.h#2 $

	$NoKeywords: $

****************************************************************************/

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include <stack>

/********************************************************************************************************************/
/*																													*/
/*																													*/
/********************************************************************************************************************/

class RiffFile
{
public:

	RiffFile( char const * filename, char const * type );

	virtual ~RiffFile();

	// Returns the size of the current chunk
	_off_t GetChunkSize() const;

	// Move to the specified LIST
	bool FindList( char const * type );

	// Move the specified chunk
	bool FindChunk( char const * type );

	// Descend into the current chunk
	bool Descend();

	// Ascend out of the current chunk
	bool Ascend();

	// Read the next n bytes, returns the number of bytes read or -1 if error
	// Note: You cannot read past the end of a chunk.
	long Read( void * buffer, size_t n );

	// Skip the next n bytes, returns the number of bytes skipped or -1 if error
	// Note: You cannot skip past the end of a chunk.
	_off_t Skip( _off_t n );

protected:

	// Close the file
	void Close();

	// Return true if the file is currently open
	bool IsOpen() const								{ return m_hMmio != NULL;		}
	
private:

	typedef std::stack< struct _MMCKINFO > CkInfoStack;

	// Open the file, return true if successful
	bool Open( char const * filename, char const * type );

	// Move to the specified RIFF
	bool FindRiff( char const * type );

	// Returns the current position in the file, or -1 if error
	_off_t GetCurrentPosition() const;

	// Returns the current chunk info struct
	struct _MMCKINFO &			GetCkInfo()			{ return m_CkInfoStack.top();	}
	struct _MMCKINFO const &	GetCkInfo() const	{ return m_CkInfoStack.top();	}

	struct HMMIO__ *	m_hMmio;			// File handle
	CkInfoStack			m_CkInfoStack;		// Chunk info stack

};



#endif // !defined( RIFFFILE_H_INCLUDED )
