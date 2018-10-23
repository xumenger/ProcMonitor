#include <stdio.h>
#include <windows.h>
#include <WinNT.h>
#include <Tlhelp32.h>
#include <crtdbg.h>
#include <string.h>
#include "procmonitor.h"
#include "VMQuery.h"

//使用Psapi可下载安装Windows SDK，或将Psapi.h、Psapi.lib放到源码路径下
#include "./Psapi/Psapi.h"
#pragma comment(lib, "./Psapi/Psapi.lib")

#define VMQ_LINE_LEN  1024

const char *TABLE_HEAD   = "<table border='1' cellspacing='1' cellpadding='1' width='95%'>\n";
const char *TABLE_TH     = "  <tr bgcolor='#EEC591'><th>基地址</th><th>类型</th><th>大小</th><th>块数</th><th>保护属性</th><th>描述</th></tr>\n";
const char *TABLE_TAIL   = "</table>\n";
const char *RGN_TABLE_TD = "  <tr bgcolor='#F0F8FF'><td>%p</td><td>%s</td><td>%d</td><td>%d</td><td>%s</td><td>%s</td></tr>\n";
const char *BLK_TABLE_TD = "  <tr><td>%p</td><td>%s</td><td>%d</td><td>%s</td><td>%s</td><td>%s</td></tr>\n";

SYSTEM_INFO GSystemInfo;        //系统信息
int GIsFirstCall = 0;           //是否第一次调用

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
* Description: 获取进程号为ProcessId的进程信息
* Input      :
    * ProcessId, 进程号。如果是0，则认为是获取当前进程信息
* Output     :
    * processInfo, 进程信息结构体。如果为NULL直接返回-1
* Return     : 
    * <0, 获取进程信息错误
	* 0, 获取进程信息成功
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

    //获取操作系统信息
    processInfo->MaximumApplicationAddress = GSystemInfo.lpMaximumApplicationAddress;
    processInfo->MinimumApplicationAddress = GSystemInfo.lpMinimumApplicationAddress;
    processInfo->PageSize = GSystemInfo.dwPageSize;
    processInfo->AllocationGranularity = GSystemInfo.dwAllocationGranularity;

    //获取进程psapi内存信息
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

    //获取进程堆内存信息
    processInfo->HeapCount = GetProcessHeaps(0, NULL);

    //找出前10大的空闲内存区域
    GetMaxNFreeRegionInfo(processInfo->ContinuousFreeMem, 10, processId);
    return 0;
}

/*******************************************************************************
* Function   : SaveProcessInfo
* Description: 保存进程号为processId的信息
* Input      :
    * path, 保存文件的路径
    * ProcessId, 进程号。如果是0，则认为是获取当前进程信息
* Output     :
* Return     : 
    * <0, 保存进程信息错误
	* 0, 保存进程信息成功
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

    //输出HTML表格头部
    const char *TableHead = TABLE_HEAD;
    fwrite(TableHead, strlen(TableHead), 1, fp);
    const char *TableTh = TABLE_TH;
    fwrite(TableTh, strlen(TableTh), 1, fp);

    //输出HTML表格内容
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
            //保存区域信息
            GetRegionVMQLine(VMQLine, VMQ_LINE_LEN, hProcess, &vmqRegion);
            fwrite(VMQLine, strlen(VMQLine), 1, fp);
            if(vmqRegion.dwRgnStorage != MEM_FREE){
                //循环保存非FREE区域下的块信息
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
    
    //输出HTML表格关闭标签
    const char *TableTail =TABLE_TAIL;
    fwrite(TableTail, strlen(TableTail), 1, fp);
    fclose(fp);
    return 0;
}

/*******************************************************************************
* Function   : GetMaxNFreeRegionInfo
* Description: 获取前N大的内存区域信息，使用小顶堆实现排序（这是Top(k)问题的经典解法）
    * 小顶堆是一个完全二叉树，每个非叶子节点一定不大于孩子节点，根节点是最小值
    * 每次有数据输入先于根节点比较，若不大于根节点则舍弃，否则用新数值替换根节点数值，并进行最小堆调整
    * 在数组中维护一个堆，第一个元素是堆的根节点。第k个节点的左节点是2k+1，右节点是2k+2
* Input      :
    * N, 空闲内存区域个数
* Output     : 
    * freeRegionInfo, 空闲内存区域信息指针
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
                //重新构建小顶堆
                RebuildMinHeap(freeRegionInfo, N, 0);
            }
            pvRgnAddress = ((PBYTE) vmqRegion.pvRgnBaseAddress + vmqRegion.RgnSize);
        }
    }
    //将小顶堆结构的数组变成顺序数组
    HeapToSortArray(freeRegionInfo, N);

    return 0;
}

/*------------------------------------------------------------------------------
* Function   : GetRegionVMQLine
* Description: 将内存区域的信息转换成HTML Table的一行
* Input      :
    * length, VMQLine指向的内存的可用长度
    * hProcess, 进程句柄
    * pVMQ, 内存区域信息
* Output     : 
    * VMQLine, HTML Table格式的内存区域信息
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
* Description: 将内存块的信息转换成HTML Table的一行
* Input      :
    * length, VMQLine指向的内存的可用长度
    * hProcess, 进程句柄
    * pVMQ, 内存块信息
* Output     : 
    * VMQLine, HTML Table格式的内存块信息
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
* Description: 将数字格式的物理存储器类型信息转换为字符串类型
* Input      :
    * len, StorageText内存区域的字节数
    * dwStorage, 数字格式的物理存储器类型信息
* Output     :
    * StorageText, 物理存储器信息以字符串格式输出在这块内存区域
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
* Description: 将数字格式的保护类型信息转换为字符串类型
* Input      :
    * len, ProtectText内存区域的字节数
    * dwProtect, 数字格式的保护类型类型信息
* Output     :
    * ProtectText, 保护类型信息以字符串格式输出在这块内存区域
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
* Description: 获取pVMQ对应内存区域的描述信息，映射文件……
* Input      :
    * len, RegionInfo可用的内存大小
    * hProcess, 进程句柄
    * pVMQ, 内存区域信息
* Output     :
    * RegionInfo, 描述信息以字符串格式输出在此内存区域
* Return     : 
* Others     : 
------------------------------------------------------------------------------*/
void GetRegionInfo(char *RegionInfo, size_t len, HANDLE hProcess, VMQUERY *pVMQ)
{
    if(pVMQ->bRgnIsAStack)
        strcpy(RegionInfo, "Thread Stack");
    if((pVMQ->dwRgnStorage != MEM_FREE) && (pVMQ->pvRgnBaseAddress != NULL)){
        MODULEENTRY32 me = { sizeof(me) };
        //获取pVMQ->pvRgnBaseAddress地址处的模块信息
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
* Description: 获取pvBaseAddr地址处的模块信息
* Input      :
    * pvBaseAddr, 地址
* Output     :
    * pme, 地址处的模块信息
* Return     : 
    * 是否在指定地址处找到模块
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
* Description: 重新构建最小堆
    * 0节点和左右两个节点比较，如果0节点比它们都小则堆特性保持
    * 若比左或右节点大，则将0节点与左(右)节点中最小的节点交换，并继续维护该子树
    * 依次执行，直到遍历完N，堆中保留的n个数就是N中最大的n个数
    * 小顶堆大概是这样的
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
    //如果没有孩子节点则不继续构建堆
    if(left >= N)
        return ;
    if(right >= N)       //只有左孩子节点
        pos = left;
    else                 //有两个节点则取两个节点中的较小值
        pos = freeRegionInfo[left].RgnSize > freeRegionInfo[right].RgnSize ? right : left;
    //如果新的根节点较大，则和两个子节点中的较小值交换，并向下继续维护新子树为小顶堆
    if(freeRegionInfo[root].RgnSize > freeRegionInfo[pos].RgnSize){
        //根节点放最小值
        tmpRegion = freeRegionInfo[root];
        freeRegionInfo[root] = freeRegionInfo[pos];
        freeRegionInfo[pos] = tmpRegion;
        //递归维护子树为小顶堆
        RebuildMinHeap(freeRegionInfo, N, pos);
    }
}

/*------------------------------------------------------------------------------
* Function   : HeapToSortArray
* Description: 将小顶堆结构的数组变成顺序数组
    * 将小顶堆结构的数组中元素按照从大到小从新排列
    * 交换堆顶元素和最后一个元素，此时最后一个位置作为有序区，然后将无序区调整为小顶堆
    * 然后继续将堆顶元素和倒数第二个元素交换，继续调整，直到循环完所有元素
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
