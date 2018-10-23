object Form1: TForm1
  Left = 468
  Top = 46
  BorderStyle = bsDialog
  Caption = 'ProcMonitor'
  ClientHeight = 455
  ClientWidth = 654
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object lblProcessId: TLabel
    Left = 13
    Top = 22
    Width = 46
    Height = 13
    AutoSize = False
    Caption = #36827#31243'ID'#65306
  end
  object lblMemSize: TLabel
    Left = 8
    Top = 198
    Width = 60
    Height = 13
    AutoSize = False
    Caption = #20869#23384#22823#23567#65306
  end
  object edtProcessId: TEdit
    Left = 76
    Top = 18
    Width = 121
    Height = 21
    AutoSize = False
    ImeName = #20013#25991'('#31616#20307') - '#25628#29399#25340#38899#36755#20837#27861
    TabOrder = 0
  end
  object mmoProcessInfo: TMemo
    Left = 215
    Top = 0
    Width = 233
    Height = 454
    ImeName = #20013#25991'('#31616#20307') - '#25628#29399#25340#38899#36755#20837#27861
    TabOrder = 10
  end
  object btnGetProcessInfo: TButton
    Left = 76
    Top = 57
    Width = 132
    Height = 25
    Caption = #33719#21462#36827#31243#20449#24687
    TabOrder = 1
    OnClick = btnGetProcessInfoClick
  end
  object btnSaveProcessInfo: TButton
    Left = 76
    Top = 89
    Width = 132
    Height = 25
    Caption = #20445#23384#36827#31243#20449#24687
    TabOrder = 2
    OnClick = btnSaveProcessInfoClick
  end
  object mmoProcessInfo2: TMemo
    Left = 448
    Top = 0
    Width = 206
    Height = 454
    ImeName = #20013#25991'('#31616#20307') - '#25628#29399#25340#38899#36755#20837#27861
    TabOrder = 11
  end
  object btnDelGetMemory: TButton
    Left = 32
    Top = 229
    Width = 157
    Height = 25
    Caption = 'Delphi GetMemory'
    TabOrder = 6
    OnClick = btnDelGetMemoryClick
  end
  object edtMemSize: TEdit
    Left = 76
    Top = 195
    Width = 121
    Height = 21
    ImeName = #20013#25991'('#31616#20307') - '#25628#29399#25340#38899#36755#20837#27861
    TabOrder = 5
  end
  object btnVirtualAllocReserve: TButton
    Left = 32
    Top = 300
    Width = 157
    Height = 25
    Caption = 'Delphi VirtualAlloc Reserve'
    TabOrder = 8
    OnClick = btnVirtualAllocReserveClick
  end
  object btnVirtualAllocCommit: TButton
    Left = 32
    Top = 337
    Width = 157
    Height = 25
    Caption = 'Delphi VirtualAlloc Commit'
    TabOrder = 9
    OnClick = btnVirtualAllocCommitClick
  end
  object btnDelphiNew: TButton
    Left = 32
    Top = 265
    Width = 157
    Height = 25
    Caption = 'Delphi Record New'
    TabOrder = 7
    OnClick = btnDelphiNewClick
  end
  object btnTestGetProcessInfi: TButton
    Left = 76
    Top = 121
    Width = 132
    Height = 25
    Caption = 'GetProcessInfo'
    TabOrder = 3
    OnClick = btnTestGetProcessInfiClick
  end
  object btnTestGetMaxNFreeRegionInfo: TButton
    Left = 76
    Top = 154
    Width = 132
    Height = 25
    Caption = 'GetMaxNFreeRegionInfo'
    TabOrder = 4
    OnClick = btnTestGetMaxNFreeRegionInfoClick
  end
end
