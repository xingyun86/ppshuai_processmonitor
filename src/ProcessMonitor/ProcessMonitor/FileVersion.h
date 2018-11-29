#pragma once
// FileVersion.h: interface for the CFileVersion class.
// by Manuel Laflamme
//////////////////////////////////////////////////////////////////////
#ifndef __FILEVERSION_H_
#define __FILEVERSION_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CFileVersion
{
	// Construction
public:
	CFileVersion()
	{
		m_lpVersionData = NULL;
		m_dwLangCharset = 0;
	}
	// Operations	
public:
	BOOL    Open(LPCTSTR lpszModuleName)
	{
		ATLASSERT(_tcslen(lpszModuleName) > 0);
		ATLASSERT(m_lpVersionData == NULL);
		// Get the version information size for allocate the buffer
		DWORD dwHandle;
		DWORD dwDataSize = ::GetFileVersionInfoSize((LPTSTR)lpszModuleName, &dwHandle);
		if (dwDataSize == 0)
			return FALSE;
		// Allocate buffer and retrieve version information
		m_lpVersionData = new BYTE[dwDataSize];
		if (!::GetFileVersionInfo((LPTSTR)lpszModuleName, dwHandle, dwDataSize,
			(void**)m_lpVersionData))
		{
			Close();
			return FALSE;
		}
		// Retrieve the first language and character-set identifier
		UINT nQuerySize;
		DWORD* pTransTable;
		if (!::VerQueryValue(m_lpVersionData, _T("\\VarFileInfo\\Translation"),
			(void **)&pTransTable, &nQuerySize))
		{
			Close();
			return FALSE;
		}
		// Swap the words to have lang-charset in the correct format
		m_dwLangCharset = MAKELONG(HIWORD(pTransTable[0]), LOWORD(pTransTable[0]));
		return TRUE;
	}
	void    Close()
	{
		delete[] m_lpVersionData;
		m_lpVersionData = NULL;
		m_dwLangCharset = 0;
	}
	CString QueryValue(LPCTSTR lpszValueName, DWORD dwLangCharset = 0)
	{
		// Must call Open() first
		ATLASSERT(m_lpVersionData != NULL);
		if (m_lpVersionData == NULL)
			return (CString)_T("");
		// If no lang-charset specified use default
		if (dwLangCharset == 0)
			dwLangCharset = m_dwLangCharset;
		// Query version information value
		UINT nQuerySize;
		LPVOID lpData;
		CString strValue, strBlockName;
		strBlockName.Format(_T("\\StringFileInfo\\%08lx\\%s"),
			dwLangCharset, lpszValueName);
		if (::VerQueryValue((void **)m_lpVersionData, strBlockName.GetBuffer(0),
			&lpData, &nQuerySize))
			strValue = (LPCTSTR)lpData;
		strBlockName.ReleaseBuffer();
		return strValue;
	}
	CString GetFileDescription() { return QueryValue(_T("FileDescription")); };
	CString GetFileVersion() { return QueryValue(_T("FileVersion")); };
	CString GetInternalName() { return QueryValue(_T("InternalName")); };
	CString GetCompanyName() { return QueryValue(_T("CompanyName")); };
	CString GetLegalCopyright() { return QueryValue(_T("LegalCopyright")); };
	CString GetOriginalFilename() { return QueryValue(_T("OriginalFilename")); };
	CString GetProductName() { return QueryValue(_T("ProductName")); };
	CString GetProductVersion() { return QueryValue(_T("ProductVersion")); };
	BOOL    GetFixedInfo(VS_FIXEDFILEINFO& vsffi)
	{
		// Must call Open() first
		ATLASSERT(m_lpVersionData != NULL);
		if (m_lpVersionData == NULL)
			return FALSE;
		UINT nQuerySize;
		VS_FIXEDFILEINFO* pVsffi;
		if (::VerQueryValue((void **)m_lpVersionData, _T("\\"),
			(void**)&pVsffi, &nQuerySize))
		{
			vsffi = *pVsffi;
			return TRUE;
		}
		return FALSE;
	}
	CString GetFixedFileVersion()
	{
		CString strVersion;
		VS_FIXEDFILEINFO vsffi;
		if (GetFixedInfo(vsffi))
		{
			strVersion.Format(_T("%u,%u,%u,%u"), HIWORD(vsffi.dwFileVersionMS),
				LOWORD(vsffi.dwFileVersionMS),
				HIWORD(vsffi.dwFileVersionLS),
				LOWORD(vsffi.dwFileVersionLS));
		}
		return strVersion;
	}
	CString GetFixedProductVersion()
	{
		CString strVersion;
		VS_FIXEDFILEINFO vsffi;
		if (GetFixedInfo(vsffi))
		{
			strVersion.Format(_T("%u,%u,%u,%u"), HIWORD(vsffi.dwProductVersionMS),
				LOWORD(vsffi.dwProductVersionMS),
				HIWORD(vsffi.dwProductVersionLS),
				LOWORD(vsffi.dwProductVersionLS));
		}
		return strVersion;
	}
	// Attributes
protected:
	LPBYTE  m_lpVersionData;
	DWORD   m_dwLangCharset;
	// Implementation
public:
	~CFileVersion()
	{
		Close();
	}
};
//////////////////////////////////////////////////////////////////////
#endif  // __FILEVERSION_H_