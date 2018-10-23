unit MainForm;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ProcMonitor;

type
  TForm1 = class(TForm)
    edtProcessId: TEdit;
    lblProcessId: TLabel;
    mmoProcessInfo: TMemo;
    btnGetProcessInfo: TButton;
    btnSaveProcessInfo: TButton;
    mmoProcessInfo2: TMemo;
    btnDelGetMemory: TButton;
    edtMemSize: TEdit;
    lblMemSize: TLabel;
    btnVirtualAllocReserve: TButton;
    btnVirtualAllocCommit: TButton;
    btnDelphiNew: TButton;
    btnTestGetProcessInfi: TButton;
    btnTestGetMaxNFreeRegionInfo: TButton;
    procedure FormCreate(Sender: TObject);
    procedure btnGetProcessInfoClick(Sender: TObject);
    procedure btnSaveProcessInfoClick(Sender: TObject);
    procedure btnTestGetProcessInfiClick(Sender: TObject);
    procedure btnTestGetMaxNFreeRegionInfoClick(Sender: TObject);
    procedure btnDelGetMemoryClick(Sender: TObject);
    procedure btnDelphiNewClick(Sender: TObject);
    procedure btnVirtualAllocReserveClick(Sender: TObject);
    procedure btnVirtualAllocCommitClick(Sender: TObject);
  private
    SaveTime: Integer;
  public
    { Public declarations }
  end;

  PRecord = ^TRecord;
  TRecord = record
    c: Char;
    i: Integer;
    d: Double;
    s: string;
  end;

var
  Form1: TForm1;

implementation

{$R *.dfm}

procedure TForm1.FormCreate(Sender: TObject);
begin
  Form1.Caption := Form1.Caption + '[' + IntToStr(GetCurrentProcessId) + ']';
  SaveTime := 0;
end;

procedure TForm1.btnGetProcessInfoClick(Sender: TObject);
var
  ProcessId: Cardinal;
  ProcessInfo: TProcessInfo;
  i: Integer;
  BaseAddress: Pointer;
  RegionSize: Double;
begin
  mmoProcessInfo.Clear();
  mmoProcessInfo2.Clear();
  ProcessId := StrToIntDef(edtProcessId.Text, 0);
  if (0 <> GetProcessInfo(@ProcessInfo, ProcessId)) then
  begin
    Application.MessageBox('获取进程信息出错', '错误', MB_ICONERROR);
    Exit;
  end;

  mmoProcessInfo.Lines.Add('可访问最高地址');        mmoProcessInfo2.Lines.Add(Format('%p', [ProcessInfo.MaximumApplicationAddress]));
  mmoProcessInfo.Lines.Add('可访问最低低至');        mmoProcessInfo2.Lines.Add(Format('%p', [ProcessInfo.MinimumApplicationAddress]));
  mmoProcessInfo.Lines.Add('内存页大小');            mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.PageSize));
  mmoProcessInfo.Lines.Add('预定地址空间分配粒度');  mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.AllocationGranularity));
  mmoProcessInfo.Lines.Add('');                      mmoProcessInfo2.Lines.Add('');

  mmoProcessInfo.Lines.Add('进程号');                mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.ProcessId));
  mmoProcessInfo.Lines.Add('缺页中断次数');          mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.PageFaultCount));
  mmoProcessInfo.Lines.Add('高峰工作集');            mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.PeakWorkingSetSize));
  mmoProcessInfo.Lines.Add('工作集');                mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.WorkingSetSize));
  mmoProcessInfo.Lines.Add('使用页面缓存池高峰');    mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.QuotaPeakPagedPoolUsage));
  mmoProcessInfo.Lines.Add('使用页面缓存池');        mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.QuotaPagedPoolUsage));
  mmoProcessInfo.Lines.Add('使用非分页缓存池高峰');  mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.QuotaPeakNonPagedPoolUsage));
  mmoProcessInfo.Lines.Add('使用非分页缓存池');      mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.QuotaNonPagedPoolUsage));
  mmoProcessInfo.Lines.Add('使用分页文件');          mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.PagefileUsage));
  mmoProcessInfo.Lines.Add('使用分页文件高峰');      mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.PeakPagefileUsage));
  mmoProcessInfo.Lines.Add('');                      mmoProcessInfo2.Lines.Add('');

  mmoProcessInfo.Lines.Add('堆内存个数');            mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.HeapCount));
  mmoProcessInfo.Lines.Add('');                      mmoProcessInfo2.Lines.Add('');
  
  for i:=0 to 9 do
  begin
    BaseAddress := ProcessInfo.ContinuousFreeMem[i].RgnBaseAddress;
    RegionSize := ProcessInfo.ContinuousFreeMem[i].RgnSize / 1024 / 1024;
    mmoProcessInfo.Lines.Add('空闲内存' + IntToStr(i+1));  mmoProcessInfo2.Lines.Add(Format('%p:%f', [BaseAddress, RegionSize]));
  end;
end;

procedure TForm1.btnSaveProcessInfoClick(Sender: TObject);
var
  ProcessId: Cardinal;
  HtmlPath:  string;
begin
  ProcessId := StrToIntDef(edtProcessId.Text, GetCurrentProcessId());
  Inc(SaveTime);
  HtmlPath := './ProcMonitor_' + IntToStr(ProcessId) + '_' + IntToStr(SaveTime) + '.html';

  if (0 <> SaveProcessInfo(PChar(HtmlPath), ProcessId)) then
  begin
    Application.MessageBox('保存进程信息出错', '错误', MB_ICONERROR);
    Exit;
  end;
  mmoProcessInfo.Lines.Add('第' + IntToStr(SaveTime) + '次保存内存信息');
  mmoProcessInfo2.Lines.Add('');
end;

procedure TForm1.btnTestGetProcessInfiClick(Sender: TObject);
var
  Success, Fail: Integer;
  ProcessId: Cardinal;
  time: Cardinal;
  ProcessInfo: TProcessInfo;
begin
  time := GetTickCount();
  Success := 0;
  Fail := 0;
  while True do
  begin
    ProcessId := StrToIntDef(edtProcessId.Text, 0);
    if 0 = GetProcessInfo(@ProcessInfo, ProcessId) then
    begin
      Inc(Success);
    end
    else
    begin
      Inc(Fail);
    end;
    if GetTickCount - time > 4 * 1000 then
    begin
      Break;
    end;
  end;
  mmoProcessInfo.Lines.Add(Format('4s GetProcessInfo成功%d次', [Success]));
  mmoProcessInfo2.Lines.Add(Format('4s GetProcessInfo失败%d次', [Fail]));
  mmoProcessInfo.Lines.Add('');                      mmoProcessInfo2.Lines.Add('');
end;

procedure TForm1.btnTestGetMaxNFreeRegionInfoClick(Sender: TObject);
var
  Success, Fail: Integer;
  ProcessId: Cardinal;
  time: Cardinal;
  FreeRegionInfo: array[0..9] of TFreeRegionInfo;
begin
  time := GetTickCount();
  Success := 0;
  Fail := 0;
  while True do
  begin
    ProcessId := StrToIntDef(edtProcessId.Text, 0);
    if 0 = GetMaxNFreeRegionInfo(@(FreeRegionInfo[0]), 10, ProcessId) then
    begin
      Inc(Success);
    end
    else
    begin
      Inc(Fail);
    end;
    if GetTickCount - time > 4 * 1000 then
    begin
      Break;
    end;
  end;
  mmoProcessInfo.Lines.Add(Format('4s GetMaxNFreeRegionInfo成功%d次', [Success]));
  mmoProcessInfo2.Lines.Add(Format('4s GetMaxNFreeRegionInfo失败%d次', [Fail]));
  mmoProcessInfo.Lines.Add('');                      mmoProcessInfo2.Lines.Add('');
end;

procedure TForm1.btnDelGetMemoryClick(Sender: TObject);
var
  Size: Integer;
  p: Pointer;
begin
  Size := StrToIntDef(edtMemSize.Text, 1);
  p := GetMemory(Size * 1024 * 1024);
  mmoProcessInfo.Lines.Add('(D)GetMemory：' + IntToStr(Size) + 'M');    mmoProcessInfo2.Lines.Add('地址是：' + Format('%p', [p]));
  mmoProcessInfo.Lines.Add('');                                         mmoProcessInfo2.Lines.Add('');
end;

procedure TForm1.btnDelphiNewClick(Sender: TObject);
var
  p: PRecord;
begin
  New(p);
  p.c := 'c';
  p.i := 100;
  p.d := 123.123;
  p.s := 'xumenger';
  mmoProcessInfo.Lines.Add('(D)Record New(PRecord)');    mmoProcessInfo2.Lines.Add('地址是：' + Format('%p', [p]));
  mmoProcessInfo.Lines.Add('(D)PRecord.c');    mmoProcessInfo2.Lines.Add('地址是：' + Format('%p', [@p.c]));
  mmoProcessInfo.Lines.Add('(D)PRecord.i');    mmoProcessInfo2.Lines.Add('地址是：' + Format('%p', [@p.i]));
  mmoProcessInfo.Lines.Add('(D)PRecord.d');    mmoProcessInfo2.Lines.Add('地址是：' + Format('%p', [@p.d]));
  mmoProcessInfo.Lines.Add('(D)PRecord.s');    mmoProcessInfo2.Lines.Add('地址是：' + Format('%p', [@p.s]));
  mmoProcessInfo.Lines.Add('');                mmoProcessInfo2.Lines.Add('');
end;

procedure TForm1.btnVirtualAllocReserveClick(Sender: TObject);
var
  Size: Integer;
  p: Pointer;
begin
  Size := StrToIntDef(edtMemSize.Text, 1);
  //在进程任意地方预订虚拟内存
  p := VirtualAlloc(nil, Size * 1024 * 1024, MEM_RESERVE, PAGE_READWRITE);
  mmoProcessInfo.Lines.Add('(D)VirtualAlloc Reserve：' + IntToStr(Size) + 'M');    mmoProcessInfo2.Lines.Add('地址是：' + Format('%p', [p]));
  mmoProcessInfo.Lines.Add('');                                                    mmoProcessInfo2.Lines.Add('');
end;

procedure TForm1.btnVirtualAllocCommitClick(Sender: TObject);
var
  Size: Integer;
  p: Pointer;
begin
  Size := StrToIntDef(edtMemSize.Text, 1);
  //给区域调拨物理存储器
  p := VirtualAlloc(nil, Size * 1024 * 1024, MEM_COMMIT, PAGE_READWRITE);
  mmoProcessInfo.Lines.Add('(D)VirtualAlloc Commit：' + IntToStr(Size) + 'M');    mmoProcessInfo2.Lines.Add('地址是：' + Format('%p', [p]));
  mmoProcessInfo.Lines.Add('');                                                   mmoProcessInfo2.Lines.Add('');
end;

end.
