   .rodata
   
   .balign 32
   _customTMD:
   	.incbin	"./fileBrowser/HW64/hw64.tmd"
   .globl customTMD
   customTMD:
   	.long _customTMD
   .globl customTMDSize
   customTMDSize:
   	.long (customTMD - _customTMD)

   .balign 32
   _customTicket:
   	.incbin	"./fileBrowser/HW64/hw64.tik"
   .globl customTicket
   customTicket:
   	.long _customTicket
   .globl customTicketSize
   customTicketSize:
   	.long (customTicket - _customTicket)

   .balign 32
   _customCert:
   /**/
   	.incbin	"./fileBrowser/HW64/cert"
    /**/
   .globl customCert
   customCert:
   	.long /*0/**/_customCert/**/
   .globl customCertSize
   customCertSize:
   	.long (customCert - _customCert)
   
   .balign 32
   _customBanner:
   	.incbin	"./fileBrowser/HW64/banner.bin"
   .globl customBanner
   customBanner:
   	.long _customBanner
   .globl customBannerSize
   customBannerSize:
   	.long (customBanner - _customBanner)


