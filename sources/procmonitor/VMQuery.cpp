#include <windowsx.h>
#include<Windows.h>
#include "VMQuery.h"

//Helper structure
typedef struct{
    SIZE_T RgnSize;              //�ڴ�����Ĵ�С
    DWORD  dwRgnStorage;         //�ڴ����������洢�����ͣ�MEM_*: Free, Image, Mapped, Private
    DWORD  dwRgnBlocks;          //�ڴ��������ڴ��ĸ���
    DWORD  dwRgnGuardBlks;       //��Ӧ�ڴ������о���PAGE_GUARD�������Ա�־�Ŀ����������Ϊ1��ʾ��������Ϊ���߳�ջ��Ԥ����
    BOOL   bRgnIsAStack;         //��Ӧ�ڴ������Ƿ�����߳�ջ����ֵ��ͨ�����Ʋ²�õ���
}VMQUERY_HELP;

//ȫ�ֱ��������ڿ���ֻ��Ҫ����һ��GetSystemInfo�ӿ�
static DWORD gs_dwAllocGran = 0;

/*------------------------------------------------------------------------------
* Function   : VMQueryHelp
* Description: ѭ���ڴ���������п���ܵõ��ڴ��������Ϣ
* Input      :
    * hProcess, ���̾��
    * pvAddress, ָ���ڴ������е�һ���ڴ��ַ
* Output     :
    * pVMQHelp, �ڴ�������Ϣ
* Return     : 
    * �Ƿ�ɹ���ȡ�ڴ�������Ϣ
* Others     : 
------------------------------------------------------------------------------*/
static BOOL VMQueryHelp(HANDLE hProcess, LPCVOID pvAddress, VMQUERY_HELP *pVMQHelp)
{   
    ZeroMemory(pVMQHelp, sizeof(*pVMQHelp));
    /*
    DWORD VirtualQueryEx(
        HANDLE hProcess,
        LPCVOID pvAddress,
        PMEMORY_BASIC_INFORMATION pmbi,
        DWORD dwLength);

    typedef struct _MEMORY_BASIC_INFORMATION{
	    PVOID BaseAddress;        //���ڽ�����pvAdress����ȡ����ҳ��Ĵ�С
	    PVOID AllocationBase;     //����Ļ���ַ���������������pvAddress��ָ���ĵ�ַ
	    DWORD AllocationProtect;  //���ʼԤ������ʱΪ������ָ���ı�������
	    SIZE_T RegionSize;        //�飨��Ȼ����ΪRegionSize���Ĵ�С(Byte)���������ʼ��ַ��BaseAddress������������ҳ������ͬ�ı������ԡ�״̬������
	    DWORD State;              //������ҳ�棨�ڴ�飩��״̬(MEM_FREE��MEM_RESERVE��MEM_COMMIT)
	    DWORD Protect;            //����������ڵ�ҳ�棬��ʾ���ǵı�������(PAGE_*)
	    DWORD Type;               //������ҳ�棨�ڴ�飩������(MEM_IMAGE��MEM_MAPPED��MEM_PRIVATE)
    } MEMORY_BASIC_INFORMATION, * PMEMORY_BASIC_INFORMATION;
    */
    MEMORY_BASIC_INFORMATION mbi;
    BOOL bOk = (VirtualQueryEx(hProcess, pvAddress, &mbi, sizeof(mbi)) == sizeof(mbi));
    if(!bOk)
        return bOk;

    //��ȡ�ڴ�����Ļ���ַ
    PVOID pvRgnBaseAddress = mbi.AllocationBase;
    PVOID pvAddressBlk = pvRgnBaseAddress;

    //�����ڴ����������洢������
    pVMQHelp->dwRgnStorage = mbi.Type;

    for(;;){
        //����VirtualQueryEx��ȡ��Ӧ�ڴ����Ϣ
        bOk = (VirtualQueryEx(hProcess, pvAddressBlk, &mbi, sizeof(mbi)) == sizeof(mbi));
        if(!bOk)
            break;

        //�жϵ�ǰ���Ƿ���ͬһ�ڴ�����������˵��ѭ������ڴ������˳�ѭ��
        if(mbi.AllocationBase != pvRgnBaseAddress)
            break;

        //�õ����ڴ������е�һ���µ��ڴ����Ϣ
        pVMQHelp->dwRgnBlocks++;
        pVMQHelp->RgnSize += mbi.RegionSize;

        //����ÿ���PAGE_GUARD���ԣ���dwRgnGuardBlks��������һ
        if((mbi.Protect & PAGE_GUARD) == PAGE_GUARD)
            pVMQHelp->dwRgnGuardBlks++;

        //Take a best guess as to the type of physical storage commited to the
        //block. This is a guess because some blocks can convert from MEM_IMAGE
        //to MEM_PRIVATE or from MEM_MAPPED to MEM_PRIVATE; MEM_PRIVATE can
        //always be overridden by MEM_IMAGE or MEM_MAPPED
        if(pVMQHelp->dwRgnStorage == MEM_PRIVATE)
            pVMQHelp->dwRgnStorage = mbi.Type;

        //��ȡ��һ���ڴ��ĵ�ַ��Ϣ
        pvAddressBlk = (PVOID) ((PBYTE) pvAddressBlk + mbi.RegionSize);
    }

    //ѭ������ڴ�����Ŀ���Ϣ�󣬸���dwRgnGuardBlks��ֵ�ж����Ƿ���ջ��
    pVMQHelp->bRgnIsAStack = (pVMQHelp->dwRgnGuardBlks > 0);

    return TRUE;
}

/*------------------------------------------------------------------------------
* Function   : VMQuery
* Description: ��ȡpvAddress�����ڴ������ڴ�����Ϣ 
* Input      :
    * hProcess, ���̾��
    * pvAddress, ָ���ڴ������е�һ���ڴ��ַ
* Output     :
    * pVMQ, �ڴ������ڴ����Ϣ
* Return     : 
    * �Ƿ�ɹ���ȡ�ڴ������ڴ�������Ϣ
* Others     : 
------------------------------------------------------------------------------*/
BOOL VMQuery(HANDLE hProcess, LPCVOID pvAddress, PVMQUERY pVMQ)
{
    //GetSustemInfo����Ϣ�ڽ��������ڼ䲻��䣬���Կ���ֻ����һ�μ���
    if(gs_dwAllocGran == 0){
        SYSTEM_INFO sinf;
        GetSystemInfo(&sinf);
        gs_dwAllocGran = sinf.dwAllocationGranularity;
    }

    ZeroMemory(pVMQ, sizeof(*pVMQ));

    //Get the MEMORY_BASIC_INFORMATION for the passed address
    MEMORY_BASIC_INFORMATION mbi;
    BOOL bOk = (VirtualQueryEx(hProcess, pvAddress, &mbi, sizeof(mbi)) == sizeof(mbi));
    if(!bOk)
        return bOk;

    //The MEMORY_BASIC_INFORMATION structure contains valid information
    //Time to start setting the members of our owm VMQUERY structure

    //���Ȼ�ȡ�õ�ַ��Ӧ���ڴ����Ϣ������������ȥ��ȡ�ڴ�������Ϣ
    switch(mbi.State){
        //Free block (not reserved)
        case MEM_FREE:
            pVMQ->pvBlkBaseAddress = NULL;
            pVMQ->BlkSize = 0;
            pVMQ->dwBlkProtection = 0;
            pVMQ->dwBlkStorage = MEM_FREE;
            break;

        //Reserved block without commited storage in it
        case MEM_RESERVE:
            pVMQ->pvBlkBaseAddress = mbi.BaseAddress;
            pVMQ->BlkSize = mbi.RegionSize;
            //����һ��û��Commit���ڴ�飬mbi.Protect�ǲ��Ϸ���
            //��������������ڴ���Ӧ������ı���������Ϊ�䱣������
            pVMQ->dwBlkProtection = mbi.AllocationProtect;  
            pVMQ->dwBlkStorage = MEM_RESERVE;
            break;

        //Reserved block with committed storage in it
        case MEM_COMMIT:
            pVMQ->pvBlkBaseAddress = mbi.BaseAddress;
            pVMQ->BlkSize = mbi.RegionSize;
            pVMQ->dwBlkProtection = mbi.Protect;
            pVMQ->dwBlkStorage = mbi.Type;
            break;

        default:
            DebugBreak();
            break;
    }

    //���ڻ�ȡ��Ӧ���ڴ��������Ϣ
    VMQUERY_HELP VMQHelp;
    switch(mbi.State){
        case MEM_FREE:
            pVMQ->pvRgnBaseAddress = mbi.BaseAddress;
            pVMQ->dwRgnProtection = mbi.AllocationProtect;
            pVMQ->RgnSize = mbi.RegionSize;
            pVMQ->dwRgnStorage = MEM_FREE;
            pVMQ->dwRgnBlocks = 0;
            pVMQ->dwRgnGuardBlks = 0;
            pVMQ->bRgnIsAStack = FALSE;;
            break;
            
        case MEM_RESERVE:
            pVMQ->pvRgnBaseAddress = mbi.AllocationBase;
            pVMQ->dwRgnProtection = mbi.AllocationProtect;
            
            //Iterate through all blocks to get complete region information
            VMQueryHelp(hProcess, pvAddress, &VMQHelp);

            pVMQ->RgnSize = VMQHelp.RgnSize;
            pVMQ->dwRgnStorage = VMQHelp.dwRgnStorage;
            pVMQ->dwRgnBlocks = VMQHelp.dwRgnBlocks;
            pVMQ->dwRgnGuardBlks = VMQHelp.dwRgnGuardBlks;
            pVMQ->bRgnIsAStack = VMQHelp.bRgnIsAStack;
            break;

        case MEM_COMMIT:
            pVMQ->pvRgnBaseAddress = mbi.AllocationBase;
            pVMQ->dwRgnProtection = mbi.AllocationProtect;
            
            VMQueryHelp(hProcess, pvAddress, &VMQHelp);

            pVMQ->RgnSize = VMQHelp.RgnSize;
            pVMQ->dwRgnStorage = VMQHelp.dwRgnStorage;
            pVMQ->dwRgnBlocks = VMQHelp.dwRgnBlocks;
            pVMQ->dwRgnGuardBlks = VMQHelp.dwRgnGuardBlks;
            pVMQ->bRgnIsAStack = VMQHelp.bRgnIsAStack;
            break;

        default:
            DebugBreak();
            break;
    }
    return bOk;
}
