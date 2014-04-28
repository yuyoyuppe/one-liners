#include <MsgBoxConstants.au3>

Global $winhndl
Global $seconds_since_play = 0
Func _WinWaitActivate($title,$text,$timeout=0)
	WinWait($title,$text,$timeout)
	If Not WinActive($title,$text) Then WinActivate($title,$text)
	WinWaitActive($title,$text,$timeout)
EndFunc

Func MouseRelMove($offset_x, $offset_y, $move_speed = 1)
   Opt('MouseCoordMode', 0)
   Local $newpos = MouseGetPos()
   $newpos[0] = $newpos[0] + $offset_x
   $newpos[1] = $newpos[1] + $offset_y
   MouseMove($newpos[0], $newpos[1], $move_speed)
EndFunc

Func Turn180ish($cw)
   Local $offset_x
   Local $sign = 1

   If $cw = True Then
	  $sign = 1
   Else
	  $sign = -1
   EndIf

   $offset_x = 101 * $sign
   MouseRelMove($offset_x, 0)
EndFunc


Func CheckCheckout()
   PixelSearch(1234, 515, 1334, 560, 0xC90000, 12, 2, $winhndl)
   If Not @error Then
	  ; SoundPlay("ding.wma")
	  $seconds_since_play = 0
	  Return True
   EndIf
   Return False
EndFunc

Func GrabPrize()
   Send("e")
   Sleep(100)
   CheckCheckout()
   Send("e")
   Sleep(100)
   CheckCheckout()
   Send("e")
   CheckCheckout()
EndFunc

Func TakePrizes()
   ; taking left prize
   MouseRelMove(1, 1)
   GrabPrize()
   MouseRelMove(0, -1)
   GrabPrize()
   MouseRelMove(-1, 0)

; taking front prize
   MouseRelMove(-7, 3)
   GrabPrize()
   MouseRelMove(7, -3)

   MouseRelMove(-8, 3) ; nearest jackpot fix
   GrabPrize()
   MouseRelMove(8, -3)

   MouseRelMove(7, 3)
   GrabPrize()
   MouseRelMove(-7, -3)

EndFunc


Func Init()
   Opt('WinWaitDelay',100)
   _WinWaitActivate("Borderlands 2 (32-bit, DX9)","")
   $winhndl = WinGetHandle("Borderlands 2 (32-bit, DX9)")
EndFunc

Func SellItems()
;~    Send("{a down}")
;~    Sleep(2000)
;~    Send("{a up}")
;~    Sleep(1000)
;~    MouseRelMove(10, 1, 50)
;~    Sleep(1000)
;~    Send("{w down}")
;~    Sleep(5000)
;~    Send("{w up}")
;~    ;moxxis exited - stuck at the column
;~    Send("{d down}")
;~    Sleep(250)
;~    Send("{d up}")
;~    MouseRelMove(6, 0, 50)
;~    Send("{w down}")
;~    Sleep(7000)
;~    Send("{w up}")
;~    Send("{w down}")
;~    Sleep(5000)
;~    Send("{w up}")
;~    Send("{a down}")
;~    Sleep(300)
;~    Send("{a up}")
;~    Send("{w down}")
;~    Sleep(6700)
;~    Send("{w up}")
;~    MouseRelMove(17, 0, 50)
;~    Send("{a down}")
;~    Sleep(1000)
;~    Send("{a up}")
;~    Send("{w down}")
;~    Sleep(4000)
;~    Send("{w up}")
EndFunc

Func MainLoop()
   Local $flip = True

   While 1
	  Sleep(500)

	  TakePrizes()

	  Turn180ish($flip)
	  $flip = Not $flip

	  $seconds_since_play += 1.5

	  If ($seconds_since_play > 30) And ($flip = True) Then
		 SellItems()
	  EndIf
   Wend

EndFunc


Init()
MainLoop()
;~ SellItems()


