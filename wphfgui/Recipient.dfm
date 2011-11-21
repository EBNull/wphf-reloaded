object RecipientName: TRecipientName
  Left = 296
  Top = 450
  BorderStyle = bsDialog
  Caption = 'Recipient'#39's name'
  ClientHeight = 124
  ClientWidth = 327
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  FormStyle = fsStayOnTop
  OldCreateOrder = False
  Position = poOwnerFormCenter
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 44
    Top = 19
    Width = 62
    Height = 13
    Alignment = taRightJustify
    Caption = 'FAX number:'
  end
  object Label2: TLabel
    Left = 22
    Top = 46
    Width = 84
    Height = 13
    Alignment = taRightJustify
    Caption = 'Recipient'#39's name:'
  end
  object FAXNumber: TEdit
    Left = 112
    Top = 16
    Width = 193
    Height = 21
    TabStop = False
    Color = clYellow
    ReadOnly = True
    TabOrder = 0
    Text = 'FAXNumber'
  end
  object RecipientName: TEdit
    Left = 112
    Top = 43
    Width = 193
    Height = 21
    TabOrder = 1
    OnKeyPress = RecipientNameKeyPress
  end
  object OK: TButton
    Left = 64
    Top = 80
    Width = 75
    Height = 25
    Caption = 'OK'
    Default = True
    ModalResult = 1
    TabOrder = 2
  end
  object Cancel: TButton
    Left = 184
    Top = 80
    Width = 75
    Height = 25
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 3
  end
end
