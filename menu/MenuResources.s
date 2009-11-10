   .rodata
   
   .globl BackgroundTexture
   .balign 32
   BackgroundTexture:
   .incbin	"./menu/resources/bg.tx"
   .globl BackgroundTexture_length
   BackgroundTexture_length:
   .long (BackgroundTexture_length - BackgroundTexture)
   
   .globl LogoTexture
   .balign 32
   LogoTexture:
   .incbin	"./menu/resources/wii64.tx"
   .globl LogoTexture_length
   LogoTexture_length:
   .long (LogoTexture_length - LogoTexture)

   .globl LoadingTexture
   .balign 32
   LoadingTexture:
   .incbin	"./menu/resources/Loading.tx"
   .globl LoadingTexture_length
   LoadingTexture_length:
   .long (LoadingTexture_length - LoadingTexture)

