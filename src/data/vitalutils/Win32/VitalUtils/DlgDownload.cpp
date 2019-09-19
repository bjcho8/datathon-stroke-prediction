#include "stdafx.h"
#include "VitalUtils.h"
#include "DlgDownload.h"
#include "afxdialogex.h"
#include "Https.h"
#include <process.h>
#include <Tlhelp32.h>
#include <winbase.h> 
#include <string.h>

IMPLEMENT_DYNAMIC(CDlgDownload, CDialogEx)

CDlgDownload::CDlgDownload(CWnd* pParent, LPCTSTR url, LPCTSTR localpath)
: CDialogEx(CDlgDownload::IDD, pParent), m_strUrl(url), m_strLocalPath(localpath) {
	m_strStatus = m_strUrl;
	if (m_strUrl.Left(8).MakeLower() != "https://") {
		AfxMessageBox("URL is not HTTPS");
		return;
	}
}

bool WaitForEvt(HANDLE hEvent, DWORD nMilisec) {
	return WaitForSingleObject(hEvent, nMilisec) == WAIT_OBJECT_0;
}

typedef UINT(*THREADPROC)(LPVOID);
CWinThread* NewThread(THREADPROC f, LPVOID p) {
	auto pthread = AfxBeginThread(f, p, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	pthread->m_bAutoDelete = FALSE;
	pthread->ResumeThread();
	return pthread;
}

void DeleteThread(CWinThread* &pWinThread, DWORD timeout = 1000) {
	CWinThread* pThread = pWinThread;

	// �ι� ����ǵ� ���� ����
	if (!pWinThread) return;
	pWinThread = nullptr;

	auto dwThreadId = GetThreadId(pThread->m_hThread);

	// �� �����忡�� �ڽ��� wait �ϰ� �����ϴ� ���? �̷� ���� ���踦 �����ؾ��Ѵ�.
	// �����尡 �ڽ��� delete �� ��� return �� ����� ����� assert fail �� �߻��Ѵ�.
	ASSERT(GetCurrentThreadId() != dwThreadId);

	if (pThread->m_hThread) {
		//		TRACE(NumToStr(GetUt()) + "wait start %x\n", dwThreadId);
		//		auto t1 = GetUt();
		if (!WaitForEvt(pThread->m_hThread, timeout)) { // ���� ���� �� thread object�� signaled ���°� �ǹǷ� WAIT_OBJECT_0 ����
														//if (GetUt() - t1 > 0.8) {
														//	int a = 0;
														//}
														// �����带 ���� �����ϸ� �޸� ������ �����.
														//			TRACE(NumToStr(GetUt()) + " terminate thread %x\n", dwThreadId);
			TerminateThread(pThread->m_hThread, 0);
		}
	}

	delete pThread;
}

CDlgDownload::~CDlgDownload() {
	DeleteThread(m_pDownloadThread);
}

void CDlgDownload::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STATUS, m_strStatus);
	DDX_Control(pDX, IDC_PROGRESS1, m_ctrlProgress);
}

BEGIN_MESSAGE_MAP(CDlgDownload, CDialogEx)
	ON_BN_CLICKED(IDCANCEL, &CDlgDownload::OnBnClickedCancel)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CDlgDownload::OnBnClickedCancel() {
	m_bCancelling = true;

	CDialogEx::OnCancel();
}

BOOL CDlgDownload::OnInitDialog() {
	CDialogEx::OnInitDialog();
	m_ctrlProgress.SetRange(0, 100);

	ASSERT(!m_pDownloadThread);
	m_pDownloadThread = NewThread([](LPVOID p)->UINT{((CDlgDownload*)p)->DownloadThreadFunc(); return 0; }, this);

	return TRUE;
}

void CDlgDownload::OnDestroy() {
	// �ٿ�ε� ����
	DeleteThread(m_pDownloadThread);

	CDialogEx::OnDestroy();
}

void CDlgDownload::OnProgress(DWORD dwRead, DWORD dwTotal) {
	if (!dwTotal) return;

	int iperc = m_ctrlProgress.GetPos();
	int newperc = (dwRead * 100) / dwTotal;
	if (iperc == newperc) return;
	m_ctrlProgress.SetPos(newperc);
}

void CDlgDownload::DownloadThreadFunc() {
	// theApp.Log("DownloadThread started");

	CString str = m_strUrl.Mid(8);
	
	int ipos = str.Find("/");
	CString strHost = str.Left(ipos);
	CString strRemotePath = str.Mid(ipos);

	Https https(strHost);
	if (!https.Download(strRemotePath, m_strLocalPath, [this](DWORD dwRead, DWORD dwTotal)->bool{ OnProgress(dwRead, dwTotal); return !m_bCancelling; })) {
		m_bCancelling = false;
		PostMessage(WM_COMMAND, IDCANCEL);
		return;
	}

	// �ڱ� �ڽ��� ���� ��Ų��
	PostMessage(WM_COMMAND, IDOK);
	theApp.Log("Download completed");
}

