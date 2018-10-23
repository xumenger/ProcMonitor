#include <stdio.h>
#include <windows.h>
#include <WinNT.h>
#include <Tlhelp32.h>
#include <crtdbg.h>
#include <string.h>
#include "procmonitor.h"
#include "VMQuery.h"

//ʹ��Psapi�����ذ�װWindows SDK����Psapi.h��Psapi.lib�ŵ�Դ��·����
#include "./Psapi/Psapi.h"
#pragma comment(lib, "./Psapi/Psapi.lib")

#define VMQ_LINE_LEN  1024

const char *TABLE_HEAD   = "<table border='1' cellspacing='1' cellpadding='1' width='95%'>\n";
const char *TABLE_TH     = "  <tr bgcolor='#EEC591'><th>����ַ</th><th>����</th><th>��С</th><th>����</th><th>��������</th><th>����</th></tr>\n";
const char *TABLE_TAIL   = "</table>\n";
const char *RGN_TABLE_TD = "  <tr bgcolor='#F0F8FF'><td>%p</td><td>%s</td><td>%d</td><td>%d</td><td>%s</td><td>%s</td></tr>\n";
const char *BLK_TABLE_TD = "  <tr><td>%p</td><td>%s</td><td>%d</td><td>%s</td><td>%s</td><td>%s</td></tr>\n";

SYSTEM_INFO GSystemInfo;        //ϵͳ��Ϣ
int GIsFirstCall = 0;           //�Ƿ��һ�ε���

void GetRegionVMQLine(char *VMQLine, size_t length, HANDLE hProcess, VMQUERY *pVMQ);
void GetBlockVMQLine(char *VMQLine, size_t length, HANDLE hProcess, VMQUERY *pVMQ);
void GetMemStorageText(char *StorageText, size_t len, DWORD dwStorage);
void GetProtectText(char *ProtectText, size_t len, DWORD dwProtect);
void GetRegionInfo(char *RegionInfo, size_t len, HANDLE hProcess, VMQUERY *pVMQ);
BOOL ModuleFind(PVOID pvBaseAddr, PMODULEENTRY32 pme);
void RebuildMinHeap(FreeRegionInfo *freeRegionInfo, int N, int root);
void HeapToSortArray(FreeRegionInfo *freeRegionInfo, int N);

/*******************************************************************************
* Function   : GetProcessInfo
* Description: ��ȡ���̺�ΪProcessId�Ľ�����Ϣ
* Input      :
    * ProcessId, ���̺š������0������Ϊ�ǻ�ȡ��ǰ������Ϣ
* Output     :
    * processInfo, ������Ϣ�ṹ�塣���ΪNULLֱ�ӷ���-1
* Return     : 
    * <0, ��ȡ������Ϣ����
	* 0, ��ȡ������Ϣ�ɹ�
* Others     : 
*******************************************************************************/
int __stdcall GetProcessInfo(ProcessInfo *processInfo, unsigned long processId)
{
    ZeroMemory(processInfo, sizeof(*processInfo));
    if(0 == GIsFirstCall){
        GetSystemInfo(&GSystemInfo);
        GIsFirstCall++;
    }
    if(NULL == processInfo)
        return -1;

    if(0 == processId)
        processId = GetCurrentProcessId();
    processInfo->ProcessId = processId;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if(0 == hProcess)
        return -2;

    //��ȡ����ϵͳ��Ϣ
    processInfo->MaximumApplicationAddress = GSystemInfo.lpMaximumApplicationAddress;
    processInfo->MinimumApplicationAddress = GSystemInfo.lpMinimumApplicationAddress;
    processInfo->PageSize = GSystemInfo.dwPageSize;
    processInfo->AllocationGranularity = GSystemInfo.dwAllocationGranularity;

    //��ȡ����psapi�ڴ���Ϣ
    _PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc));
    processInfo->PageFaultCount = pmc.PageFaultCount;
    processInfo->PeakWorkingSetSize = pmc.PeakWorkingSetSize;
    processInfo->WorkingSetSize = pmc.WorkingSetSize;
    processInfo->QuotaPeakPagedPoolUsage = pmc.QuotaPeakPagedPoolUsage;
    processInfo->QuotaPagedPoolUsage = pmc.QuotaPagedPoolUsage;
    processInfo->QuotaPeakNonPagedPoolUsage = pmc.QuotaPeakNonPagedPoolUsage;
    processInfo->QuotaNonPagedPoolUsage = pmc.QuotaNonPagedPoolUsage;
    processInfo->PagefileUsage = pmc.PagefileUsage;
    processInfo->PeakPagefileUsage = pmc.PeakPagefileUsage;    

    //��ȡ���̶��ڴ���Ϣ
    processInfo->HeapCount = GetProcessHeaps(0, NULL);

    //�ҳ�ǰ10��Ŀ����ڴ�����
    GetMaxNFreeRegionInfo(processInfo->ContinuousFreeMem, 10, processId);
    return 0;
}

/*******************************************************************************
* Function   : SaveProcessInfo
* Description: ������̺�ΪprocessId����Ϣ
* Input      :
    * path, �����ļ���·��
    * ProcessId, ���̺š������0������Ϊ�ǻ�ȡ��ǰ������Ϣ
* Output     :
* Return     : 
    * <0, ���������Ϣ����
	* 0, ���������Ϣ�ɹ�
* Others     : 
*******************************************************************************/
int __stdcall SaveProcessInfo(const char *path, unsigned long processId)
{
    if(0 == processId)
        processId = GetCurrentProcessId();
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if(0 == hProcess)
        return -1;

    FILE *fp = fopen(path, "wt");
    if(NULL == fp)
        return -2;

    //���HTML���ͷ��
    const char *TableHead = TABLE_HEAD;
    fwrite(TableHead, strlen(TableHead), 1, fp);
    const char *TableTh = TABLE_TH;
    fwrite(TableTh, strlen(TableTh), 1, fp);

    //���HTML�������
    char VMQLine[VMQ_LINE_LEN + 1] = "";
    VMQUERY vmqRegion;
    BOOL bRgnOk = TRUE;
    PVOID pvRgnAddress = NULL;
    VMQUERY vmqBlock;
    BOOL bBlkOk = TRUE;    
    PVOID pvBlockAddress = NULL;
    int blockCount = 0;
    int i = 0;
    while(bRgnOk){
        bRgnOk = VMQuery(hProcess, pvRgnAddress, &vmqRegion);
        if(bRgnOk){
            //����������Ϣ
            GetRegionVMQLine(VMQLine, VMQ_LINE_LEN, hProcess, &vmqRegion);
            fwrite(VMQLine, strlen(VMQLine), 1, fp);
            if(vmqRegion.dwRgnStorage != MEM_FREE){
                //ѭ�������FREE�����µĿ���Ϣ
                blockCount = vmqRegion.dwRgnBlocks;
                pvBlockAddress = pvRgnAddress;
                for(i = 0; i < blockCount; i++)
                {
                    bBlkOk = VMQuery(hProcess, pvBlockAddress, &vmqBlock);
                    if(bBlkOk){
                        GetBlockVMQLine(VMQLine, VMQ_LINE_LEN, hProcess, &vmqBlock);
                        fwrite(VMQLine, strlen(VMQLine), 1, fp);
                        pvBlockAddress = ((PBYTE) vmqBlock.pvBlkBaseAddress + vmqBlock.BlkSize);
                    }
                }
            }
            pvRgnAddress = ((PBYTE) vmqRegion.pvRgnBaseAddress + vmqRegion.RgnSize);
        }
    }
    
    //���HTML���رձ�ǩ
    const char *TableTail =TABLE_TAIL;
    fwrite(TableTail, strlen(TableTail), 1, fp);
    fclose(fp);
    return 0;
}

/*******************************************************************************
* Function   : GetMaxNFreeRegionInfo
* Description: ��ȡǰN����ڴ�������Ϣ��ʹ��С����ʵ����������Top(k)����ľ���ⷨ��
    * С������һ����ȫ��������ÿ����Ҷ�ӽڵ�һ�������ں��ӽڵ㣬���ڵ�����Сֵ
    * ÿ���������������ڸ��ڵ�Ƚϣ��������ڸ��ڵ�������������������ֵ�滻���ڵ���ֵ����������С�ѵ���
    * ��������ά��һ���ѣ���һ��Ԫ���Ƕѵĸ��ڵ㡣��k���ڵ����ڵ���2k+1���ҽڵ���2k+2
* Input      :
    * N, �����ڴ��������
* Output     : 
    * freeRegionInfo, �����ڴ�������Ϣָ��
* Return     : 
* Others     : 
*******************************************************************************/
int __stdcall GetMaxNFreeRegionInfo(FreeRegionInfo *freeRegionInfo, int N, unsigned long processId)
{
    ZeroMemory(freeRegionInfo, sizeof(*freeRegionInfo) * N);
    if(0 == processId)
        processId = GetCurrentProcessId();
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if(0 == hProcess)
        return -1;
    VMQUERY vmqRegion;
    BOOL bRgnOk = TRUE;
    PVOID pvRgnAddress = NULL;
    while(bRgnOk){
        bRgnOk = VMQuery(hProcess, pvRgnAddress, &vmqRegion);
        if(bRgnOk){
            if((vmqRegion.dwRgnStorage == MEM_FREE) && (vmqRegion.RgnSize > freeRegionInfo[0].RgnSize)){
                freeRegionInfo[0].RgnBaseAddress = vmqRegion.pvRgnBaseAddress;
                freeRegionInfo[0].RgnSize = vmqRegion.RgnSize;
                //���¹���С����
                RebuildMinHeap(freeRegionInfo, N, 0);
            }
            pvRgnAddress = ((PBYTE) vmqRegion.pvRgnBaseAddress + vmqRegion.RgnSize);
        }
    }
    //��С���ѽṹ��������˳������
    HeapToSortArray(freeRegionInfo, N);

    return 0;
}

/*------------------------------------------------------------------------------
* Function   : GetRegionVMQLine
* Description: ���ڴ��������Ϣת����HTML Table��һ��
* Input      :
    * length, VMQLineָ����ڴ�Ŀ��ó���
    * hProcess, ���̾��
    * pVMQ, �ڴ�������Ϣ
* Output     : 
    * VMQLine, HTML Table��ʽ���ڴ�������Ϣ
* Return     : 
* Others     : 
------------------------------------------------------------------------------*/
void GetRegionVMQLine(char *VMQLine, size_t length, HANDLE hProcess, VMQUERY *pVMQ)
{
    const char *tdFormat = RGN_TABLE_TD;
    char StorageText[20] = "";
    char ProtectText[20] = "";
    char RegionInfo[500] = "";
    GetMemStorageText(StorageText, 19, pVMQ->dwRgnStorage);
    GetProtectText(ProtectText, 19, pVMQ->dwRgnProtection);
    GetRegionInfo(RegionInfo, 499, hProcess, pVMQ);
    sprintf(VMQLine, tdFormat, pVMQ->pvRgnBaseAddress
                             , StorageText
                             , pVMQ->RgnSize
                             , pVMQ->dwRgnBlocks
                             , ProtectText
                             , RegionInfo);
}

/*------------------------------------------------------------------------------
* Function   : GetBlockVMQLine
* Description: ���ڴ�����Ϣת����HTML Table��һ��
* Input      :
    * length, VMQLineָ����ڴ�Ŀ��ó���
    * hProcess, ���̾��
    * pVMQ, �ڴ����Ϣ
* Output     : 
    * VMQLine, HTML Table��ʽ���ڴ����Ϣ
* Return     : 
* Others     : 
------------------------------------------------------------------------------*/
void GetBlockVMQLine(char *VMQLine, size_t length, HANDLE hProcess, VMQUERY *pVMQ)
{
    const char *tdFormat = BLK_TABLE_TD;
    char StorageText[20] = "";
    char ProtectText[20] = "";
    GetMemStorageText(StorageText, 19, pVMQ->dwBlkStorage);
    GetProtectText(ProtectText, 19, pVMQ->dwBlkProtection);
    sprintf(VMQLine, tdFormat, pVMQ->pvBlkBaseAddress
                             , StorageText
                             , pVMQ->BlkSize
                             , ""
                             , ProtectText
                             , "");
}

/*------------------------------------------------------------------------------
* Function   : GetMemStorageText
* Description: �����ָ�ʽ������洢��������Ϣת��Ϊ�ַ�������
* Input      :
    * len, StorageText�ڴ�������ֽ���
    * dwStorage, ���ָ�ʽ������洢��������Ϣ
* Output     :
    * StorageText, ����洢����Ϣ���ַ�����ʽ���������ڴ�����
* Return     : 
* Others     : 
------------------------------------------------------------------------------*/
void GetMemStorageText(char *StorageText, size_t len, DWORD dwStorage)
{
    if(len < 7)
        return;
    switch (dwStorage) {
        case MEM_FREE:    strcpy(StorageText, "Free   "); break;
        case MEM_RESERVE: strcpy(StorageText, "Reserve"); break;
        case MEM_IMAGE:   strcpy(StorageText, "Image  "); break;
        case MEM_MAPPED:  strcpy(StorageText, "Mapped "); break;
        case MEM_PRIVATE: strcpy(StorageText, "Private"); break;
        default:          strcpy(StorageText, "");
   }
}

/*------------------------------------------------------------------------------
* Function   : GetProtectText
* Description: �����ָ�ʽ�ı���������Ϣת��Ϊ�ַ�������
* Input      :
    * len, ProtectText�ڴ�������ֽ���
    * dwProtect, ���ָ�ʽ�ı�������������Ϣ
* Output     :
    * ProtectText, ����������Ϣ���ַ�����ʽ���������ڴ�����
* Return     : 
* Others     : 
------------------------------------------------------------------------------*/
void GetProtectText(char *ProtectText, size_t len, DWORD dwProtect)
{
    if(len < 4)
        return;
    switch (dwProtect & ~(PAGE_GUARD | PAGE_NOCACHE | PAGE_WRITECOMBINE)) {
        case PAGE_READONLY:          strcpy(ProtectText, "-R--"); break;
        case PAGE_READWRITE:         strcpy(ProtectText, "-RW-"); break;
        case PAGE_WRITECOPY:         strcpy(ProtectText, "-RWC"); break;
        case PAGE_EXECUTE:           strcpy(ProtectText, "E---"); break;
        case PAGE_EXECUTE_READ:      strcpy(ProtectText, "ER--"); break;
        case PAGE_EXECUTE_READWRITE: strcpy(ProtectText, "ERW-"); break;
        case PAGE_EXECUTE_WRITECOPY: strcpy(ProtectText, "ERWC"); break;
        case PAGE_NOACCESS:          strcpy(ProtectText, "----"); break;
        default:                     strcpy(ProtectText, "");
   }
}

/*------------------------------------------------------------------------------
* Function   : GetRegionInfo
* Description: ��ȡpVMQ��Ӧ�ڴ������������Ϣ��ӳ���ļ�����
* Input      :
    * len, RegionInfo���õ��ڴ��С
    * hProcess, ���̾��
    * pVMQ, �ڴ�������Ϣ
* Output     :
    * RegionInfo, ������Ϣ���ַ�����ʽ����ڴ��ڴ�����
* Return     : 
* Others     : 
------------------------------------------------------------------------------*/
void GetRegionInfo(char *RegionInfo, size_t len, HANDLE hProcess, VMQUERY *pVMQ)
{
    if(pVMQ->bRgnIsAStack)
        strcpy(RegionInfo, "Thread Stack");
    if((pVMQ->dwRgnStorage != MEM_FREE) && (pVMQ->pvRgnBaseAddress != NULL)){
        MODULEENTRY32 me = { sizeof(me) };
        //��ȡpVMQ->pvRgnBaseAddress��ַ����ģ����Ϣ
        if(ModuleFind(pVMQ->pvRgnBaseAddress, &me))
            strcpy(RegionInfo, me.szExePath);
        else{
            int exePathLen = strlen(RegionInfo);
            DWORD dwLen = GetMappedFileNameA(hProcess, pVMQ->pvRgnBaseAddress, RegionInfo + exePathLen, len - exePathLen);
            if(dwLen == 0)
                RegionInfo[exePathLen] = '\0';
        }
    }
}

/*------------------------------------------------------------------------------
* Function   : ModuleFind
* Description: ��ȡpvBaseAddr��ַ����ģ����Ϣ
* Input      :
    * pvBaseAddr, ��ַ
* Output     :
    * pme, ��ַ����ģ����Ϣ
* Return     : 
    * �Ƿ���ָ����ַ���ҵ�ģ��
* Others     : 
------------------------------------------------------------------------------*/
BOOL ModuleFind(PVOID pvBaseAddr, PMODULEENTRY32 pme)
{
    BOOL bFound = FALSE;
    HANDLE Snapshot = INVALID_HANDLE_VALUE;
    for(BOOL bOk = Module32First(Snapshot, pme); bOk; bOk = Module32Next(Snapshot, pme)) {
        bFound = (pme->modBaseAddr == pvBaseAddr);
        if(bFound) 
            break;
    }
    return bFound;
}

/*------------------------------------------------------------------------------
* Function   : RebuildMinHeap
* Description: ���¹�����С��
    * 0�ڵ�����������ڵ�Ƚϣ����0�ڵ�����Ƕ�С������Ա���
    * ��������ҽڵ����0�ڵ�����(��)�ڵ�����С�Ľڵ㽻����������ά��������
    * ����ִ�У�ֱ��������N�����б�����n��������N������n����
    * С���Ѵ����������
           1
      2         3
   4     5   6     7
  8 9  10

* Input      :
* Output     :
* Return     : 
* Others     : 
------------------------------------------------------------------------------*/
void RebuildMinHeap(FreeRegionInfo *freeRegionInfo, int N, int root)
{
    FreeRegionInfo tmpRegion;
    int left = 2 * root + 1;
    int right  = 2 * root + 2;
    int pos;
    //���û�к��ӽڵ��򲻼���������
    if(left >= N)
        return ;
    if(right >= N)       //ֻ�����ӽڵ�
        pos = left;
    else                 //�������ڵ���ȡ�����ڵ��еĽ�Сֵ
        pos = freeRegionInfo[left].RgnSize > freeRegionInfo[right].RgnSize ? right : left;
    //����µĸ��ڵ�ϴ���������ӽڵ��еĽ�Сֵ�����������¼���ά��������ΪС����
    if(freeRegionInfo[root].RgnSize > freeRegionInfo[pos].RgnSize){
        //���ڵ����Сֵ
        tmpRegion = freeRegionInfo[root];
        freeRegionInfo[root] = freeRegionInfo[pos];
        freeRegionInfo[pos] = tmpRegion;
        //�ݹ�ά������ΪС����
        RebuildMinHeap(freeRegionInfo, N, pos);
    }
}

/*------------------------------------------------------------------------------
* Function   : HeapToSortArray
* Description: ��С���ѽṹ��������˳������
    * ��С���ѽṹ��������Ԫ�ذ��մӴ�С��������
    * �����Ѷ�Ԫ�غ����һ��Ԫ�أ���ʱ���һ��λ����Ϊ��������Ȼ������������ΪС����
    * Ȼ��������Ѷ�Ԫ�غ͵����ڶ���Ԫ�ؽ���������������ֱ��ѭ��������Ԫ��
* Input      :
* Output     :
* Return     : 
* Others     : 
------------------------------------------------------------------------------*/
void HeapToSortArray(FreeRegionInfo *freeRegionInfo, int N)
{
    int i;
    FreeRegionInfo tmpRegion;
    for(i = 0; i < N; i++){
        tmpRegion = freeRegionInfo[N - i - 1];
        freeRegionInfo[N - i - 1] = freeRegionInfo[0];
        freeRegionInfo[0] = tmpRegion;
        RebuildMinHeap(freeRegionInfo, N - i - 1, 0);
    }
}
