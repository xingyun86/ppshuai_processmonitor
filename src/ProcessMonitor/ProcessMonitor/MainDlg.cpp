// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	UIUpdateChildWindows();
	return FALSE;
}

// function to convert FILETIME to meaningful string
BOOL GetFileTimeAsString(LPFILETIME pFt, _TCHAR * pszTime, unsigned cbIn)
{
	FILETIME ftLocal;
	SYSTEMTIME st;

	if (!FileTimeToLocalFileTime(pFt, &ftLocal))
		return FALSE;

	if (!FileTimeToSystemTime(&ftLocal, &st))
		return FALSE;

	_sntprintf_s(pszTime, cbIn, cbIn, _T("%02u:%02u:%02u"), st.wHour, st.wMinute, st.wSecond);
	return TRUE;
}

// function to convert FILETIME to meaningful string
BOOL GetFileDateAsString(LPFILETIME pFt, _TCHAR * pszDate, unsigned cbIn)
{
	FILETIME ftLocal;
	SYSTEMTIME st;

	if (!FileTimeToLocalFileTime(pFt, &ftLocal))
		return FALSE;

	if (!FileTimeToSystemTime(&ftLocal, &st))
		return FALSE;
	
	_sntprintf_s(pszDate, cbIn, cbIn, _T("%04u/%02u/%02u"), st.wYear, st.wMonth, st.wDay);
	return TRUE;
}


// this will update the process start time and end time(end time, only if the process has terminated)
void CMainDlg::UpdateProcessTime()
{
	CListViewCtrl listViewCtrl = (CListViewCtrl)GetDlgItem(IDC_LIST1);
	FILETIME ftCreate, ftExit, ftKernel, ftUser;

	int nCount = listViewCtrl.GetItemCount();

	// loop all the process in the list box
	for (int i = 0; i < nCount; i++)
	{
		struct CMainDlg::ST_PROCESSINFO *pstProcessInfo = (struct CMainDlg::ST_PROCESSINFO *)listViewCtrl.GetItemData(i);
		if (!pstProcessInfo || !pstProcessInfo->hProcess)
		{
			continue;
		}
		
		if (::GetProcessTimes(pstProcessInfo->hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser))
		{
			// Horrible, disgusting hack!  The two lines below basically grab the
			// contents of a FILETIME structure and store it in a 64 bit integer.
			LONGLONG tUser64 = *(LONGLONG *)&ftUser;
			LONGLONG tKernel64 = *(LONGLONG *)&ftKernel;

			DWORD tUser, tKernel;

			// The LONGLONGs contain the time in 100 nanosecond intervals (now
			// there's a useful unit of measurement...).  Divide each of them by
			// 10000 to convert into milliseconds, and store the results in a
			// DWORD.  This means that the max time before overflowing is around
			// 4 Million seconds (about 49 days)
			tUser = (DWORD)(tUser64 / 10000);
			tKernel = (DWORD)(tKernel64 / 10000);

			// Format the user and kernel times, and add to the process node
			_TCHAR szItem[128];

			_TCHAR szFileDate[32] = { 0 };
			_TCHAR szFileTime[32] = { 0 };

			if (!ftCreate.dwHighDateTime && !ftCreate.dwLowDateTime)
			{
				_tcscpy_s(szFileDate, _T(""));
				_tcscpy_s(szFileTime, _T(""));
			}
			else
			{
				GetFileDateAsString(&ftCreate, szFileDate, sizeof(szFileDate) / sizeof(*szFileDate));
				GetFileTimeAsString(&ftCreate, szFileTime, sizeof(szFileTime) / sizeof(*szFileTime));
			}

			_stprintf_s(szItem, _T("%s %s"), szFileDate, szFileTime);
			_TCHAR tzText[MAX_PATH] = { 0 };
			listViewCtrl.GetItemText(i, 3, tzText, sizeof(tzText) / sizeof(*tzText));

			// if already exists then don't update, this will reduce the flicker
			if (_tcsicmp(tzText, szItem) != 0)
			{
				listViewCtrl.SetItemText(i, 3, szItem);
			}
			if (!ftExit.dwHighDateTime && !ftExit.dwLowDateTime)
			{
				_tcscpy_s(szFileDate, _T(""));
				_tcscpy_s(szFileTime, _T(""));
			}
			else
			{
				GetFileDateAsString(&ftExit, szFileDate, sizeof(szFileDate) / sizeof(*szFileDate));
				GetFileTimeAsString(&ftExit, szFileTime, sizeof(szFileTime) / sizeof(*szFileTime));
			}

			//wsprintf(szItem, _T("%s %s"), szFileDate, szFileTime);
			DOUBLE dCpuUsage = pstProcessInfo->pCpuUsage->GetCpuUsage();
			_stprintf_s(szItem, _T("%.6f%%"), dCpuUsage);
			memset(tzText, 0, sizeof(tzText));
			listViewCtrl.GetItemText(i, 4, tzText, sizeof(tzText) / sizeof(*tzText));

			// if already exists then don't update, this will reduce the flicker
			if (!(*tzText) || (dCpuUsage >= 0.0f && lstrcmpi(tzText, szItem) != 0))
			{
				listViewCtrl.SetItemText(i, 4, szItem);
			}
		}
	}
}
BOOL NtPathToDosPath(LPTSTR pszDosPath, LPTSTR pszNtPath)
{
	_TCHAR tzDevName[MAX_PATH] = { 0 };
	_TCHAR tzDrive[4] = { 0 };
	//检查参数
	if (!pszDosPath || !pszNtPath)
	{
		return FALSE;
	}

	if ((*pszNtPath) >= _T('C') && (*pszNtPath) <= _T('Z'))
	{
		memset(tzDrive, 0, sizeof(tzDrive));
		_tcsncpy(tzDrive, pszNtPath, sizeof(WORD));
		if (!QueryDosDevice(tzDrive, tzDevName, MAX_PATH))//查询设备名，这里是重点
		{
			return FALSE;
		}
		_stprintf_s(pszDosPath, MAX_PATH, _T("%s%s"), tzDevName, pszNtPath + sizeof(WORD));
		return TRUE;
	}
	_stprintf_s(pszDosPath, MAX_PATH, _T("%s"), pszNtPath);
	return FALSE;
}
BOOL DosPathToNtPath(LPTSTR pszNtPath, LPTSTR pszDosPath)
{
	_TCHAR	tzDriveStrings[MAX_PATH] = { 0 };
	_TCHAR	tzDevName[MAX_PATH] = { 0 };
	_TCHAR	tzDrive[4] = { 0 };

	//检查参数
	if (!pszDosPath || !pszNtPath)
	{
		return FALSE;
	}

	//获取本地磁盘字符串
	if (GetLogicalDriveStrings(sizeof(tzDriveStrings)/sizeof(*tzDriveStrings), tzDriveStrings))
	{
		for (auto nIndex = 0; tzDriveStrings[nIndex]; nIndex += sizeof(tzDrive) / sizeof(*tzDrive))
		{
			if (tzDriveStrings[nIndex] >= _T('C') && tzDriveStrings[nIndex] <= _T('Z'))
			{
				memset(tzDrive, 0, sizeof(tzDrive));
				_tcsncpy(tzDrive, &tzDriveStrings[nIndex], sizeof(WORD));
				if (!QueryDosDevice(tzDrive, tzDevName, MAX_PATH))//查询 Dos 设备名
				{
					return FALSE;
				}
				if (!_tcsnicmp(pszDosPath, tzDevName, _tcslen(tzDevName)))//命中
				{
					_stprintf_s(pszNtPath, MAX_PATH, _T("%s%s"), tzDrive, pszDosPath + _tcslen(tzDevName));
					return TRUE;
				}
			}			
		}
	}

	_stprintf_s(pszNtPath, MAX_PATH, _T("%s"), pszDosPath);
	
	return FALSE;
}
void CMainDlg::AddProcessToList(DWORD processID)
{
	//
	// Adds the process name and ID to the ListCtrl
	//

	// first update the process time 
	UpdateProcessTime();
		
	CListViewCtrl listViewCtrl = (CListViewCtrl)GetDlgItem(IDC_LIST1);
	_TCHAR szBuff[MAX_PATH] = { 0 };
	int nCount = listViewCtrl.GetItemCount();
	_TCHAR szItemString[MAX_PATH + 64] = { 0 };
	_TCHAR szProcessName[MAX_PATH] = _T("Unknown");
	struct CMainDlg::ST_PROCESSINFO *pstProcessInfo = NULL;

	// open the process to query the time information
	//   this handle will remain open untill ClearProcessList call
	//   This should remain open to get the process terminated time 
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	if (!hProcess)
	{
		for (int i = 0; i < nCount; i++)
		{
			struct CMainDlg::ST_PROCESSINFO *pSTP = (struct CMainDlg::ST_PROCESSINFO *)listViewCtrl.GetItemData(i);

			// If the passed process id is same as the already updated process in the ListCtrl
			//    then check whether it is a terminated process
			//       if not then return immediately without updating (to avoid flicker)
			if (pSTP && pSTP->dwProcessId == processID)
			{
				delete pSTP;
				pSTP = NULL;
				listViewCtrl.DeleteItem(i);
				break;
			}
		}
		return;
	}
	for (int i = 0; i < nCount; i++)
	{
		struct CMainDlg::ST_PROCESSINFO *pSTP = (struct CMainDlg::ST_PROCESSINFO *)listViewCtrl.GetItemData(i);

		// If the passed process id is same as the already updated process in the ListCtrl
		//    then check whether it is a terminated process
		//       if not then return immediately without updating (to avoid flicker)
		if (pSTP && pSTP->dwProcessId == processID)
		{
			return;
		}
	}

	HMODULE hMod = NULL;
	DWORD cbNeeded = 0L;

	if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
	{
		_TCHAR tzDosPathName[MAX_PATH] = { 0 };
		GetProcessImageFileName(hProcess, tzDosPathName, sizeof(tzDosPathName) / sizeof(*tzDosPathName));
		DosPathToNtPath(szProcessName, tzDosPathName);
	}
	_stprintf_s(szItemString, _T("%u"), processID);
	_stprintf_s(szBuff, _T("%d"), nCount);

	// fill the structure and store the info for later updates
	pstProcessInfo = new struct CMainDlg::ST_PROCESSINFO(processID, hProcess);

	listViewCtrl.InsertItem(nCount, szItemString);
	listViewCtrl.SetItemText(nCount, 2, szProcessName);
	listViewCtrl.SetItemText(nCount, 1, szBuff);

	listViewCtrl.SetItemData(nCount, (DWORD_PTR)pstProcessInfo);
}

// enumerates all the process running in the system
void CMainDlg::UpdateProcessList()
{
	// Get the list of process IDs
	DWORD aProcesses[MAXWORD] = { 0L }, cbNeeded = 0L;
	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	{
		return;
	}
	// Calculate how many process IDs were returned
	DWORD cProcesses = cbNeeded / sizeof(DWORD);

	// Spit out the information for each ID
	for (DWORD i = 0; i < cProcesses; i++)
	{
		AddProcessToList(aProcesses[i]);
	}
}

// clears the all the process and closes the handles
void CMainDlg::ClearProcessList()
{
	CListViewCtrl listViewCtrl = (CListViewCtrl)GetDlgItem(IDC_LIST1);

	int nCount = listViewCtrl.GetItemCount();
	struct CMainDlg::ST_PROCESSINFO *pstProcessInfo = NULL;

	for (int i = nCount - 1; i >= 0; i--)
	{
		pstProcessInfo = (struct CMainDlg::ST_PROCESSINFO *)listViewCtrl.GetItemData(i);
		if (pstProcessInfo)
		{
			delete pstProcessInfo;
		}
		
		listViewCtrl.DeleteItem(i);
	}
}

// fill the ListCtrl with drivers loaded 
void CMainDlg::UpdateDriverList()
{
	CListViewCtrl listViewCtrl = (CListViewCtrl)GetDlgItem(IDC_LIST2);

	// Get the list of device driver base addresses
	PVOID aDrivers[1024];
	DWORD cbNeeded;
	if (!EnumDeviceDrivers(aDrivers, sizeof(aDrivers), &cbNeeded))
		return;

	// Calculate how many drivers were returned
	DWORD cDrivers = cbNeeded / sizeof(*aDrivers);

	// Spit out the information for each driver
	for (unsigned i = 0; i < cDrivers; i++)
	{
		_TCHAR szBaseName[MAX_PATH] = _T("");
		_TCHAR szDriverFileName[MAX_PATH] = _T("");

		// Get the driver's base name
		if (GetDeviceDriverBaseName(aDrivers[i], szBaseName, sizeof(szBaseName)))
		{
			_TCHAR szItem[MAX_PATH + 64];
			_TCHAR szBuff[MAX_PATH];

			wsprintf(szItem, _T("0x%08X"), aDrivers[i]);
			wsprintf(szBuff, _T("%d"), i);

			// Get the full path to the driver
			GetDeviceDriverFileName(aDrivers[i], szDriverFileName, sizeof(szDriverFileName));

			int nCount = listViewCtrl.GetItemCount();

			for (int i = 0; i<nCount; i++)
			{
				_TCHAR tzText[MAX_PATH] = { 0 };
				listViewCtrl.GetItemText(i, 0, tzText, sizeof(tzText) / sizeof(*tzText));
				if (lstrcmpi(tzText, szItem) == 0)
					goto endloop;
			}

			listViewCtrl.InsertItem(i, szItem);
			listViewCtrl.SetItemText(i, 1, szBuff);
			listViewCtrl.SetItemText(i, 2, szBaseName);
			listViewCtrl.SetItemText(i, 3, szDriverFileName);

			listViewCtrl.SetItemData(i, (DWORD_PTR)aDrivers[i]);
		endloop:;
		}
	}
}

// thread calls UpdateProcessList & UpdateDriverList to update the process time
DWORD WINAPI CMainDlg::UpdateProcessThread(void *lpVoid)
{
	CMainDlg *pDlg = (CMainDlg *)lpVoid;

	while (!pDlg->m_bStopUpdateProcessThread)
	{
		pDlg->UpdateProcessList();
		//pDlg->UpdateDriverList();
		Sleep(3000L);
	}
	return 0;
}
///////////////自定义函数实现/////////////////

BOOL PromotePrivilege(BOOL bEnable)
{
	// 附给本进程特权,以便访问系统进程
	HANDLE hToken;

	// 打开一个进程的访问令牌
	if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		// 取得特权名称为"SetDebugPrivilege"的LUID
		LUID uID = { 0 };
		if (!::LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &uID))
		{
			return FALSE;
		}


		// 调整特权级别
		TOKEN_PRIVILEGES tp = { 0 };
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = uID;
		tp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
		if (!::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL))
		{
			return FALSE;
		}

		// 关闭访问令牌句柄
		::CloseHandle(hToken);
		return TRUE;
	}
	return FALSE;
}
LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	{
		PromotePrivilege(TRUE);

		CListViewCtrl listViewCtrl1 = (CListViewCtrl)GetDlgItem(IDC_LIST1);
		CListViewCtrl listViewCtrl2 = (CListViewCtrl)GetDlgItem(IDC_LIST2);

		// user mode process ListCtrl initialization
		listViewCtrl1.InsertColumn(0, _T("ID"), LVCFMT_RIGHT, 40, 0);
		listViewCtrl1.InsertColumn(1, _T("#"), LVCFMT_RIGHT, 30, 1);
		listViewCtrl1.InsertColumn(2, _T("Name"), LVCFMT_LEFT, 450, 2);
		listViewCtrl1.InsertColumn(3, _T("Start"), LVCFMT_RIGHT, 120, 3);
		listViewCtrl1.InsertColumn(4, _T("Cpu"), LVCFMT_RIGHT, 120, 4);

		// kernel mode process ListCtrl initialization
		listViewCtrl2.InsertColumn(1, _T("Desc"), LVCFMT_LEFT, 160, 1);
		listViewCtrl2.InsertColumn(0, _T("ModName"), LVCFMT_LEFT, 350, 0);
		listViewCtrl2.InsertColumn(2, _T("Company"), LVCFMT_LEFT, 100, 2);
		listViewCtrl2.InsertColumn(3, _T("Version"), LVCFMT_LEFT, 100, 3);

		int nArr[] = { 1,0,2,3,4 };

		listViewCtrl1.SetColumnOrderArray(5, nArr);
		listViewCtrl2.SetColumnOrderArray(4, nArr);

		// create a thread to update the process consecutively
		m_bStopUpdateProcessThread = false;
		m_hUpdateProcessThread = CreateThread(NULL, 0, UpdateProcessThread, this, 0, NULL);
	}
	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	// if UI is the last thread, no need to wait
	if(_Module.GetLockCount() == 1)
	{
		_Module.m_dwTimeOut = 0L;
		_Module.m_dwPause = 0L;
	}
	_Module.Unlock();

	return 0;
}

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}
BOOL CMainDlg::GetDllList(DWORD dwProcID)
{
	BOOL bResult = FALSE;
	//OpenProcess，根据ID打开进程ID
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcID);

	if (hProcess != NULL)
	{
		CListViewCtrl listViewCtrl2 = (CListViewCtrl)GetDlgItem(IDC_LIST2);
		HMODULE hMod[MAXWORD] = { NULL };
		DWORD cbNeeded = 0L;
		TCHAR szModName[MAX_PATH] = { 0 };
		TCHAR lPath[MAX_PATH] = _T("");
		// 获取进程模块
		if (EnumProcessModules(hProcess, hMod, sizeof(hMod), &cbNeeded))
		{
			//获得模块路径
			for (DWORD i = 0; i <= (cbNeeded / sizeof(HMODULE)); ++i)
			{
				if (GetModuleFileNameEx(hProcess, hMod[i], szModName, sizeof(szModName)))
				{
					//这个获得模块的相应信息类将在下面提到。
					CFileVersion fileVer; 
					CString ModName = _T(""), FTime=_T("");
					//CFileStatus status;
					ModName = szModName;
					//特殊处理
					if (ModName.Find(_T("\\??\\")) != -1)
					{
						ModName.Replace(_T("\\??\\"), _T(""));
					}
					if (ModName.Find(_T("\\SystemRoot")) != -1)
					{
						ModName.Replace(_T("\\SystemRoot"), _T("C:\\WINDOWS"));
					}

					listViewCtrl2.InsertItem(i, ModName); //0.模块路径
					
					if (fileVer.Open(ModName))
					{
						CString str = fileVer.GetFileDescription();
						listViewCtrl2.SetItemText(i, 1, str);  //1.文件描述
						str = fileVer.GetCompanyName();
						listViewCtrl2.SetItemText(i, 2, str); //2. 公司名称
						str = fileVer.GetProductVersion();
						listViewCtrl2.SetItemText(i, 3, str);//3.产品版本
						fileVer.Close();
					}
					else
					{
						listViewCtrl2.SetItemText(i, 1, _T(""));  //1.文件描述
						listViewCtrl2.SetItemText(i, 2, _T("")); //2. 公司名称
						listViewCtrl2.SetItemText(i, 3, _T(""));//3.产品版本
					}
					listViewCtrl2.SetItemData(i, i); //准备排序
				}
			}
			//更新listCtrl内容
			listViewCtrl2.SetRedraw(TRUE);
			listViewCtrl2.Invalidate();
			listViewCtrl2.UpdateWindow();
			int iCnt = listViewCtrl2.GetItemCount();
			bResult = (iCnt > 0 ? iCnt - 1 : FALSE);
		}
		CloseHandle(hProcess);
	}
	return bResult;
}
LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add validation code 
	//CloseDialog(wID);

	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	m_bStopUpdateProcessThread = true;
	//ClearProcessList();
	DestroyWindow();
	::PostQuitMessage(nVal);
}

LRESULT CMainDlg::OnLvnItemActivateList1(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here
	if (pNMIA->iItem != (-1L))
	{
		CListViewCtrl listViewCtrl = (CListViewCtrl)GetDlgItem(pNMIA->hdr.idFrom);
		struct CMainDlg::ST_PROCESSINFO *pSTP = (struct CMainDlg::ST_PROCESSINFO *)listViewCtrl.GetItemData(pNMIA->iItem);

		// If the passed process id is same as the already updated process in the ListCtrl
		//    then check whether it is a terminated process
		//       if not then return immediately without updating (to avoid flicker)
		if (pSTP && pSTP->dwProcessId)
		{
			GetDllList(pSTP->dwProcessId);
		}
	}
	return 0;
}
