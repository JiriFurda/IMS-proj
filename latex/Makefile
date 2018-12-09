all:
	latex manual.tex
	bibtex manual.aux
	latex manual.tex 
	latex manual.tex
	dvips -t a4 manual.dvi
	ps2pdf -sPAPERSIZE=a4 manual.ps 
