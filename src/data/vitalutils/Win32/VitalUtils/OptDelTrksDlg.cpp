#include "stdafx.h"
#include "VitalUtils.h"
#include "OptDelTrksDlg.h"
#include "afxdialogex.h"

IMPLEMENT_DYNAMIC(COptDelTrksDlg, CDialogEx)

COptDelTrksDlg::COptDelTrksDlg(CWnd* pParent /*=NULL*/)
: CDialogEx(COptDelTrksDlg::IDD, pParent) {
}

COptDelTrksDlg::~COptDelTrksDlg() {
}

void COptDelTrksDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(COptDelTrksDlg, CDialogEx)
END_MESSAGE_MAP()

BOOL COptDelTrksDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // ����: OCX �Ӽ� �������� FALSE�� ��ȯ�ؾ� �մϴ�.
}

