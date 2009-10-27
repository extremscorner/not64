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
