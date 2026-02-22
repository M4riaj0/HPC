gcc -o programa prueba1.c
for i in 100 200 400 600 800 1000 2000; do ./programa $i >> times2.doc; done
