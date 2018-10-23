#ifndef PROC_MONITOR_H
#define PROC_MONITOR_H

typedef struct{
    PVOID  RgnBaseAddress;     //��Ӧ�ڴ��������ַ
    SIZE_T RgnSize;              //��Ӧ�ڴ������С
}FreeRegionInfo;

//������Ϣ�ӿ��嶨��
typedef struct{
    unsigned long ProcessId;          //����ID
    //����ϵͳ��Ϣ
    LPVOID MinimumApplicationAddress; //����ɷ��ʵ�����ڴ��ַ
    LPVOID MaximumApplicationAddress; //����ɷ��ʵ�����ڴ��ַ
    int PageSize;                     //�ڴ�ҳ��С
    DWORD AllocationGranularity;      //����Ԥ����ַ�ռ�����ķ�������
	//Psapi�ڴ���Ϣ
	DWORD PageFaultCount;             //ȱҳ�жϴ���
    DWORD PeakWorkingSetSize;         //ʹ���ڴ�߷壨�߷幤������
    DWORD WorkingSetSize;             //��ǰʹ�õ��ڴ棨��������
    DWORD QuotaPeakPagedPoolUsage;    //ʹ��ҳ�滺��ظ߷�
    DWORD QuotaPagedPoolUsage;        //ʹ��ҳ�滺���
    DWORD QuotaPeakNonPagedPoolUsage; //ʹ�÷Ƿ�ҳ����ظ߷�
    DWORD QuotaNonPagedPoolUsage;     //ʹ�÷Ƿ�ҳ�����
    DWORD PagefileUsage;              //ʹ�÷�ҳ�ļ�
    DWORD PeakPagefileUsage;          //ʹ�÷�ҳ�ļ��߷�
    //���ڴ���Ϣ
	int HeapCount;                    //�Ѹ���
	FreeRegionInfo ContinuousFreeMem[10];
}ProcessInfo;

int __stdcall GetProcessInfo(ProcessInfo *processInfo, unsigned long processId = 0);
int __stdcall SaveProcessInfo(const char *path, unsigned long processId = 0);
int __stdcall GetMaxNFreeRegionInfo(FreeRegionInfo *freeRegionInfo, int N, unsigned long processId = 0);

#endif