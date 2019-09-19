#include "stdafx.h"
#include "VitalUtils.h"
#include "VitalUtilsDlg.h"
#include <time.h>
#include <thread>
#include "dlgdownload.h"
#pragma comment(lib, "Version.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CVitalUtilsApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CVitalUtilsApp::CVitalUtilsApp() {
	// �ٽ� ���� ������ ����
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: ���⿡ ���� �ڵ带 �߰��մϴ�.
	// InitInstance�� ��� �߿��� �ʱ�ȭ �۾��� ��ġ�մϴ�.
}

CVitalUtilsApp theApp;

// create dir recursively
void CreateDir(CString path) {
	for (unsigned int pos = 0; pos != -1; pos = path.Find('\\', pos + 1)) {
		if (!pos) continue;
		auto subdir = path.Left(pos);
		HRESULT hres = CreateDirectory(subdir, NULL);
	}
}

CString GetModulePath() {
	TCHAR temp_path[MAX_PATH];
	GetModuleFileName(AfxGetInstanceHandle(), temp_path, sizeof(temp_path));
	return temp_path;
}

CString GetModuleDir() {
	return DirName(GetModulePath());
}

bool executeCommandLine(CString cmdLine) {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	STARTUPINFO si = {};
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi = {};

	if (CreateProcess(NULL, (LPTSTR)(LPCTSTR)cmdLine, &sa, &sa, FALSE, 
					  CREATE_NO_WINDOW | CREATE_DEFAULT_ERROR_MODE,
					  NULL, NULL, &si, &pi) == FALSE) return false;

	// Wait until application has terminated
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles
	::CloseHandle(pi.hThread);
	::CloseHandle(pi.hProcess);

	return true;
}

CString GetLastErrorString() {
	//Get the error message, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return CString(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
								 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	CString message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}

// output�� error�� ��� ����
CString ExecCmdGetErr(CString cmd) {
	CString strResult;
	HANDLE hPipeRead, hPipeWrite;

	SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES) };
	saAttr.bInheritHandle = TRUE;   //Pipe handles are inherited by child process.
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe to get results from child's stdout.
	if (!CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0))
		return strResult;

	STARTUPINFO si = { sizeof(STARTUPINFO) };
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.hStdOutput = hPipeWrite;
	si.hStdError = hPipeWrite;
	si.wShowWindow = SW_HIDE;       // Prevents cmd window from flashing. Requires STARTF_USESHOWWINDOW in dwFlags.

	PROCESS_INFORMATION pi = { 0 };

	BOOL fSuccess = CreateProcess(NULL, (LPSTR)(LPCTSTR)cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
	if (!fSuccess) {
		theApp.Log(cmd + " " + GetLastErrorString());
		CloseHandle(hPipeWrite);
		CloseHandle(hPipeRead);
		return strResult;
	}

	bool bProcessEnded = false;
	for (; !bProcessEnded;) {
		// Give some timeslice (50ms), so we won't waste 100% cpu.
		bProcessEnded = WaitForSingleObject(pi.hProcess, 50) == WAIT_OBJECT_0;

		// Even if process exited - we continue reading, if there is some data available over pipe.
		for (;;) {
			char buf[1024];
			DWORD dwRead = 0;
			DWORD dwAvail = 0;
			if (!::PeekNamedPipe(hPipeRead, NULL, 0, NULL, &dwAvail, NULL))
				break;

			if (!dwAvail) // no data available, return
				break;

			if (!::ReadFile(hPipeRead, buf, min(sizeof(buf) - 1, dwAvail), &dwRead, NULL) || !dwRead)
				// error, the child process might ended
				break;

			buf[dwRead] = 0;
			strResult += buf;
		}
	} //for

	CloseHandle(hPipeWrite);
	CloseHandle(hPipeRead);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return strResult;
}

CString ExecCmdGetOutput(CString cmd) {
	CString strResult;
	HANDLE hPipeRead, hPipeWrite;

	SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES) };
	saAttr.bInheritHandle = TRUE;   //Pipe handles are inherited by child process.
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe to get results from child's stdout.
	if (!CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0))
		return strResult;

	STARTUPINFO si = { sizeof(STARTUPINFO) };
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.hStdOutput = hPipeWrite;
	si.hStdError = NULL;
	si.wShowWindow = SW_HIDE;       // Prevents cmd window from flashing. Requires STARTF_USESHOWWINDOW in dwFlags.

	PROCESS_INFORMATION pi = { 0 };

	BOOL fSuccess = CreateProcess(NULL, (LPSTR)(LPCTSTR)cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
	if (!fSuccess) {
		theApp.Log(cmd + " " + GetLastErrorString());
		CloseHandle(hPipeWrite);
		CloseHandle(hPipeRead);
		return strResult;
	}

	bool bProcessEnded = false;
	for (; !bProcessEnded;) {
		// Give some timeslice (50ms), so we won't waste 100% cpu.
		bProcessEnded = WaitForSingleObject(pi.hProcess, 50) == WAIT_OBJECT_0;

		// Even if process exited - we continue reading, if there is some data available over pipe.
		for (;;) {
			char buf[1024];
			DWORD dwRead = 0;
			DWORD dwAvail = 0;
			if (!::PeekNamedPipe(hPipeRead, NULL, 0, NULL, &dwAvail, NULL))
				break;

			if (!dwAvail) // no data available, return
				break;

			if (!::ReadFile(hPipeRead, buf, min(sizeof(buf) - 1, dwAvail), &dwRead, NULL) || !dwRead)
				// error, the child process might ended
				break;

			buf[dwRead] = 0;
			strResult += buf;
		}
	} //for

	CloseHandle(hPipeWrite);
	CloseHandle(hPipeRead);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return strResult;
}

// pipe �� ������ �ϰ� ��� ����� ofile �� ������
bool executeCommandLine(CString cmdLine, CString ofile) {
	auto jobid = GetThreadId(GetCurrentThread());

	// ���� �ڵ� https://stackoverflow.com/questions/7018228/how-do-i-redirect-output-to-a-file-with-createprocess
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	// �������� ����. // temp file �� ����.
	bool ret = false;
	HANDLE fo = CreateFile(ofile, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fo == INVALID_HANDLE_VALUE) {
		CString str;
		str.Format(_T("[%d] cannot open file"), jobid);
		theApp.Log(str);
		return false;
	}

	// ������ ����� �������� ����
	// https://msdn.microsoft.com/ko-kr/library/windows/desktop/ms682499(v=vs.85).aspx
	HANDLE stder, stdew;
	if (!CreatePipe(&stder, &stdew, &sa, 0)) {
		theApp.Log(_T("cannot create pipe"));
		return false;
	}
	if (!SetHandleInformation(stder, HANDLE_FLAG_INHERIT, 0)) {
		theApp.Log(_T("set handle information"));
		return false;
	}

	STARTUPINFO si = {};
	si.cb = sizeof(si);
	si.wShowWindow = SW_HIDE;
	si.hStdOutput = fo;
	si.hStdError = stdew;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	PROCESS_INFORMATION pi = {};

	if (!CreateProcess(NULL, (LPTSTR)(LPCTSTR)cmdLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		theApp.Log(_T("createprocess error ") + GetLastErrorString());
		goto clean;
	}
	
	char buf[4096];
	while (1) {
		if (WAIT_TIMEOUT != WaitForSingleObject(pi.hProcess, 500)) break;
		for (DWORD dwAvail = 0; PeekNamedPipe(stder, 0, 0, 0, &dwAvail, 0) && dwAvail; dwAvail = 0) {
			DWORD dwRead = 0;
			ReadFile(stder, buf, min(4095, dwAvail), &dwRead, 0);
			CString str(buf, dwRead);
			theApp.Log(str);
		}
	}
	// ���μ��� ����
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	ret = true;

clean:
	if (fo) CloseHandle(fo);

	if (stder) CloseHandle(stder);
	if (stdew) CloseHandle(stdew); // �������ٰ� �츮�� �� �� ����

	return ret;
}

CString ExtName(CString path) {
	int pos = path.ReverseFind('.');
	if (pos == -1) return _T("");
	return path.Mid(pos + 1).MakeLower();
}

CString DirName(CString path) {
	return path.Left(max(path.ReverseFind('/'), path.ReverseFind('\\')) + 1);
}

void ListFiles(LPCTSTR path, vector<CString>& files, CString ext) {
	int next = ext.GetLength();

	CString dirName = DirName(path);

	BOOL bSearchFinished = FALSE;
	WIN32_FIND_DATA fd = { 0, };

	HANDLE hFind = FindFirstFile(dirName + _T("*.*"), &fd);
	for (BOOL ret = (hFind != INVALID_HANDLE_VALUE); ret; ret = FindNextFile(hFind, &fd)) {
		CString filename = fd.cFileName;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { // directory
			if (filename.Left(1) == _T(".")) continue;
			ListFiles(dirName + filename, files, ext);
		} else if (filename.Right(next).MakeLower() == ext) {
			files.push_back(dirName + fd.cFileName);
		}
	}
	FindClose(hFind);
}

CString BaseName(CString path) {
	return path.Mid(max(path.ReverseFind('/'), path.ReverseFind('\\')) + 1);
}

DWORD FileTimeToUnixTime(const FILETIME &ft) {
	ULARGE_INTEGER ull;
	ull.LowPart = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;
	return (DWORD)(ull.QuadPart / 10000000ULL - 11644473600ULL);
}

UINT WINAPI WorkThread(void* p) {
	while (1) {
		if (theApp.m_nJob == theApp.JOB_SCANNING) {
			CString dirname;
			if (theApp.m_scans.Pop(dirname)) { // ��ĳ�� ����
				theApp.m_nrunning.Inc();

				theApp.LoadCache(dirname);

				WIN32_FIND_DATA fd;
				ZeroMemory(&fd, sizeof(WIN32_FIND_DATA));

				HANDLE hFind = FindFirstFile(dirname + "*.*", &fd);
				for (BOOL ret = (hFind != INVALID_HANDLE_VALUE); ret; ret = FindNextFile(hFind, &fd)) {
					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { // directory
						if (strcmp(fd.cFileName, ".") == 0) continue;
						if (strcmp(fd.cFileName, "..") == 0) continue;

						theApp.Log("Scanning " + dirname + fd.cFileName);
						theApp.m_scans.Push(dirname + fd.cFileName + "\\");
						theApp.m_ntotal++;
					} else { // vital ���� ����
						if (ExtName(fd.cFileName) != "vital") continue;

						auto p = new VITAL_FILE_INFO();
						p->path = dirname + fd.cFileName;
						p->filename = fd.cFileName;
						p->dirname = dirname; // �����ִ� �������θ� ����Ѵ�.
						p->mtime = FileTimeToUnixTime(fd.ftLastWriteTime);
						p->size = fd.nFileSizeLow + fd.nFileSizeHigh * ((ULONGLONG)MAXDWORD + 1);

						EnterCriticalSection(&theApp.m_csFile);
						theApp.m_files.push_back(p);
						LeaveCriticalSection(&theApp.m_csFile);

						// ĳ���� ���� ���� ����� m_parses �� ����
						bool hascache = false;
						auto it = theApp.m_path_trklist.find(p->path);
						if (it != theApp.m_path_trklist.end()) { // ��ΰ� �����ϰ�
							if (it->second.first == p->mtime) {// �ð��� ������
								hascache = true;
							} 
						}

						if (!hascache) { // ĳ���� ����. ��¡ ��Ͽ� �߰�
							TRACE("no cache: " + p->path + '\n');
							theApp.m_parses.Push(make_pair(p->mtime, p->path));
							theApp.m_ntotal++;
							// ��¡�� ������ �� �����忡�� m_path_trklist �� �Ű� �ְ� ĳ���� ������
						}
					}
				}
				FindClose(hFind);

				theApp.m_nrunning.Dec();
			} else {
				Sleep(100);
			}
		} else if (theApp.m_nJob == theApp.JOB_PARSING) {
			static time_t tlast = 0;
			time_t tnow = time(nullptr);
			if (!theApp.m_cache_updated.empty() && tlast + 30 < tnow) { // ���������� ������ ĳ�� ������ �ְ�, 30�� ��������
				tlast = tnow;
				theApp.SaveCaches();
			}

			DWORD_CString dstr;
			if (theApp.m_parses.Pop(dstr)) { // ��¡ �ؾ��� ������ �ִٸ�
				theApp.m_nrunning.Inc();

				auto mtime = dstr.first;
				auto path = dstr.second;
				auto dirname = DirName(path);
				auto filename = BaseName(path);
				auto cmd = "\"" + GetModuleDir() + "utilities\\vital_trks.exe\" -s \"" + path + "\"";
				auto res = ExecCmdGetOutput(cmd); // �̰� ���� �ɸ�

				// ��¡�� �Ϸ��
				EnterCriticalSection(&theApp.m_csCache);
				theApp.m_path_trklist[path] = make_pair(mtime, res); // m_path_trklist�� �����͸� �߰�
				theApp.m_cache_updated.insert(dirname); // ���ο� ��¡�� �Ϸ�Ǿ����Ƿ� ĳ���� ������Ʈ �ؾ���
				LeaveCriticalSection(&theApp.m_csCache);

				theApp.m_nrunning.Dec();
			} else {
				Sleep(100);
			}
		} else if (theApp.m_nJob == theApp.JOB_RUNNING) { // �����ؾ��� ������ �ִٸ�
			CString cmd;
			if (theApp.m_jobs.Pop(cmd)) {
				theApp.m_nrunning.Inc();

				auto jobid = GetThreadId(GetCurrentThread());

				CString str;
				str.Format(_T("[%d] %s"), jobid, cmd);
				theApp.Log(str);

				time_t tstart = time(nullptr);

				// ���� ���α׷��� ����
				int dred = cmd.Find(">>");
				int red = cmd.Find('>');
				if (dred >= 1) { // Longitudinal file
					CString ofile = cmd.Mid(dred + 2);
					ofile = ofile.Trim(_T("\t \""));
					cmd = cmd.Left(dred);
					auto s = ExecCmdGetOutput(cmd);

					// lock and save the result
					EnterCriticalSection(&theApp.m_csLong);
					FILE* fa = fopen(ofile, "ab"); // append file
					s.TrimRight("\r\n");
					fwrite(s, 1, s.GetLength(), fa);
					if (!s.IsEmpty()) fprintf(fa, "\r\n");
					fclose(fa);
					LeaveCriticalSection(&theApp.m_csLong);
				} else if (red >= 1) { // ������ ����
					CString ofile = cmd.Mid(red + 1);
					ofile = ofile.Trim(_T("\t \""));
					cmd = cmd.Left(red);
					executeCommandLine(cmd, ofile);
				} else { // �ܼ� ����
					auto s = ExecCmdGetErr(cmd);
					theApp.Log(s);
				}

				str.Format(_T("[%d] finished in %d sec"), jobid, (int)difftime(time(nullptr), tstart));
				theApp.Log(str);

				theApp.m_nrunning.Dec();
			} else {
				Sleep(100);
			}
		} else {
			Sleep(100);
		}
	}
	return 0;
}

CString GetProductVer() {
	auto temp_path = GetModulePath();

	// ���������� ���������� �����׸��� �߰��� �����ɼ� �ֱ� ������ ũ�Ⱑ �������� �ʽ��ϴ�.���� ����
	// ���������� ũ�⸦ �����Ŀ� �� ũ�⸸ŭ �޸𸮸� �Ҵ��ϰ� ���������� �ش� �޸𸮿� �����ؾ� �մϴ�.
	// ���� ������ ��� ���� ����� �ڵ鰪�� �����ϴ� �����̴�.
	DWORD h_version_handle;
	// ���������� �׸��� ����ڰ� �߰�/���� �Ҽ� �ֱ� ������ ������ ũ�Ⱑ �ƴϴ�.
	// ���� ���� ���α׷��� ���������� ���� ũ�⸦ �� �� ũ�⿡ �´� �޸𸮸� �Ҵ��ϰ� �۾��ؾ��Ѵ�.
	DWORD version_info_size = GetFileVersionInfoSize(temp_path, &h_version_handle);

	// ���������� �����ϱ� ���� �ý��� �޸𸮸� �����Ѵ�. ( �ڵ� �������� ���� )
	HANDLE h_memory = GlobalAlloc(GMEM_MOVEABLE, version_info_size);
	// �ڵ� ������ �޸𸮸� ����ϱ� ���ؼ� �ش� �ڵ鿡 �����Ҽ� �ִ� �ּҸ� ��´�.
	LPVOID p_info_memory = GlobalLock(h_memory);

	// ���� ���α׷��� ���� ������ �����´�.
	GetFileVersionInfo(temp_path, h_version_handle, version_info_size, p_info_memory);

	// ���� ������ ���Ե� �� �׸� ���� ��ġ�� ������ �����̴�. �� �����Ϳ� ���޵� �ּҴ�
	// p_info_memory �� ���� ��ġ�̱� ������ �����ϸ� �ȵ˴ϴ�.
	// ( p_info_memory �� �����ϴ� ������ ������ �Դϴ�. )
	char *p_data = NULL;

	// ������ ���� ������ ũ�⸦ ������ �����̴�.
	UINT data_size = 0;

	// �����׸� ��ÿ� ���� 041204b0 �� ����ڵ��̰� "Korean"�� �ǹ��մϴ�.
	// ���������� ���Ե� Comments ������ �� ����մϴ�.
	//VerQueryValue(p_info_memory, "\\StringFileInfo\\041204b0\\Comments", (void **)&p_data, &data_size);

	// ���������� ���Ե� CompanyName ������ �� ����Ѵ�.
	//VerQueryValue(p_info_memory, "\\StringFileInfo\\041204b0\\CompanyName", (void **)&p_data, &data_size);

	// ���������� ���Ե� FileDescription ������ �� ����Ѵ�.
	//VerQueryValue(p_info_memory, "\\StringFileInfo\\041204b0\\FileDescription", (void **)&p_data, &data_size);

	// ���������� ���Ե� FileVersion ������ �� ����Ѵ�.
	VerQueryValue(p_info_memory, "\\StringFileInfo\\000904b0\\ProductVersion", (void **)&p_data, &data_size);

	// ���� ������ �����ϱ� ���� ����ߴ� �޸𸮸� �����Ѵ�.
	GlobalUnlock(h_memory);
	GlobalFree(h_memory);

	return p_data;
}

BOOL CVitalUtilsApp::InitInstance() {
	// ���� ���α׷� �Ŵ��佺Ʈ�� ComCtl32.dll ���� 6 �̻��� ����Ͽ� ���־� ��Ÿ����
	// ����ϵ��� �����ϴ� ���, Windows XP �󿡼� �ݵ�� InitCommonControlsEx()�� �ʿ��մϴ�.
	// InitCommonControlsEx()�� ������� ������ â�� ���� �� �����ϴ�.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ���� ���α׷����� ����� ��� ���� ��Ʈ�� Ŭ������ �����ϵ���
	// �� �׸��� �����Ͻʽÿ�.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if (!FileExists(GetModuleDir() + "utilities\\vital_trks.exe") || (!FileExists(GetModuleDir() + "utilities\\vital_recs.exe") 
																	  && !FileExists(GetModuleDir() + "utilities\\vital_recs_x64.exe"))) {
		AfxMessageBox(_T("vital_recs.exe and vital_trks.exe should be exist in utilities folder"));
		return FALSE;
	}

	// initialize winsock
	WSADATA wd;
	if (WSAStartup(MAKEWORD(2, 2), &wd) != 0) return FALSE;

	AfxOleInit();
	AfxEnableControlContainer(); //���� ������Ʈ���� ActiveX��Ʈ���� ����� �� �ְ� ����
	AfxInitRichEdit();

	// ��ȭ ���ڿ� �� Ʈ�� �� �Ǵ�
	// �� ��� �� ��Ʈ���� ���ԵǾ� �ִ� ��� �� �����ڸ� ����ϴ�.
	CShellManager *pShellManager = new CShellManager;

	// MFC ��Ʈ���� �׸��� ����ϱ� ���� "Windows ����" ���־� ������ Ȱ��ȭ
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	SetRegistryKey(_T("VitalUtils"));

	InitializeCriticalSection(&m_csFile);
	InitializeCriticalSection(&m_csTrk);
	InitializeCriticalSection(&m_csCache);
	InitializeCriticalSection(&m_csLong);

	// begin worker thread pool
	auto ncores = (int)thread::hardware_concurrency();
	for (int i = 0; i < ncores; ++i)
		_beginthreadex(0, 0, WorkThread, 0, 0, 0);

	CVitalUtilsDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK) {
		// TODO: ���⿡ [Ȯ��]�� Ŭ���Ͽ� ��ȭ ���ڰ� ������ �� ó����
		//  �ڵ带 ��ġ�մϴ�.
	} else if (nResponse == IDCANCEL) {
		// TODO: ���⿡ [���]�� Ŭ���Ͽ� ��ȭ ���ڰ� ������ �� ó����
		//  �ڵ带 ��ġ�մϴ�.
	} else if (nResponse == -1) {
	}

	// ������ ���� �� �����ڸ� �����մϴ�.
	if (pShellManager != NULL) {
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// ��ȭ ���ڰ� �������Ƿ� ���� ���α׷��� �޽��� ������ �������� �ʰ�  ���� ���α׷��� ���� �� �ֵ��� FALSE��
	// ��ȯ�մϴ�.
	return FALSE;
}

void CVitalUtilsApp::Log(CString msg) {
	// ���뱸 ����
	auto trivial = GetModuleDir();
	while (msg.Find(trivial) >= 0) msg.Replace(trivial, "");

	if (!msg.IsEmpty()) m_msgs.Push(make_pair((DWORD)time(nullptr), msg));
}

int CVitalUtilsApp::ExitInstance() {
	EnterCriticalSection(&m_csFile);
	
	auto copy = m_files;
	m_files.clear();
	LeaveCriticalSection(&m_csFile);
	for (auto& p : copy)
		delete p;

	DeleteCriticalSection(&m_csTrk);
	DeleteCriticalSection(&m_csCache);
	DeleteCriticalSection(&m_csFile);
	DeleteCriticalSection(&m_csLong);

	return CWinApp::ExitInstance();
}

vector<CString> Explode(CString str, TCHAR sep) {
	vector<CString> ret;
	if (str.IsEmpty()) return ret;
	if (str[str.GetLength() - 1] != sep) str += sep;
	for (int i = 0, j = 0; (j = str.Find(sep, i)) >= 0; i = j + 1) {
		CString tmp = str.Mid(i, j - i);
		tmp.TrimLeft(sep); tmp.TrimRight(sep);
		ret.push_back(tmp);
	}
	if (ret.empty()) {
		str.TrimLeft(sep); str.TrimRight(sep);
		ret.push_back(str);
	}
	return ret;
}

void CVitalUtilsApp::InstallPython() {
	char tmpdir[MAX_PATH];
	GetTempPath(MAX_PATH, tmpdir);

	CString strSetupUrl = "https:/""/vitaldb.net/python_setup.exe";
	CString tmppath; tmppath.Format("%spysetup_%u.exe", tmpdir, time(nullptr));
	CDlgDownload dlg(nullptr, strSetupUrl, tmppath);
	if (IDOK != dlg.DoModal()) {
		AfxMessageBox("Cannot download python setup file.\nPlease download it from " + strSetupUrl + "\nand run it in the Vital Recorder installation folder");
		return;
	}

	SHELLEXECUTEINFO shExInfo = { 0 };
	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = 0;
	shExInfo.lpVerb = _T("runas"); // Operation to perform
	shExInfo.lpFile = tmppath; // Application to start    
	shExInfo.lpParameters = ""; // Additional parameters
	shExInfo.lpDirectory = GetModuleDir(); // ���� ���α׷� ���丮�� ������ Ǯ�����
	shExInfo.nShow = SW_SHOW;
	shExInfo.hInstApp = 0;

	if (!ShellExecuteEx(&shExInfo)) {
		AfxMessageBox("cannot start installer");
		return;
	}

	if (WAIT_OBJECT_0 != WaitForSingleObject(shExInfo.hProcess, INFINITE)) {
		AfxMessageBox("cannot install python");
		return;
	}

	CloseHandle(shExInfo.hProcess);

	theApp.Log("Python installed");
}

bool GetFileContent(LPCTSTR path, vector<BYTE>& ret) {
	FILE* f = fopen(path, "rb");
	if (!f) return false;
	fseek(f, 0, SEEK_END);
	auto len = ftell(f);
	rewind(f);
	ret.resize(len);
	fread(&ret[0], 1, len, f);
	fclose(f);
	return true;
}

void CVitalUtilsApp::LoadCache(CString dirname) {
	// ������ �ִ� trks ������ �о��
	dirname.TrimRight('\\');
	CString cachepath = dirname + "\\trks.tsv";
	vector<BYTE> buf;
	if (!GetFileContent(cachepath, buf)) return;
	
	// ���ϸ�\t�ð�\tƮ�����
	int icol = 0;
	int num = 0;
	CString tabs[3];
	for (auto c : buf) {
		switch (c) {
		case '\t':
			icol++;
			if (icol < 3) tabs[icol] = "";
			break;
		case '\n':
			if (tabs[0].Find('\\') == -1) { // old version �� ���� ������ ���ԵǾ��־���. �̰��� ����
				DWORD mtime = strtoul(tabs[1], nullptr, 10);
				auto path = dirname + '\\' + tabs[0];
				if (!path.IsEmpty() && mtime) {
					EnterCriticalSection(&theApp.m_csCache);
					m_path_trklist[path] = make_pair(mtime, tabs[2]);
					num++;
					LeaveCriticalSection(&theApp.m_csCache);
				}
			}
			icol = 0;
			tabs[0] = "";
			break;
		case '\r':
			break;
		default:
			if (icol < 3) tabs[icol].AppendChar(c);
		}
	}

	CString str; str.Format("Cache Loaded %s (%d records)", dirname, num);
	theApp.Log(str);
}

void CVitalUtilsApp::SaveCaches() {
	EnterCriticalSection(&m_csCache);
	auto need_updated = m_cache_updated; // ĳ�� ������Ʈ�� �ʿ��� ������
	m_cache_updated.clear(); // �ٸ� �����忡�� �߰��� �� �ֵ��� ��
	auto copyed = m_path_trklist;
	LeaveCriticalSection(&m_csCache);

	for (auto dirname : need_updated) {
		CString str;
		int num = 0;
		for (const auto& it : copyed) { // �ٸ� �����忡�� ��� �߰����̴�
			if (DirName(it.first) != dirname) continue;
			if (it.second.first == 0) continue; // mtime �� 0�̸�
			str += BaseName(it.first) + '\t';
			CString s; s.Format("%u", it.second.first);
			str += s + '\t' + it.second.second + '\n';
			num++;
		}

		auto cachefile = dirname + "trks.tsv.tmp";
		FILE* fo = fopen(cachefile, "wb");
		if (fo) {
			CString s; s.Format("Cache file saved: %s (%d records)", dirname, num);
			Log(s);
			::fwrite(str, 1, str.GetLength(), fo);
			::fclose(fo);

			// hidden file�� ����
			auto newfile = cachefile.Left(cachefile.GetLength() - 4);
			SetFileAttributes(cachefile, GetFileAttributes(cachefile) | FILE_ATTRIBUTE_HIDDEN);
			MoveFileEx(cachefile, newfile, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
		}
	}
}
