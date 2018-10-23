#ifndef PROC_MONITOR_H
#define PROC_MONITOR_H

typedef struct{
    PVOID  RgnBaseAddress;     //对应内存区域基地址
    SIZE_T RgnSize;              //对应内存区域大小
}FreeRegionInfo;

//进程信息接口体定义
typedef struct{
    unsigned long ProcessId;          //进程ID
    //操作系统信息
    LPVOID MinimumApplicationAddress; //程序可访问的最低内存地址
    LPVOID MaximumApplicationAddress; //程序可访问的最高内存地址
    int PageSize;                     //内存页大小
    DWORD AllocationGranularity;      //用于预定地址空间区域的分配粒度
	//Psapi内存信息
	DWORD PageFaultCount;             //缺页中断次数
    DWORD PeakWorkingSetSize;         //使用内存高峰（高峰工作集）
    DWORD WorkingSetSize;             //当前使用的内存（工作集）
    DWORD QuotaPeakPagedPoolUsage;    //使用页面缓存池高峰
    DWORD QuotaPagedPoolUsage;        //使用页面缓存池
    DWORD QuotaPeakNonPagedPoolUsage; //使用非分页缓存池高峰
    DWORD QuotaNonPagedPoolUsage;     //使用非分页缓存池
    DWORD PagefileUsage;              //使用分页文件
    DWORD PeakPagefileUsage;          //使用分页文件高峰
    //堆内存信息
	int HeapCount;                    //堆个数
	FreeRegionInfo ContinuousFreeMem[10];
}ProcessInfo;

int __stdcall GetProcessInfo(ProcessInfo *processInfo, unsigned long processId = 0);
int __stdcall SaveProcessInfo(const char *path, unsigned long processId = 0);
int __stdcall GetMaxNFreeRegionInfo(FreeRegionInfo *freeRegionInfo, int N, unsigned long processId = 0);

#endif