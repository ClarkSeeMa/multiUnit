
// recodeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "recode.h"
#include "recodeDlg.h"
#include "afxdialogex.h"
#include <windows.h>
#include "MMSystem.h"				//多媒体接口头文件	

#pragma comment(lib,"winmm.lib") 

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define INP_BUFFER_SIZE (20 * 1024)	//定义缓冲区大小
bool m_record, m_play;				//m_record表示是否正在录音，m_play表示是否正在回放

WAVEFORMATEX waveform;				//WAV文件头包含音频格式
DWORD dwDataLength, dwRepetitions;	//dwDataLength已有的数据长度，dwRepetitions重复次数
HWAVEIN hWaveIn;					//输入设备句柄
HWAVEOUT hWaveOut;					//输出设备句柄
PBYTE pBuffer1, pBuffer2;			//保存输入数据的两个缓冲区。
//如果只要一个缓冲区，当缓冲区满，保存数据时，会无法保存这段时间采集的语音，导致最后获得的声音断断续续。
//使用两个缓冲区，当一个缓冲区满的时候，保存这个已满的缓冲区数据，而由另一个缓冲区继续采集语音。
PBYTE pSaveBuffer, pNewBuffer;		//保存数据的内存地址。
PWAVEHDR pWaveHdr1, pWaveHdr2;		//声音文件头


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
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


// CrecodeDlg 对话框


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


void RecordStart()     //录音准备
{
	m_record=true;
	pBuffer1=(PBYTE)malloc(INP_BUFFER_SIZE);
	pBuffer2=(PBYTE)malloc(INP_BUFFER_SIZE);    //给缓冲区分配空间
	if (!pBuffer1||!pBuffer2)
	{
		if (pBuffer1) free(pBuffer1);
		if (pBuffer2) free(pBuffer2);
		MessageBeep(MB_ICONEXCLAMATION);
		AfxMessageBox(L"Memory error!");
		return ;
	}

	//设置录音方式
	waveform.wFormatTag  = WAVE_FORMAT_PCM;		//PCM编码
	waveform.nChannels  = 1;					//单声道
	waveform.nSamplesPerSec = 16000;			//采样频率，每秒采集次数
	waveform.nAvgBytesPerSec= waveform.nSamplesPerSec * sizeof(unsigned short);
	waveform.wBitsPerSample = 16;				//采样位，模拟信号转数字信号的精准度
	waveform.nBlockAlign = waveform.nChannels * waveform.wBitsPerSample / 8;
	waveform.cbSize   = 0;						//PCM编码时，此处为0
	MMRESULT xx;
	if (MMSYSERR_NOERROR != (xx = waveInOpen(&hWaveIn,WAVE_MAPPER,&waveform,(DWORD)theApp.m_pMainWnd->m_hWnd,NULL,CALLBACK_WINDOW))) //打开输入设备
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
	//初始化声音文件头
	pWaveHdr1->lpData=(LPSTR)pBuffer1;   //设置缓冲区
	pWaveHdr1->dwBufferLength=INP_BUFFER_SIZE; //缓冲区大小
	pWaveHdr1->dwBytesRecorded=0;
	pWaveHdr1->dwUser=0;
	pWaveHdr1->dwFlags=0;
	pWaveHdr1->dwLoops=1;
	pWaveHdr1->lpNext=NULL;
	pWaveHdr1->reserved=0;
	waveInPrepareHeader(hWaveIn,pWaveHdr1,sizeof(WAVEHDR));  //将缓冲区信息和输入设备关联
	waveInAddBuffer (hWaveIn, pWaveHdr1, sizeof (WAVEHDR)) ; //将缓冲区地址添加到输入设备中

	pWaveHdr2->lpData=(LPSTR)pBuffer2;
	pWaveHdr2->dwBufferLength=INP_BUFFER_SIZE;
	pWaveHdr2->dwBytesRecorded=0;
	pWaveHdr2->dwUser=0;
	pWaveHdr2->dwFlags=0;
	pWaveHdr2->dwLoops=1;
	pWaveHdr2->lpNext=NULL;
	pWaveHdr2->reserved=0;
	waveInPrepareHeader(hWaveIn,pWaveHdr2,sizeof(WAVEHDR));
	waveInAddBuffer (hWaveIn, pWaveHdr2, sizeof (WAVEHDR)) ; //同上

	pSaveBuffer = (PBYTE)realloc (pSaveBuffer, 1) ;
	dwDataLength = 0 ;
	waveInStart (hWaveIn) ; //打开输入设备，开始录音
}

void RecordStop()
{
	m_record=false;
	waveInReset(hWaveIn); //停止录音，关闭输入设备
}
void PlayStart()
{
	if (waveOutOpen(&hWaveOut,WAVE_MAPPER,&waveform,(DWORD)theApp.m_pMainWnd->m_hWnd,NULL,CALLBACK_WINDOW)) //打开输出设备，开始回放
	{
		MessageBeep(MB_ICONEXCLAMATION);
		AfxMessageBox(L"Audio output error");
	}
	m_play=true;
}
void PlayStop()
{
	waveOutReset(hWaveOut);  //停止回放，关闭输出设备
	m_play = false;
}

// CrecodeDlg 消息处理程序
LRESULT CrecodeDlg::OnMM_WIM_OPEN(UINT wParam, LONG lParam) //开始录音
{
	// TODO: Add your message handler code here and/or call default
	m_record=TRUE;
	TRACE(L"MM_WIM_OPEN\n");
	return 0;
}

LRESULT CrecodeDlg::OnMM_WIM_DATA(UINT wParam, LONG lParam) //缓冲区满的时候，对应的声音文件头如pWaveHdr1作为lParam传递进来
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

	pSaveBuffer = pNewBuffer ;  //在pSaveBuffer尾部继续申请空间（上面的realloc 函数）
	//////////////////////////////////////////////////////////////////////////

	CopyMemory(pSaveBuffer + dwDataLength, ((PWAVEHDR) lParam)->lpData,
		((PWAVEHDR) lParam)->dwBytesRecorded) ; //将缓冲区数据((PWAVEHDR) lParam)->lpData复制到pSaveBuffer的尾部刚申请的空间中

	dwDataLength += ((PWAVEHDR) lParam)->dwBytesRecorded ;//加长pSaveBuffer的实际数据长度

	if (m_record==false)
	{
		waveInClose (hWaveIn) ;//停止录音，关闭输入设备
		return 0;
	}

	//将音频写入到文件中
	FILE* fp=fopen("ecord.pcm","ab+");
	if(fp==NULL)
	{
		printf("fopen error,%d",__LINE__);
	}
	fwrite(((PWAVEHDR) lParam)->lpData,((PWAVEHDR) lParam)->dwBytesRecorded,1,fp);
	fclose(fp);

	// Send out a new buffer
	waveInAddBuffer (hWaveIn, (PWAVEHDR) lParam, sizeof (WAVEHDR)) ;//将缓冲区添加回到设备中
	//假如现在是pWaveHdr1满了，lParam就是pWaveHdr1，在我们保存pWaveHdr1的数据时，pWaveHdr2正在录音，保存完pWaveHdr1，再把pWaveHdr1添加回到设备中，这样达到两个缓冲区交替使用。
	TRACE(L"done input data\n");
	return 0;
	

}

LRESULT CrecodeDlg::OnMM_WIM_CLOSE(UINT wParam, LONG lParam) //停止录音时
{	
	// TODO: Add your message handler code here and/or call default
	TRACE(L"MM_WIM_CLOSE\n");

	if (0==dwDataLength) {   //没有数据，长度为0
		return 0;
	}
	waveInUnprepareHeader (hWaveIn, pWaveHdr1, sizeof (WAVEHDR)) ;//取消输入设备和pWaveHdr1的关联
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

LRESULT CrecodeDlg::OnMM_WOM_OPEN(UINT wParam, LONG lParam)//开始回放
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

LRESULT CrecodeDlg::OnMM_WOM_DONE(UINT wParam, LONG lParam)//回放完毕
{ 	
	TRACE(L"open MM_WOM_DONE\n");
	waveOutUnprepareHeader (hWaveOut, pWaveHdr1, sizeof (WAVEHDR));
	waveOutClose (hWaveOut) ;

	dwRepetitions = 1 ;
	m_play = FALSE ; 

	return  0;
	
}
LRESULT CrecodeDlg::OnMM_WOM_CLOSE(UINT wParam, LONG lParam)//关闭回放
{ 	
	TRACE(L"open MM_WOM_CLOSE\n");
	dwRepetitions = 1 ;
	m_play = FALSE ; 

	return 0;
}


BOOL CrecodeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	pWaveHdr1=reinterpret_cast<PWAVEHDR>(malloc(sizeof(WAVEHDR)));
	pWaveHdr2=reinterpret_cast<PWAVEHDR>(malloc(sizeof(WAVEHDR))); //给声音文件头分配内存空间
	pSaveBuffer = reinterpret_cast<PBYTE>(malloc(1));    //给数据内存地址分配空间


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CrecodeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CrecodeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CrecodeDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	RecordStart();
}


void CrecodeDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	RecordStop();
}


void CrecodeDlg::OnBnClickedMfcbutton2()
{
	// TODO: 在此添加控件通知处理程序代码
	PlayStart();
}


void CrecodeDlg::OnBnClickedMfcbutton3()
{
	// TODO: 在此添加控件通知处理程序代码
	PlayStop();
}
