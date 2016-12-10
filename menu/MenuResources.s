   .rodata
   
   .globl BackgroundTexture
   .balign 32
   BackgroundTexture:
   .incbin	"./menu/resources/bg.tx"
   .globl BackgroundTexture_length
   BackgroundTexture_length:
   .long (BackgroundTexture_length - BackgroundTexture)

   .globl LoadingTexture
   .balign 32
   LoadingTexture:
   .incbin	"./menu/resources/Loading.tx"
   .globl LoadingTexture_length
   LoadingTexture_length:
   .long (LoadingTexture_length - LoadingTexture)

   .globl ControlEmptyTexture
   .balign 32
   ControlEmptyTexture:
   .incbin	"./menu/resources/cntrlEmpty.tx"
   .globl ControlEmptyTexture_length
   ControlEmptyTexture_length:
   .long (ControlEmptyTexture_length - ControlEmptyTexture)

   .globl ControlGameCubeTexture
   .balign 32
   ControlGameCubeTexture:
   .incbin	"./menu/resources/cntrlGC.tx"
   .globl ControlGameCubeTexture_length
   ControlGameCubeTexture_length:
   .long (ControlGameCubeTexture_length - ControlGameCubeTexture)

   .globl ControlWiiUProTexture
   .balign 32
   ControlWiiUProTexture:
   .incbin	"./menu/resources/cntrlWUPro.tx"
   .globl ControlWiiUProTexture_length
   ControlWiiUProTexture_length:
   .long (ControlWiiUProTexture_length - ControlWiiUProTexture)

   .globl ControlClassicTexture
   .balign 32
   ControlClassicTexture:
   .incbin	"./menu/resources/cntrlClassic.tx"
   .globl ControlClassicTexture_length
   ControlClassicTexture_length:
   .long (ControlClassicTexture_length - ControlClassicTexture)

   .globl ControlWiimoteNunchuckTexture
   .balign 32
   ControlWiimoteNunchuckTexture:
   .incbin	"./menu/resources/cntrlWNC.tx"
   .globl ControlWiimoteNunchuckTexture_length
   ControlWiimoteNunchuckTexture_length:
   .long (ControlWiimoteNunchuckTexture_length - ControlWiimoteNunchuckTexture)

   .globl ControlWiimoteTexture
   .balign 32
   ControlWiimoteTexture:
   .incbin	"./menu/resources/cntrlWM.tx"
   .globl ControlWiimoteTexture_length
   ControlWiimoteTexture_length:
   .long (ControlWiimoteTexture_length - ControlWiimoteTexture)

   .globl N64ControllerTexture
   .balign 32
   N64ControllerTexture:
   .incbin	"./menu/resources/n64Cntrl.tx"
   .globl N64ControllerTexture_length
   N64ControllerTexture_length:
   .long (N64ControllerTexture_length - N64ControllerTexture)

