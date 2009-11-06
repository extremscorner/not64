   .rodata
   .globl CursorPointTexture
   .balign 32
   CursorPointTexture:
   .incbin	"./libgui/resources/CursorPoint.tx"
   .globl CursorPointTexture_length
   CursorPointTexture_length:
   .long (CursorPointTexture_length - CursorPointTexture)

   .globl CursorGrabTexture
   .balign 32
   CursorGrabTexture:
   .incbin	"./libgui/resources/CursorGrab.tx"
   .globl CursorGrabTexture_length
   CursorGrabTexture_length:
   .long (CursorGrabTexture_length - CursorGrabTexture)

   .globl ButtonTexture
   .balign 32
   ButtonTexture:
   .incbin	"./libgui/resources/Button.tx"
   .globl ButtonTexture_length
   ButtonTexture_length:
   .long (ButtonTexture_length - ButtonTexture)

   .globl ButtonFocusTexture
   .balign 32
   ButtonFocusTexture:
   .incbin	"./libgui/resources/ButtonFocus.tx"
   .globl ButtonFocusTexture_length
   ButtonFocusTexture_length:
   .long (ButtonFocusTexture_length - ButtonFocusTexture)

   .globl StyleAButtonTlut
   .balign 32
   StyleAButtonTlut:
   .incbin	"./libgui/resources/ButtonA.tlt"
   .globl StyleAButtonTlut_length
   StyleAButtonTlut_length:
   .long (StyleAButtonTlut_length - StyleAButtonTlut)

   .globl StyleAButtonTexture
   .balign 32
   StyleAButtonTexture:
   .incbin	"./libgui/resources/ButtonA.tx"
   .globl StyleAButtonTexture_length
   StyleAButtonTexture_length:
   .long (StyleAButtonTexture_length - StyleAButtonTexture)

   .globl StyleAButtonFocusTexture
   .balign 32
   StyleAButtonFocusTexture:
   .incbin	"./libgui/resources/ButtonAFoc.tx"
   .globl StyleAButtonFocusTexture_length
   StyleAButtonFocusTexture_length:
   .long (StyleAButtonFocusTexture_length - StyleAButtonFocusTexture)

   .globl StyleAButtonSelectOffTexture
   .balign 32
   StyleAButtonSelectOffTexture:
   .incbin	"./libgui/resources/ButtonASelOff.tx"
   .globl StyleAButtonSelectOffTexture_length
   StyleAButtonSelectOffTexture_length:
   .long (StyleAButtonSelectOffTexture_length - StyleAButtonSelectOffTexture)
   
   .globl StyleAButtonSelectOffFocusTexture
   .balign 32
   StyleAButtonSelectOffFocusTexture:
   .incbin	"./libgui/resources/ButtonASelOffFoc.tx"
   .globl StyleAButtonSelectOffFocusTexture_length
   StyleAButtonSelectOffFocusTexture_length:
   .long (StyleAButtonSelectOffFocusTexture_length - StyleAButtonSelectOffFocusTexture)

   .globl StyleAButtonSelectOnTexture
   .balign 32
   StyleAButtonSelectOnTexture:
   .incbin	"./libgui/resources/ButtonASelOn.tx"
   .globl StyleAButtonSelectOnTexture_length
   StyleAButtonSelectOnTexture_length:
   .long (StyleAButtonSelectOnTexture_length - StyleAButtonSelectOnTexture)
   
   .globl StyleAButtonSelectOnFocusTexture
   .balign 32
   StyleAButtonSelectOnFocusTexture:
   .incbin	"./libgui/resources/ButtonASelOnFoc.tx"
   .globl StyleAButtonSelectOnFocusTexture_length
   StyleAButtonSelectOnFocusTexture_length:
   .long (StyleAButtonSelectOnFocusTexture_length - StyleAButtonSelectOnFocusTexture)
   