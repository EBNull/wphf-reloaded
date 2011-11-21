object SelectRcpt: TSelectRcpt
  Left = 0
  Top = 0
  Caption = 'Select Recipients'
  ClientHeight = 362
  ClientWidth = 384
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poOwnerFormCenter
  OnCanResize = FormCanResize
  OnConstrainedResize = FormConstrainedResize
  DesignSize = (
    384
    362)
  PixelsPerInch = 96
  TextHeight = 13
  object lbNames: TCheckListBox
    Left = 8
    Top = 8
    Width = 368
    Height = 307
    OnClickCheck = lbNamesClickCheck
    Anchors = [akLeft, akTop, akRight, akBottom]
    ItemHeight = 15
    PopupMenu = popup
    Style = lbOwnerDrawFixed
    TabOrder = 0
    OnDrawItem = lbNamesDrawItem
  end
  object OK: TButton
    Left = 24
    Top = 325
    Width = 75
    Height = 25
    Anchors = [akLeft, akBottom]
    Caption = 'OK'
    Default = True
    ModalResult = 1
    TabOrder = 1
  end
  object Cancel: TButton
    Left = 286
    Top = 325
    Width = 75
    Height = 25
    Anchors = [akRight, akBottom]
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 2
    ExplicitLeft = 226
  end
  object popup: TPopupMenu
    Left = 136
    Top = 120
    object mnuSelectAll: TMenuItem
      Caption = 'Select All'
      OnClick = mnuSelectAllClick
    end
    object mnuSelectNone: TMenuItem
      Caption = 'Select None'
      OnClick = mnuSelectNoneClick
    end
  end
end
