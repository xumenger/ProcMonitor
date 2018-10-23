program testProcMonitor;

uses
  Forms,
  MainForm in 'MainForm.pas' {Form1},
  ProcMonitor in '..\..\sources\interface\ProcMonitor.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
