#include <windowsx.h>
#include<Windows.h>
#include "VMQuery.h"

//Helper structure
typedef struct{
    SIZE_T RgnSize;              //内存区域的大小
    DWORD  dwRgnStorage;         //内存区域的物理存储器类型，MEM_*: Free, Image, Mapped, Private
    DWORD  dwRgnBlocks;          //内存区域下内存块的个数
    DWORD  dwRgnGuardBlks;       //对应内存区域中具有PAGE_GUARD保护属性标志的块的数量，若为1表示该区域是为了线程栈而预订的
    BOOL   bRgnIsAStack;         //对应内存区域是否包含线程栈。该值是通过近似猜测得到的
}VMQUERY_HELP;

//全局变量，用于控制只需要调用一次GetSystemInfo接口
static DWORD gs_dwAllocGran = 0;

/*------------------------------------------------------------------------------
* Function   : VMQueryHelp
* Description: 循环内存区域的所有块汇总得到内存区域的信息
* Input      :
    * hProcess, 进程句柄
    * pvAddress, 指定内存区域中的一个内存地址
* Output     :
    * pVMQHelp, 内存区域信息
* Return     : 
    * 是否成功获取内存区域信息
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
	    PVOID BaseAddress;        //等于将参数pvAdress向下取整到页面的大小
	    PVOID AllocationBase;     //区域的基地址，该区域包含参数pvAddress所指定的地址
	    DWORD AllocationProtect;  //在最开始预定区域时为该区域指定的保护属性
	    SIZE_T RegionSize;        //块（虽然命名为RegionSize）的大小(Byte)。区域的起始地址是BaseAddress，区域中所有页面有相同的保护属性、状态和类型
	    DWORD State;              //区域中页面（内存块）的状态(MEM_FREE、MEM_RESERVE、MEM_COMMIT)
	    DWORD Protect;            //针对所有相邻的页面，表示它们的保护属性(PAGE_*)
	    DWORD Type;               //区域中页面（内存块）的类型(MEM_IMAGE、MEM_MAPPED、MEM_PRIVATE)
    } MEMORY_BASIC_INFORMATION, * PMEMORY_BASIC_INFORMATION;
    */
    MEMORY_BASIC_INFORMATION mbi;
    BOOL bOk = (VirtualQueryEx(hProcess, pvAddress, &mbi, sizeof(mbi)) == sizeof(mbi));
    if(!bOk)
        return bOk;

    //获取内存区域的基地址
    PVOID pvRgnBaseAddress = mbi.AllocationBase;
    PVOID pvAddressBlk = pvRgnBaseAddress;

    //保存内存区域的物理存储器类型
    pVMQHelp->dwRgnStorage = mbi.Type;

    for(;;){
        //调用VirtualQueryEx获取对应内存块信息
        bOk = (VirtualQueryEx(hProcess, pvAddressBlk, &mbi, sizeof(mbi)) == sizeof(mbi));
        if(!bOk)
            break;

        //判断当前块是否在同一内存区域，若不是说明循环完该内存区域，退出循环
        if(mbi.AllocationBase != pvRgnBaseAddress)
            break;

        //得到该内存区域中的一个新的内存块信息
        pVMQHelp->dwRgnBlocks++;
        pVMQHelp->RgnSize += mbi.RegionSize;

        //如果该块有PAGE_GUARD属性，将dwRgnGuardBlks计数器加一
        if((mbi.Protect & PAGE_GUARD) == PAGE_GUARD)
            pVMQHelp->dwRgnGuardBlks++;

        //Take a best guess as to the type of physical storage commited to the
        //block. This is a guess because some blocks can convert from MEM_IMAGE
        //to MEM_PRIVATE or from MEM_MAPPED to MEM_PRIVATE; MEM_PRIVATE can
        //always be overridden by MEM_IMAGE or MEM_MAPPED
        if(pVMQHelp->dwRgnStorage == MEM_PRIVATE)
            pVMQHelp->dwRgnStorage = mbi.Type;

        //获取下一个内存块的地址信息
        pvAddressBlk = (PVOID) ((PBYTE) pvAddressBlk + mbi.RegionSize);
    }

    //循环完该内存区域的块信息后，根据dwRgnGuardBlks的值判断其是否是栈区
    pVMQHelp->bRgnIsAStack = (pVMQHelp->dwRgnGuardBlks > 0);

    return TRUE;
}

/*------------------------------------------------------------------------------
* Function   : VMQuery
* Description: 获取pvAddress所在内存区域、内存块的信息 
* Input      :
    * hProcess, 进程句柄
    * pvAddress, 指定内存区域中的一个内存地址
* Output     :
    * pVMQ, 内存区域、内存块信息
* Return     : 
    * 是否成功获取内存区域、内存区域信息
* Others     : 
------------------------------------------------------------------------------*/
BOOL VMQuery(HANDLE hProcess, LPCVOID pvAddress, PVMQUERY pVMQ)
{
    //GetSustemInfo的信息在进程运行期间不会变，所以控制只调用一次即可
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

    //首先获取该地址对应的内存块信息，后面我们再去获取内存区域信息
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
            //对于一块没有Commit的内存块，mbi.Protect是不合法的
            //所以我们用这个内存块对应的区域的保护属性作为其保护属性
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

    //现在获取对应的内存区域的信息
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
