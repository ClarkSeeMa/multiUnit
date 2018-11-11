
// recodeDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "recode.h"
#include "recodeDlg.h"
#include "afxdialogex.h"
#include <windows.h>
#include "MMSystem.h"				//��ý��ӿ�ͷ�ļ�	

#pragma comment(lib,"winmm.lib") 

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define INP_BUFFER_SIZE (20 * 1024)	//���建������С
bool m_record, m_play;				//m_record��ʾ�Ƿ�����¼����m_play��ʾ�Ƿ����ڻط�

WAVEFORMATEX waveform;				//WAV�ļ�ͷ������Ƶ��ʽ
DWORD dwDataLength, dwRepetitions;	//dwDataLength���е����ݳ��ȣ�dwRepetitions�ظ�����
HWAVEIN hWaveIn;					//�����豸���
HWAVEOUT hWaveOut;					//����豸���
PBYTE pBuffer1, pBuffer2;			//�����������ݵ�������������
//���ֻҪһ������������������������������ʱ�����޷��������ʱ��ɼ�����������������õ������϶�������
//ʹ����������������һ������������ʱ�򣬱�����������Ļ��������ݣ�������һ�������������ɼ�������
PBYTE pSaveBuffer, pNewBuffer;		//�������ݵ��ڴ��ַ��
PWAVEHDR pWaveHdr1, pWaveHdr2;		//�����ļ�ͷ


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CrecodeDlg �Ի���


CrecodeDlg::CrecodeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CrecodeDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CrecodeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CrecodeDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(MM_WIM_OPEN,OnMM_WIM_OPEN)
	ON_MESSAGE(MM_WIM_DATA,OnMM_WIM_DATA)
	ON_MESSAGE(MM_WIM_CLOSE,OnMM_WIM_CLOSE)
	ON_MESSAGE(MM_WOM_OPEN,OnMM_WOM_OPEN)
	ON_MESSAGE(MM_WOM_DONE,OnMM_WOM_DONE)
	ON_MESSAGE(MM_WOM_CLOSE,OnMM_WOM_CLOSE)
	ON_BN_CLICKED(IDC_BUTTON1, &CrecodeDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CrecodeDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_MFCBUTTON2, &CrecodeDlg::OnBnClickedMfcbutton2)
	ON_BN_CLICKED(IDC_MFCBUTTON3, &CrecodeDlg::OnBnClickedMfcbutton3)
END_MESSAGE_MAP()


void RecordStart()     //¼��׼��
{
	m_record=true;
	pBuffer1=(PBYTE)malloc(INP_BUFFER_SIZE);
	pBuffer2=(PBYTE)malloc(INP_BUFFER_SIZE);    //������������ռ�
	if (!pBuffer1||!pBuffer2)
	{
		if (pBuffer1) free(pBuffer1);
		if (pBuffer2) free(pBuffer2);
		MessageBeep(MB_ICONEXCLAMATION);
		AfxMessageBox(L"Memory error!");
		return ;
	}

	//����¼����ʽ
	waveform.wFormatTag  = WAVE_FORMAT_PCM;		//PCM����
	waveform.nChannels  = 1;					//������
	waveform.nSamplesPerSec = 16000;			//����Ƶ�ʣ�ÿ��ɼ�����
	waveform.nAvgBytesPerSec= waveform.nSamplesPerSec * sizeof(unsigned short);
	waveform.wBitsPerSample = 16;				//����λ��ģ���ź�ת�����źŵľ�׼��
	waveform.nBlockAlign = waveform.nChannels * waveform.wBitsPerSample / 8;
	waveform.cbSize   = 0;						//PCM����ʱ���˴�Ϊ0
	MMRESULT xx;
	if (MMSYSERR_NOERROR != (xx = waveInOpen(&hWaveIn,WAVE_MAPPER,&waveform,(DWORD)theApp.m_pMainWnd->m_hWnd,NULL,CALLBACK_WINDOW))) //�������豸
	{ 
		if (xx == MMSYSERR_ALLOCATED)
		{
			AfxMessageBox(L"MMSYSERR_ALLOCATED!");
		}else if (xx == MMSYSERR_BADDEVICEID)
		{
			AfxMessageBox(L"MMSYSERR_BADDEVICEID!");
		}else if (xx == MMSYSERR_NODRIVER)
		{
			AfxMessageBox(L"MMSYSERR_NODRIVER!");
		}else if (xx == MMSYSERR_NOMEM)
		{
			AfxMessageBox(L"MMSYSERR_NOMEM!");
		}else if (xx == WAVERR_BADFORMAT)
		{
			AfxMessageBox(L"WAVERR_BADFORMAT!");
		}

		free(pBuffer1);
		free(pBuffer2);
		MessageBeep(MB_ICONEXCLAMATION);
		AfxMessageBox(L"Audio can not be open!");
		return;
	}
	//��ʼ�������ļ�ͷ
	pWaveHdr1->lpData=(LPSTR)pBuffer1;   //���û�����
	pWaveHdr1->dwBufferLength=INP_BUFFER_SIZE; //��������С
	pWaveHdr1->dwBytesRecorded=0;
	pWaveHdr1->dwUser=0;
	pWaveHdr1->dwFlags=0;
	pWaveHdr1->dwLoops=1;
	pWaveHdr1->lpNext=NULL;
	pWaveHdr1->reserved=0;
	waveInPrepareHeader(hWaveIn,pWaveHdr1,sizeof(WAVEHDR));  //����������Ϣ�������豸����
	waveInAddBuffer (hWaveIn, pWaveHdr1, sizeof (WAVEHDR)) ; //����������ַ��ӵ������豸��

	pWaveHdr2->lpData=(LPSTR)pBuffer2;
	pWaveHdr2->dwBufferLength=INP_BUFFER_SIZE;
	pWaveHdr2->dwBytesRecorded=0;
	pWaveHdr2->dwUser=0;
	pWaveHdr2->dwFlags=0;
	pWaveHdr2->dwLoops=1;
	pWaveHdr2->lpNext=NULL;
	pWaveHdr2->reserved=0;
	waveInPrepareHeader(hWaveIn,pWaveHdr2,sizeof(WAVEHDR));
	waveInAddBuffer (hWaveIn, pWaveHdr2, sizeof (WAVEHDR)) ; //ͬ��

	pSaveBuffer = (PBYTE)realloc (pSaveBuffer, 1) ;
	dwDataLength = 0 ;
	waveInStart (hWaveIn) ; //�������豸����ʼ¼��
}

void RecordStop()
{
	m_record=false;
	waveInReset(hWaveIn); //ֹͣ¼�����ر������豸
}
void PlayStart()
{
	if (waveOutOpen(&hWaveOut,WAVE_MAPPER,&waveform,(DWORD)theApp.m_pMainWnd->m_hWnd,NULL,CALLBACK_WINDOW)) //������豸����ʼ�ط�
	{
		MessageBeep(MB_ICONEXCLAMATION);
		AfxMessageBox(L"Audio output error");
	}
	m_play=true;
}
void PlayStop()
{
	waveOutReset(hWaveOut);  //ֹͣ�طţ��ر�����豸
	m_play = false;
}

// CrecodeDlg ��Ϣ�������
LRESULT CrecodeDlg::OnMM_WIM_OPEN(UINT wParam, LONG lParam) //��ʼ¼��
{
	// TODO: Add your message handler code here and/or call default
	m_record=TRUE;
	TRACE(L"MM_WIM_OPEN\n");
	return 0;
}

LRESULT CrecodeDlg::OnMM_WIM_DATA(UINT wParam, LONG lParam) //����������ʱ�򣬶�Ӧ�������ļ�ͷ��pWaveHdr1��ΪlParam���ݽ���
{
	// TODO: Add your message handler code here and/or call default
	// Reallocate save buffer memory

	pNewBuffer = (PBYTE)realloc (pSaveBuffer, dwDataLength +
		((PWAVEHDR) lParam)->dwBytesRecorded) ; 

	if (pNewBuffer == NULL)
	{
		waveInClose (hWaveIn) ;
		MessageBeep (MB_ICONEXCLAMATION) ;
		AfxMessageBox(L"error memory");
		return 0;
	}

	pSaveBuffer = pNewBuffer ;  //��pSaveBufferβ����������ռ䣨�����realloc ������
	//////////////////////////////////////////////////////////////////////////

	CopyMemory(pSaveBuffer + dwDataLength, ((PWAVEHDR) lParam)->lpData,
		((PWAVEHDR) lParam)->dwBytesRecorded) ; //������������((PWAVEHDR) lParam)->lpData���Ƶ�pSaveBuffer��β��������Ŀռ���

	dwDataLength += ((PWAVEHDR) lParam)->dwBytesRecorded ;//�ӳ�pSaveBuffer��ʵ�����ݳ���

	if (m_record==false)
	{
		waveInClose (hWaveIn) ;//ֹͣ¼�����ر������豸
		return 0;
	}

	//����Ƶд�뵽�ļ���
	FILE* fp=fopen("ecord.pcm","ab+");
	if(fp==NULL)
	{
		printf("fopen error,%d",__LINE__);
	}
	fwrite(((PWAVEHDR) lParam)->lpData,((PWAVEHDR) lParam)->dwBytesRecorded,1,fp);
	fclose(fp);

	// Send out a new buffer
	waveInAddBuffer (hWaveIn, (PWAVEHDR) lParam, sizeof (WAVEHDR)) ;//����������ӻص��豸��
	//����������pWaveHdr1���ˣ�lParam����pWaveHdr1�������Ǳ���pWaveHdr1������ʱ��pWaveHdr2����¼����������pWaveHdr1���ٰ�pWaveHdr1��ӻص��豸�У������ﵽ��������������ʹ�á�
	TRACE(L"done input data\n");
	return 0;
	

}

LRESULT CrecodeDlg::OnMM_WIM_CLOSE(UINT wParam, LONG lParam) //ֹͣ¼��ʱ
{	
	// TODO: Add your message handler code here and/or call default
	TRACE(L"MM_WIM_CLOSE\n");

	if (0==dwDataLength) {   //û�����ݣ�����Ϊ0
		return 0;
	}
	waveInUnprepareHeader (hWaveIn, pWaveHdr1, sizeof (WAVEHDR)) ;//ȡ�������豸��pWaveHdr1�Ĺ���
	waveInUnprepareHeader (hWaveIn, pWaveHdr2, sizeof (WAVEHDR)) ;

	m_record = FALSE ;

	free (pBuffer1) ;
	free (pBuffer2) ;

	if (dwDataLength > 0)
	{
		//enable play
	}
	return 0;
}

LRESULT CrecodeDlg::OnMM_WOM_OPEN(UINT wParam, LONG lParam)//��ʼ�ط�
{	
	TRACE(L"open MM_WOM_OPEN\n");
	// Set up header

	pWaveHdr1->lpData          = (LPSTR)pSaveBuffer ;
	pWaveHdr1->dwBufferLength  = dwDataLength ;
	pWaveHdr1->dwBytesRecorded = 0 ;
	pWaveHdr1->dwUser          = 0 ;
	pWaveHdr1->dwFlags         = WHDR_BEGINLOOP | WHDR_ENDLOOP ;
	pWaveHdr1->dwLoops         = dwRepetitions ;
	pWaveHdr1->lpNext          = NULL ;
	pWaveHdr1->reserved        = 0 ;

	// Prepare and write

	waveOutPrepareHeader (hWaveOut, pWaveHdr1, sizeof (WAVEHDR)) ;
	waveOutWrite (hWaveOut, pWaveHdr1, sizeof (WAVEHDR)) ;

	m_play = TRUE ;

	return 0;
}

LRESULT CrecodeDlg::OnMM_WOM_DONE(UINT wParam, LONG lParam)//�ط����
{ 	
	TRACE(L"open MM_WOM_DONE\n");
	waveOutUnprepareHeader (hWaveOut, pWaveHdr1, sizeof (WAVEHDR));
	waveOutClose (hWaveOut) ;

	dwRepetitions = 1 ;
	m_play = FALSE ; 

	return  0;
	
}
LRESULT CrecodeDlg::OnMM_WOM_CLOSE(UINT wParam, LONG lParam)//�رջط�
{ 	
	TRACE(L"open MM_WOM_CLOSE\n");
	dwRepetitions = 1 ;
	m_play = FALSE ; 

	return 0;
}


BOOL CrecodeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	pWaveHdr1=reinterpret_cast<PWAVEHDR>(malloc(sizeof(WAVEHDR)));
	pWaveHdr2=reinterpret_cast<PWAVEHDR>(malloc(sizeof(WAVEHDR))); //�������ļ�ͷ�����ڴ�ռ�
	pSaveBuffer = reinterpret_cast<PBYTE>(malloc(1));    //�������ڴ��ַ����ռ�


	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CrecodeDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CrecodeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CrecodeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CrecodeDlg::OnBnClickedButton1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	RecordStart();
}


void CrecodeDlg::OnBnClickedButton2()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	RecordStop();
}


void CrecodeDlg::OnBnClickedMfcbutton2()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	PlayStart();
}


void CrecodeDlg::OnBnClickedMfcbutton3()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	PlayStop();
}
