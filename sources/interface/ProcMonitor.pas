unit ProcMonitor;

interface

const
  PROC_MONITOR_DLL = 'procmonitor.dll';

type
  PFreeRegionInfo = ^TFreeRegionInfo;
  TFreeRegionInfo = record
    RgnBaseAddress: Pointer;
    RgnSize: Cardinal;
  end;
  
  PProcessInfo = ^TProcessInfo;
  TProcessInfo = record
    ProcessId: Cardinal;                         //进程ID
    //SystemInfo
    MinimumApplicationAddress: Pointer;          //程序可访问的最低内存地址
    MaximumApplicationAddress: Pointer;          //程序可访问的最高内存地址
    PageSize: Integer;                           //内存页大小
    AllocationGranularity: Cardinal;             //用于预定地址空间区域的分配粒度

    //Psapi
    PageFaultCount: Cardinal;                    //缺页中断次数
    PeakWorkingSetSize: Cardinal;                //使用内存高峰（高峰工作集）
    WorkingSetSize: Cardinal;                    //当前使用的内存（工作集）
	  QuotaPeakPagedPoolUsage: Cardinal;           //使用页面缓存池高峰
    QuotaPagedPoolUsage: Cardinal;               //使用页面缓存池
    QuotaPeakNonPagedPoolUsage: Cardinal;        //使用非分页缓存池高峰
    QuotaNonPagedPoolUsage: Cardinal;            //使用非分页缓存池
    PagefileUsage: Cardinal;                     //使用分页文件
    PeakPagefileUsage: Cardinal;                 //使用分页文件高峰

    HeapCount: Integer;                          //堆内存个数
    
    ContinuousFreeMem: array[0..9] of TFreeRegionInfo; 
  end;

function GetProcessInfo(processInfo: PProcessInfo; ProcessId: Cardinal = 0): Integer; stdcall; external PROC_MONITOR_DLL;
function SaveProcessInfo(path: PChar; processId: Cardinal = 0): Integer; stdcall; external PROC_MONITOR_DLL;
function GetMaxNFreeRegionInfo(freeRegionInfo: PFreeRegionInfo; N: Integer; ProcessId: Cardinal = 0): Integer;  stdcall; external PROC_MONITOR_DLL;

implementation

end.
