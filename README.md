# Proiect-PM Cheresdi Doru 331CA

System de monitorizare a angajatilor WFH si asigurare a ergonomiei muncii la birou. Sistemul suporta setarea programului de lucru care este verificat cu un senzor RTC, iar verificarea ca angajatul se afla la birou se face cu un senzor de distanta cu ultrasunete la un anumit interval de timp.

Setarea programului se poate face dupa introducerea parolei de la PC, microcontrollerul va comunica cu acesta prin interfata seriala. Parola are rolul de a securiza sistemul astfel incat angajatul sa nu poata schimba programul de lucru fara aprobarea angajatorului. Daca comunicarea cu PC-ul nu este disponibila, in modul DEV se poate sari peste introducerea parolei apasand butonul 2. Setarea programului de lucru se face prin 2 moduri de configurare:

prin potentionmetru, tensiunea fiind tradusa intr-o ora intre 8-20
prin senzorul de distanta si pozitionarea mainii la o distanta care se traduce in ora dorita
Alegerea modului de configurare a programului (potentiometru sau senzor de distanta) se va face in modul urmator: se contorizeaza timpul folosind modulul RTC si daca in 10 secunde se apasa butonul 2 modul de configurare va fi cel bazat pe potentiometru, altfel se va folosi cel cu senzorul de distanta.

Se asigura ergonomia la birou in modul urmator:

Se verifica ca angajatul nu este prea apropriat de birou. Daca distanta fata de birou a angajatului este prea mica, atunci se trimite un warning la PC si se porneste un buzzer pentru a atentiona angajatul.
Se verifica lumina din incapare folosind un senzor de lumina si daca se detecteaza intuneric, angajatul este atentionat sa porneasc dark mode pe pc pentru ergonomie.
Se verifica ca angajatul ia pauze la un anumit interval de timp. Daca trece acel interval si angajatul nu a luat o pauza(s-a ridicat de la birou), atunci se porneste o alarma care poate fi oprita fie daca se ridica de la birou sau daca apasa pe butonul 2 de snooze. Se contorizeaza numarul de snooze-uri pe care angajatul le-a dat.
Se verifica ca angajatul a fost la birou pentru cel putin 1/4 din program. Daca lipseste pentru mai mult de 3/4 din program, atunci sistemul intra in modul WORKER_FAILED.
Toate avertismentele sunt inregistrate si afisate la sfarsitul programului in raportul de lucru. Datele sunt prelucrate si un raport este trimis la PC pentru a fi afisate.

Pentru demo, sistemul va avea modul: DEV. Pentru productie se va implementa modul PROD, diferenta fiind intervalele de timp. In modul DEV aceasta sunt mult mai scurte pentru a se putea testa mai usor proiectul, iar in modul PROD intervalele de timp vor fi cele corespunzatoare unui program de lucru normal.
