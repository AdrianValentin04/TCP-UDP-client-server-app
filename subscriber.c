#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "helpers.h"

/* raspunsul de la server */
int serverAnswer(int tcp_sock)
{
	char buffer[sizeof(msg_tcp)];
	memset(buffer, 0, sizeof(msg_tcp));

	int r = recv(tcp_sock, buffer, sizeof(msg_tcp), 0);
	if (!r) return -1;
	else if(r < 0) exit(0);

	msg_tcp *pack = (msg_tcp *)buffer;
	printf("%s:%u - %s - %s - %s\n", 
		pack->ip, 
		pack->port,
		pack->topic, 
		pack->type, 
		pack->content);
	
	return 0;
}

int main(int argc, char** argv) {

	if (argc < 4)
		exit(0);
	
	/* initializam variabilele de care avem nevoie */
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	struct Packet pack;
	fd_set fd;
	FD_ZERO(&fd);
	struct sockaddr_in server_data;

	inet_aton(argv[2], &server_data.sin_addr);
	server_data.sin_port = htons(atoi(argv[3]));
	server_data.sin_family = AF_INET;

	/* deschidem socket-ul */
	int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_sock < 0)
		exit(0);

	/* se adauga stdin-ul si socket-ul ca si cai de comunicare*/
	FD_SET(STDIN_FILENO, &fd);
	FD_SET(tcp_sock, &fd);

	int	r = connect(tcp_sock, (struct sockaddr *)&server_data, 
					sizeof(server_data));
	if (r < 0)
		exit(0);

	/* se trimite id-ul clientului */
	r = send(tcp_sock, argv[1], 10, 0);
	if (r < 0)
		exit(0);

	setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, "1", sizeof(int));

	int ok = 1;
	while(ok) {
		fd_set aux_set = fd;

		int sel = select(tcp_sock + 1, &aux_set, NULL, NULL, NULL);
		if (sel < 0)
			exit(0);

		if (FD_ISSET(STDIN_FILENO, &aux_set)) {
			char buffer[100];
			memset(buffer, 0, 100);
			fgets(buffer, 100, stdin);
			char *token = strtok(buffer, " ");

			/* verificam daca exista comenzi si trimitem 
				pachete in functie de ele*/
			memset(&pack, 0, PACKET_LEN);
			if (!strncmp(token, "exit", 4)) {
				pack.type = 'e';
				
				int sending = send(tcp_sock, &pack, PACKET_LEN, 0);
				if (sending < 0)
					exit(0);
				
				break;
			} else if (!strncmp(token, "unsubscribe", 11)) {
				token = strtok(NULL, " ");
				strcpy(pack.topic, token);
				pack.type = 'u';
				token = strtok(NULL, " ");
				pack.data_type = token[0];

				printf("Unsubscribed to topic.\n");
				int sending = send(tcp_sock, &pack, PACKET_LEN, 0);
				if (sending < 0)
					exit(0);
			} else if (!strncmp(token, "subscribe", 9)) {
				token = strtok(NULL, " ");
				strcpy(pack.topic, token);
				pack.type = 's';
				token = strtok(NULL, " ");
				pack.data_type = token[0] - '0';

				printf("Subscribed to topic.\n");
				int sending = send(tcp_sock, &pack, PACKET_LEN, 0);
				if (sending < 0)
					exit(0);
			} else break;
		}
		
		if(FD_ISSET(tcp_sock, &aux_set)) {
			r = serverAnswer(tcp_sock);
			if (r < 0) break;
		}
	}

	close(tcp_sock);
	return 0;
}

