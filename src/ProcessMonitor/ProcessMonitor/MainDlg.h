// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CMainDlg : public CAxDialogImpl<CMainDlg>, public CUpdateUI<CMainDlg>,
		public CMessageFilter, public CIdleHandler
{
public:
	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		NOTIFY_HANDLER(IDC_LIST1, LVN_ITEMACTIVATE, OnLvnItemActivateList1)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void CloseDialog(int nVal);

private:
	void AddProcessToList(DWORD processID);
	void UpdateProcessList();
	void UpdateDriverList();
	void UpdateProcessTime();
	void ClearProcessList();

	struct ST_PROCESSINFO
	{
		DWORD dwProcessId;
		HANDLE hProcess;
		CPUUsage * pCpuUsage;

		ST_PROCESSINFO()
		{
			dwProcessId = 0;
			hProcess = NULL;
			pCpuUsage = new CPUUsage();
		}
		ST_PROCESSINFO(DWORD _dwProcessId, HANDLE _hProcess)
		{
			dwProcessId = _dwProcessId;
			hProcess = _hProcess;
			pCpuUsage = new CPUUsage(_dwProcessId);
		}

		~ST_PROCESSINFO()
		{
			if (pCpuUsage)
			{
				delete pCpuUsage;
				pCpuUsage = NULL;
			}
			dwProcessId = 0;

			if (hProcess)
			{
				CloseHandle(hProcess);
				hProcess = NULL;
			}
		}
	};

	bool m_bStopUpdateProcessThread;
	HANDLE m_hUpdateProcessThread;
	BOOL GetDllList(DWORD dwProcID);
	static DWORD WINAPI UpdateProcessThread(void *lpVoid);
public:
	LRESULT OnLvnItemActivateList1(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/);
};
