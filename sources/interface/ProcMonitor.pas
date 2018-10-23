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
    ProcessId: Cardinal;                         //����ID
    //SystemInfo
    MinimumApplicationAddress: Pointer;          //����ɷ��ʵ�����ڴ��ַ
    MaximumApplicationAddress: Pointer;          //����ɷ��ʵ�����ڴ��ַ
    PageSize: Integer;                           //�ڴ�ҳ��С
    AllocationGranularity: Cardinal;             //����Ԥ����ַ�ռ�����ķ�������

    //Psapi
    PageFaultCount: Cardinal;                    //ȱҳ�жϴ���
    PeakWorkingSetSize: Cardinal;                //ʹ���ڴ�߷壨�߷幤������
    WorkingSetSize: Cardinal;                    //��ǰʹ�õ��ڴ棨��������
	  QuotaPeakPagedPoolUsage: Cardinal;           //ʹ��ҳ�滺��ظ߷�
    QuotaPagedPoolUsage: Cardinal;               //ʹ��ҳ�滺���
    QuotaPeakNonPagedPoolUsage: Cardinal;        //ʹ�÷Ƿ�ҳ����ظ߷�
    QuotaNonPagedPoolUsage: Cardinal;            //ʹ�÷Ƿ�ҳ�����
    PagefileUsage: Cardinal;                     //ʹ�÷�ҳ�ļ�
    PeakPagefileUsage: Cardinal;                 //ʹ�÷�ҳ�ļ��߷�

    HeapCount: Integer;                          //���ڴ����
    
    ContinuousFreeMem: array[0..9] of TFreeRegionInfo; 
  end;

function GetProcessInfo(processInfo: PProcessInfo; ProcessId: Cardinal = 0): Integer; stdcall; external PROC_MONITOR_DLL;
function SaveProcessInfo(path: PChar; processId: Cardinal = 0): Integer; stdcall; external PROC_MONITOR_DLL;
function GetMaxNFreeRegionInfo(freeRegionInfo: PFreeRegionInfo; N: Integer; ProcessId: Cardinal = 0): Integer;  stdcall; external PROC_MONITOR_DLL;

implementation

end.
