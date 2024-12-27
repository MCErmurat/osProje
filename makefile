#/**
#* @course		İşletim Sistemleri
#* @assignment		Proje
#* @group		51
#* @author Y245012115 	Mürvet Esmer
#* @author Y245012116 	Oktay Esmer
#* @author G221210381 	Mehmet Can Ermurat
#* @author G221210372 	Mustafa Sait Karadağ
#* @author Y245060024 	Nihan Hüsna Kılıç Beşik
#*/

# Makefile: Kabuk ve Increment Programı için Basit Yapı

# Hedefler
hepsi: derle calistir

derle:
	gcc -g ./src/shell.c -o ./bin/shell
	gcc -g ./src/increment.c -o ./bin/increment

calistir:
	./bin/shell

temizle:
	rm -f ./bin/shell ./bin/increment
