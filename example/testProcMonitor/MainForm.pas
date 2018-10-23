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
    Application.MessageBox('��ȡ������Ϣ����', '����', MB_ICONERROR);
    Exit;
  end;

  mmoProcessInfo.Lines.Add('�ɷ�����ߵ�ַ');        mmoProcessInfo2.Lines.Add(Format('%p', [ProcessInfo.MaximumApplicationAddress]));
  mmoProcessInfo.Lines.Add('�ɷ�����͵���');        mmoProcessInfo2.Lines.Add(Format('%p', [ProcessInfo.MinimumApplicationAddress]));
  mmoProcessInfo.Lines.Add('�ڴ�ҳ��С');            mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.PageSize));
  mmoProcessInfo.Lines.Add('Ԥ����ַ�ռ��������');  mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.AllocationGranularity));
  mmoProcessInfo.Lines.Add('');                      mmoProcessInfo2.Lines.Add('');

  mmoProcessInfo.Lines.Add('���̺�');                mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.ProcessId));
  mmoProcessInfo.Lines.Add('ȱҳ�жϴ���');          mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.PageFaultCount));
  mmoProcessInfo.Lines.Add('�߷幤����');            mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.PeakWorkingSetSize));
  mmoProcessInfo.Lines.Add('������');                mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.WorkingSetSize));
  mmoProcessInfo.Lines.Add('ʹ��ҳ�滺��ظ߷�');    mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.QuotaPeakPagedPoolUsage));
  mmoProcessInfo.Lines.Add('ʹ��ҳ�滺���');        mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.QuotaPagedPoolUsage));
  mmoProcessInfo.Lines.Add('ʹ�÷Ƿ�ҳ����ظ߷�');  mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.QuotaPeakNonPagedPoolUsage));
  mmoProcessInfo.Lines.Add('ʹ�÷Ƿ�ҳ�����');      mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.QuotaNonPagedPoolUsage));
  mmoProcessInfo.Lines.Add('ʹ�÷�ҳ�ļ�');          mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.PagefileUsage));
  mmoProcessInfo.Lines.Add('ʹ�÷�ҳ�ļ��߷�');      mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.PeakPagefileUsage));
  mmoProcessInfo.Lines.Add('');                      mmoProcessInfo2.Lines.Add('');

  mmoProcessInfo.Lines.Add('���ڴ����');            mmoProcessInfo2.Lines.Add(IntToStr(ProcessInfo.HeapCount));
  mmoProcessInfo.Lines.Add('');                      mmoProcessInfo2.Lines.Add('');
  
  for i:=0 to 9 do
  begin
    BaseAddress := ProcessInfo.ContinuousFreeMem[i].RgnBaseAddress;
    RegionSize := ProcessInfo.ContinuousFreeMem[i].RgnSize / 1024 / 1024;
    mmoProcessInfo.Lines.Add('�����ڴ�' + IntToStr(i+1));  mmoProcessInfo2.Lines.Add(Format('%p:%f', [BaseAddress, RegionSize]));
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
    Application.MessageBox('���������Ϣ����', '����', MB_ICONERROR);
    Exit;
  end;
  mmoProcessInfo.Lines.Add('��' + IntToStr(SaveTime) + '�α����ڴ���Ϣ');
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
  mmoProcessInfo.Lines.Add(Format('4s GetProcessInfo�ɹ�%d��', [Success]));
  mmoProcessInfo2.Lines.Add(Format('4s GetProcessInfoʧ��%d��', [Fail]));
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
  mmoProcessInfo.Lines.Add(Format('4s GetMaxNFreeRegionInfo�ɹ�%d��', [Success]));
  mmoProcessInfo2.Lines.Add(Format('4s GetMaxNFreeRegionInfoʧ��%d��', [Fail]));
  mmoProcessInfo.Lines.Add('');                      mmoProcessInfo2.Lines.Add('');
end;

procedure TForm1.btnDelGetMemoryClick(Sender: TObject);
var
  Size: Integer;
  p: Pointer;
begin
  Size := StrToIntDef(edtMemSize.Text, 1);
  p := GetMemory(Size * 1024 * 1024);
  mmoProcessInfo.Lines.Add('(D)GetMemory��' + IntToStr(Size) + 'M');    mmoProcessInfo2.Lines.Add('��ַ�ǣ�' + Format('%p', [p]));
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
  mmoProcessInfo.Lines.Add('(D)Record New(PRecord)');    mmoProcessInfo2.Lines.Add('��ַ�ǣ�' + Format('%p', [p]));
  mmoProcessInfo.Lines.Add('(D)PRecord.c');    mmoProcessInfo2.Lines.Add('��ַ�ǣ�' + Format('%p', [@p.c]));
  mmoProcessInfo.Lines.Add('(D)PRecord.i');    mmoProcessInfo2.Lines.Add('��ַ�ǣ�' + Format('%p', [@p.i]));
  mmoProcessInfo.Lines.Add('(D)PRecord.d');    mmoProcessInfo2.Lines.Add('��ַ�ǣ�' + Format('%p', [@p.d]));
  mmoProcessInfo.Lines.Add('(D)PRecord.s');    mmoProcessInfo2.Lines.Add('��ַ�ǣ�' + Format('%p', [@p.s]));
  mmoProcessInfo.Lines.Add('');                mmoProcessInfo2.Lines.Add('');
end;

procedure TForm1.btnVirtualAllocReserveClick(Sender: TObject);
var
  Size: Integer;
  p: Pointer;
begin
  Size := StrToIntDef(edtMemSize.Text, 1);
  //�ڽ�������ط�Ԥ�������ڴ�
  p := VirtualAlloc(nil, Size * 1024 * 1024, MEM_RESERVE, PAGE_READWRITE);
  mmoProcessInfo.Lines.Add('(D)VirtualAlloc Reserve��' + IntToStr(Size) + 'M');    mmoProcessInfo2.Lines.Add('��ַ�ǣ�' + Format('%p', [p]));
  mmoProcessInfo.Lines.Add('');                                                    mmoProcessInfo2.Lines.Add('');
end;

procedure TForm1.btnVirtualAllocCommitClick(Sender: TObject);
var
  Size: Integer;
  p: Pointer;
begin
  Size := StrToIntDef(edtMemSize.Text, 1);
  //�������������洢��
  p := VirtualAlloc(nil, Size * 1024 * 1024, MEM_COMMIT, PAGE_READWRITE);
  mmoProcessInfo.Lines.Add('(D)VirtualAlloc Commit��' + IntToStr(Size) + 'M');    mmoProcessInfo2.Lines.Add('��ַ�ǣ�' + Format('%p', [p]));
  mmoProcessInfo.Lines.Add('');                                                   mmoProcessInfo2.Lines.Add('');
end;

end.
