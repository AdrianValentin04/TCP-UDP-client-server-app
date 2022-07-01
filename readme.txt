========================= SERVER-CLIENT TCP-UDP =========================
Student: Gheorghe Adrian-Valentin
Grupa: 331CB

Arhiva temei contine un fisier Makefile, asemanator celui din laborator,
un fisier helpers.h, unde mi-am definit structurile necesare rezolvarii
temei, si 2 fisiere .c (server.c si subscriber.c). Ca si structuri am
implementat structura Packet(retine informatiile legate de pachete),
Client(in care am retinut informatii precum un vector de topicuri,
unul de mesaje netrimise ce se vor trimite toate de-odata, dar si daca
acesta e online sau nu), Topic(tine minte numele unui topic si sf-ul sau),
msg_tcp(retine informatiile trimise clientului) si msg_udp(informatiile
necesare clientului udp).

In fisierul subscriber.c pasii ideii de implementare ar fi urmatorii: 
deschidem socket-ul tcp pentru conexiunea cu serverul, cream file 
descriptorul si trimitem catre server id-ul clientului ce se va conecta.
Verificam apoi daca s-a primit vreo comanda de la stdin(ca o consola
interactiva) si completam pachetul pentru server.

In server.c deschidem cei 2 socketi pentru cei doi clienti(tcp si udp),
si initializam conexiunea cu clientii tcp(folosind functia listen()).
Cream file descriptorul si alegem valoarea maxima de input pentru functia
"select". Urmatorul pas ar fi sa verificam daca s-a primit o conexiune de
la un client tcp, avand astfel 3 variante: clientul este unul nou, online
sau reconectat. In functie de fiecare varianta se vor afisa mesajele cerute
in enunt. Un ultim pas ar fi verificarea tipului packetului primit de la
client: daca este un packet de subscribe, atunci topicul ales se va pune in
vectorul de topicuri al clientului, daca este un packet de unsubscribe se 
va scoate din vector, iar daca e un packet de exit, se va deconecta.