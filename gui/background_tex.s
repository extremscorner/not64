   .rodata
   .globl BGtexture
   .balign 32
   BGtexture:
   .incbin	"C:/devkitPro/devkitPPC/mupen64gc/gui/background.tex"
   .globl BGtexture_length
   BGtexture_length:
   .long (BGtexture_length - BGtexture)

   .globl BGtextureCI
   .balign 32
   BGtextureCI:
   .incbin	"C:/devkitPro/devkitPPC/mupen64gc/gui/background.tlut"
   .globl BGtextureCI_length
   BGtextureCI_length:
   .long (BGtextureCI_length - BGtextureCI)
