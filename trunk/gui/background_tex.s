   .rodata
   .globl BGtexture
   .balign 32
   BGtexture:
   .incbin	"./gui/background.tex"
   .globl BGtexture_length
   BGtexture_length:
   .long (BGtexture_length - BGtexture)

   .globl BGtextureCI
   .balign 32
   BGtextureCI:
   .incbin	"./gui/background.tlut"
   .globl BGtextureCI_length
   BGtextureCI_length:
   .long (BGtextureCI_length - BGtextureCI)

   .globl LOGOtexture
   .balign 32
   LOGOtexture:
   .incbin	"./gui/logo.tex"
   .globl LOGOtexture_length
   LOGOtexture_length:
   .long (LOGOtexture_length - LOGOtexture)
