latex readme.latex
latex readme.latex
dvips -Ppdf -G0 readme.dvi -o readme.ps
ps2pdf -sPAPERSIZE=a4 -dMaxSubsetPct=100 -dCompatibilityLevel=1.2 -dSubSetFonts=true -dEmbedAllFonts=true readme.ps
