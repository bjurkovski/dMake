all:
	@pdflatex *.tex
	@pdflatex *.tex	# Table des matières, etc.
	@echo "Création du PDF terminée."
clean:
	@rm -f  *.ps *.log *.lot *.lof *.toc *.aux *.nav *.out *.snm *.vrb *.dvi dmake-rapport.pdf
	@echo "Nettoyage des fichiers terminé."

.PHONY: all clean
