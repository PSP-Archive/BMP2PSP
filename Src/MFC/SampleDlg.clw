; CLW ファイルは MFC ClassWizard の情報を含んでいます。

[General Info]
Version=1
LastClass=CSampleDlgDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "SampleDlg.h"

ClassCount=3
Class1=CSampleDlgApp
Class2=CSampleDlgDlg
Class3=CAboutDlg

ResourceCount=4
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Resource3=IDD_SAMPLEDLG_DIALOG
Resource4=IDR_MENU1

[CLS:CSampleDlgApp]
Type=0
HeaderFile=SampleDlg.h
ImplementationFile=SampleDlg.cpp
Filter=N
BaseClass=CWinApp
VirtualFilter=AC
LastObject=CSampleDlgApp

[CLS:CSampleDlgDlg]
Type=0
HeaderFile=SampleDlgDlg.h
ImplementationFile=SampleDlgDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC
LastObject=CSampleDlgDlg

[CLS:CAboutDlg]
Type=0
HeaderFile=SampleDlgDlg.h
ImplementationFile=SampleDlgDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC
LastObject=CAboutDlg

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC_VERNAME,static,1342308480
Control3=IDC_STATIC_VERDATE,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_SAMPLEDLG_DIALOG]
Type=1
Class=CSampleDlgDlg
ControlCount=0

[MNU:IDR_MENU1]
Type=1
Class=?
Command1=ID_APP_EXIT
Command2=ID_HELP2
CommandCount=2

