//===-- MICmnLogMediumFile.cpp ----------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//++
// File:		MICmnLogMediumFile.cpp
//
// Overview:	CMICmnLogMediumFile implementation.
//
// Environment:	Compilers:	Visual C++ 12.
//							gcc (Ubuntu/Linaro 4.8.1-10ubuntu9) 4.8.1
//				Libraries:	See MIReadmetxt. 
//
// Copyright:	None.
//--

// Include compiler configuration
#include "MICmnConfig.h"

// In-house headers:
#include "MICmnLogMediumFile.h"
#include "MICmnResources.h"

#if defined( _MSC_VER )
	#include "MIUtilSystemWindows.h"
#elif defined( __linux )
	#include "MIUtilSystemLinux.h"
#elif defined( __APPLE__ )
	#include "MIUtilSystemOsx.h"
#endif // defined( _MSC_VER )

#include "MIUtilDateTimeStd.h"

//++ ------------------------------------------------------------------------------------
// Details:	CMICmnLogMediumFile constructor.
// Type:	Method.
// Args:	None.
// Return:	None.
// Throws:	None.
//--
CMICmnLogMediumFile::CMICmnLogMediumFile( void )
:	m_constThisMediumName( MIRSRC( IDS_MEDIUMFILE_NAME ) )
,	m_constMediumFileName( "lldb-mi-log.txt" )
,	m_fileNamePath( MIRSRC( IDS_MEDIUMFILE_ERR_INVALID_PATH ) )
,	m_eVerbosityType( CMICmnLog::eLogVerbosity_Log )
,	m_strDate( CMIUtilDateTimeStd().GetDate() )
,	m_fileHeaderTxt( MIRSRC( IDS_MEDIUMFILE_ERR_FILE_HEADER ) )
{
}

//++ ------------------------------------------------------------------------------------
// Details:	CMICmnLogMediumFile destructor.
// Type:	Overridden.
// Args:	None.
// Return:	None.
// Throws:	None.
//--
CMICmnLogMediumFile::~CMICmnLogMediumFile( void )
{
}

//++ ------------------------------------------------------------------------------------
// Details:	Get the singleton instance of *this class.
// Type:	Static.
// Args:	None.
// Return:	CMICmnLogMediumFile - Reference to *this object.
// Throws:	None.
//--
CMICmnLogMediumFile & CMICmnLogMediumFile::Instance( void )
{
	static CMICmnLogMediumFile instance;

	return instance;
}

//++ ------------------------------------------------------------------------------------
// Details:	Initialize setup *this medium ready for use.
// Type:	Overridden.
// Args:	None.
// Return:	MIstatus::success - Functional succeeded.
//			MIstatus::failure - Functional failed.
// Throws:	None.
//--
bool CMICmnLogMediumFile::Initialize( void )
{
	m_bInitialized = FileFormFileNamePath();
	
	return m_bInitialized;
}

//++ ------------------------------------------------------------------------------------
// Details:	Unbind detach or release resources used by *this medium.
// Type:	Method.
// Args:	None.
// Return:	None.
// Throws:	None.
//--
bool CMICmnLogMediumFile::Shutdown( void )
{
	if( m_bInitialized )
	{
		m_bInitialized = false;
		m_file.Close();
	}
	return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details:	Retrieve the name of *this medium.
// Type:	Overridden.
// Args:	None.
// Return:	CMIUtilString - Text data.
// Throws:	None.
//--
const CMIUtilString & CMICmnLogMediumFile::GetName( void ) const
{
	return m_constThisMediumName;
}

//++ ------------------------------------------------------------------------------------
// Details:	The callee client calls the write function on the Logger. The data to be
//			written is given out to all the mediums registered. The verbosity type parameter 
//			indicates to the medium the type of data or message given to it. The medium has
//			modes of verbosity and depending on the verbosity set determines which data is
//			sent to the medium's output.
// Type:	Method.
// Args:	vData		- (R) The data to write to the logger.
//			veType		- (R) Verbosity type.
// Return:	MIstatus::success - Functional succeeded.
//			MIstatus::failure - Functional failed.
// Throws:	None.
//--
bool CMICmnLogMediumFile::Write( const CMIUtilString & vData, const CMICmnLog::ELogVerbosity veType )
{
	if( m_bInitialized && m_file.IsOk() )
	{
		const bool bDoWrite = (m_eVerbosityType & veType);
		if( bDoWrite )
		{
			bool bNewCreated = false;
			bool bOk = m_file.CreateWrite( m_fileNamePath, bNewCreated );
			if( bOk )
			{
				if( bNewCreated )
					bOk = FileWriteHeader();
				bOk = bOk && FileWriteEnglish( MassagedData( vData, veType ) );
			}
			return bOk;
		}
	}
	
	return MIstatus::failure;
}

//++ ------------------------------------------------------------------------------------
// Details:	Retrieve *this medium's last error condition.
// Type:	Method.
// Args:	None.
// Return:	CString & -  Text description.
// Throws:	None.
//--
const CMIUtilString & CMICmnLogMediumFile::GetError( void ) const
{
	return m_strMILastErrorDescription;
}

//++ ------------------------------------------------------------------------------------
// Details:	Set the verbosity mode for this medium.
// Type:	Method.
// Args:	veType	- (R) Mask value.
// Return:	MIstatus::success - Functional succeeded.
//			MIstatus::failure - Functional failed.
// Throws:	None.
//--
bool CMICmnLogMediumFile::SetVerbosity( const MIuint veType )
{
	m_eVerbosityType = veType;
	return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details:	Get the verbosity mode for this medium.
// Type:	Method.
// Args:	veType	- (R) Mask value.
// Return:	CMICmnLog::ELogVerbosity - Mask value.
// Throws:	None.
//--
MIuint CMICmnLogMediumFile::GetVerbosity( void ) const
{
	return m_eVerbosityType;
}

//++ ------------------------------------------------------------------------------------
// Details:	Write data to a file English font.
// Type:	Method.
// Args:	vData	- (R) The data to write to the logger.
// Return:	None.
// Throws:	None.
//--
bool CMICmnLogMediumFile::FileWriteEnglish( const CMIUtilString & vData )
{
	return m_file.Write( vData );
}

//++ ------------------------------------------------------------------------------------
// Details:	Determine and form the medium file's directory path and name.
// Type:	Method.
// Args:	None.
// Return:	MIstatus::success - Functional succeeded.
//			MIstatus::failure - Functional failed.
// Throws:	None.
//--
bool CMICmnLogMediumFile::FileFormFileNamePath( void )
{
	ClrErrorDescription();

	m_fileNamePath = MIRSRC( IDS_MEDIUMFILE_ERR_INVALID_PATH );

	CMIUtilString strPathName;
	if( CMIUtilSystem().GetLogFilesPath( strPathName ) )
	{
		const CMIUtilString strPath = CMIUtilFileStd().StripOffFileName( strPathName );

		// ToDo: Review this LINUX log file quick fix so not hidden
        // AD: 
        //      Linux was creating a log file here called '.\log.txt'.  The '.' on linux
        //      signifies that this file is 'hidden' and not normally visible.  A quick fix
        //      is to remove the path component all together.  Linux also normally uses '/'
        //      as directory separators, again leading to the problem of the hidden log.
#if defined ( _MSC_VER )
        m_fileNamePath = CMIUtilString::Format( "%s\\%s", strPath.c_str(), m_constMediumFileName.c_str() );
#else
        m_fileNamePath = CMIUtilString::Format( "%s", m_constMediumFileName.c_str() );
#endif // defined ( _MSC_VER )

		return MIstatus::success;
	}

	SetErrorDescription( MIRSRC( IDE_MEDIUMFILE_ERR_GET_FILE_PATHNAME_SYS ) );
	
	return MIstatus::failure;
}

//++ ------------------------------------------------------------------------------------
// Details:	Retrieve the medium file's directory path and name.
// Type:	Method.
// Args:	None.
// Return:	CMIUtilString & - File path.
// Throws:	None.
//--
const CMIUtilString & CMICmnLogMediumFile::GetFileNamePath( void ) const
{
	return m_fileNamePath;
}

//++ ------------------------------------------------------------------------------------
// Details:	Retrieve the medium file's name.
// Type:	Method.
// Args:	None.
// Return:	CMIUtilString & - File name.
// Throws:	None.
//--
const CMIUtilString & CMICmnLogMediumFile::GetFileName( void ) const
{
	return m_constMediumFileName;
}

//++ ------------------------------------------------------------------------------------
// Details:	Massage the data to behave correct when submitted to file. Insert extra log
//			specific text. The veType is there to allow in the future to parse the log and
//			filter in out specific types of message to make viewing the log more manageable.
// Type:	Method.
// Args:	vData	- (R) Raw data.
//			veType	- (R) Message type.
// Return:	CMIUtilString - Massaged data.
// Throws:	None.
//--
CMIUtilString CMICmnLogMediumFile::MassagedData( const CMIUtilString & vData, const CMICmnLog::ELogVerbosity veType )
{
	const CMIUtilString strCr( "\n" );
	CMIUtilString data;
	const MIchar verbosityCode( ConvertLogVerbosityTypeToId( veType ) );
	const CMIUtilString dt( CMIUtilString::Format( "%s %s", m_strDate.c_str(), CMIUtilDateTimeStd().GetTime().c_str() ) );
	
	data = CMIUtilString::Format( "%c,%s,%s", verbosityCode, dt.c_str(), vData.c_str() );
	data = ConvertCr( data );

	// Look for EOL...
	const MIint pos = vData.rfind( strCr );
	if( pos == (MIint) vData.size() )
		return data;
	
	// ... did not have an EOL so add one
	data += GetLineReturn();
	
	return data;
}

//++ ------------------------------------------------------------------------------------
// Details:	Convert the Log's verbosity type number into a single char character.
// Type:	Method.
// Args:	veType	- (R) Message type.
// Return:	wchar_t - A letter.
// Throws:	None.
//--
MIchar CMICmnLogMediumFile::ConvertLogVerbosityTypeToId( const CMICmnLog::ELogVerbosity veType ) const
{
	MIchar c = 0;
	if( veType != 0 )
	{
		MIuint cnt = 0;
		MIuint number( veType );
		while( 1 != number )
		{
			number = number >> 1;
			++cnt;
		}
		c = 'A' + cnt;
	}
	else
	{
		c = '*';
	}

	return c;
}

//++ ------------------------------------------------------------------------------------
// Details:	Retrieve state of whether the file medium is ok.
// Type:	Method.
// Args:	None.
// Return:	True - file ok.
//			False - file has a problem.
// Throws:	None.
//--
bool CMICmnLogMediumFile::IsOk( void ) const
{
	return m_file.IsOk();
}

//++ ------------------------------------------------------------------------------------
// Details:	Status on the file log medium existing already.
// Type:	Method.
// Args:	None.
// Return:	True - Exists.
//			False - Not found.
// Throws:	None.
//--
bool CMICmnLogMediumFile::IsFileExist( void ) const
{
	return m_file.IsFileExist( GetFileNamePath() );
}

//++ ------------------------------------------------------------------------------------
// Details:	Write the header text the logger file.
// Type:	Method.
// Args:	vText	- (R) Text.
// Return:	MIstatus::success - Functional succeeded.
//			MIstatus::failure - Functional failed.
// Throws:	None.
//--
bool CMICmnLogMediumFile::FileWriteHeader( void )
{
	return FileWriteEnglish( ConvertCr( m_fileHeaderTxt ) );
}

//++ ------------------------------------------------------------------------------------
// Details:	Convert any carriage line returns to be compatible with the platform the
//			Log fiel is being written to.
// Type:	Method.
// Args:	vData	- (R) Text data.
// Return:	CMIUtilString - Converted string data.
// Throws:	None.
//--
CMIUtilString CMICmnLogMediumFile::ConvertCr( const CMIUtilString & vData ) const
{
	const CMIUtilString strCr( "\n" );
	const CMIUtilString & rCrCmpat( GetLineReturn() );

	if( strCr == rCrCmpat )
		return vData;

	const MIuint nSizeCmpat( rCrCmpat.size() );
	const MIuint nSize( strCr.size() );
	CMIUtilString strConv( vData );
	MIint pos = strConv.find( strCr );
	while( pos != (MIint) CMIUtilString::npos )
	{
		strConv.replace( pos, nSize, rCrCmpat );
		pos = strConv.find( strCr, pos + nSizeCmpat );
	}

	return strConv;
}

//++ ------------------------------------------------------------------------------------
// Details:	Set the header text that is written to the logger file at the begining.
// Type:	Method.
// Args:	vText	- (R) Text.
// Return:	MIstatus::success - Functional succeeded.
//			MIstatus::failure - Functional failed.
// Throws:	None.
//--
bool CMICmnLogMediumFile::SetHeaderTxt( const CMIUtilString & vText )
{
	m_fileHeaderTxt = vText;

	return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details:	Retrieve the file current carriage line return characters used.
// Type:	Method.
// Args:	None.
// Return:	CMIUtilString & - Text.
// Throws:	None.
//--
const CMIUtilString & CMICmnLogMediumFile::GetLineReturn( void ) const
{
	return m_file.GetLineReturn();
}