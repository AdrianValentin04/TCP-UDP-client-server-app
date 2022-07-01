#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <limits.h>
#include "helpers.h"

int main(int argc, char** argv) {

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	if (argc < 2)
		exit(0);

	/* se initializeaza variabilele de care avem nevoie */
	client* clients = calloc(1000, sizeof(client));
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	fd_set fd;
	FD_ZERO(&fd);
	struct sockaddr_in serv_addr, udp_addr, new_tcp;
	memset((char*)&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	/* deschidem socket tcp */
	int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_sock < 0)
		exit(0);

	int tcp_bnd = bind(tcp_sock, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr));
	if (tcp_bnd < 0)
		exit(0);

	/* se adauga socket-ul ca si cale de comunicare */
	setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, "1", 4);
	FD_SET(tcp_sock, &fd);

	int tcp_lsn = listen(tcp_sock, INT_MAX);
	if (tcp_lsn < 0)
		exit(0);

	/* deschidere socket udp */
	int udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (udp_sock < 0)
		exit(0);

	udp_addr.sin_port = htons(atoi(argv[1]));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_addr.s_addr = INADDR_ANY;

	int udp_bnd = bind(udp_sock, (struct sockaddr*)&udp_addr, sizeof(struct sockaddr));
	if (udp_bnd < 0)
		exit(0);

	/* se adauga socket-ul ca si cale de comunicare */
	FD_SET(udp_sock, &fd);

	FD_SET(STDIN_FILENO, &fd);
	socklen_t udp_len = sizeof(struct sockaddr);

	int maxim = (tcp_sock > udp_sock) ? tcp_sock : udp_sock;
	int ok = 1;
	char buffer[PACKET_LEN];
	int ret;

	while (ok) {
		fd_set aux_set = fd;

		int sel = select(maxim + 1, &aux_set, NULL, NULL, NULL);
		if (sel < 0)
			exit(0);

		for (int i = 0; i <= maxim; i++) {
			if (FD_ISSET(i, &aux_set)) {
				memset(buffer, 0, PACKET_LEN);

				if (i == STDIN_FILENO) {
					fgets(buffer, 100, stdin);

					/* se primeste comand exit */
					if (strncmp(buffer, "exit", 4) == 0) {
						ok = 0;
						break;
					}
					else exit(0);

				}
				else if (i == tcp_sock) {
					int socket = accept(tcp_sock, (struct sockaddr*)&new_tcp, &udp_len);
					if (socket < 0)
						exit(0);

					ret = recv(socket, buffer, 10, 0);
					if (ret < 0)
						exit(0);

					int found = -1;
					int online = 0;

					for (int j = 5; j <= maxim; j++) {
						if (!strcmp(clients[j].id, buffer)) {
							online = clients[j].online;
							found = j;

							if (online) {
								/* client nou cu id vechi */
								close(socket);
								printf("Client %s already connected.\n", clients[found].id);

							}
							else {
								FD_SET(socket, &fd);
								clients[found].socket = socket;
								
								printf("New client %s connected from %s:%d.\n", clients[found].id,
									inet_ntoa(new_tcp.sin_addr), ntohs(new_tcp.sin_port));
								clients[found].online = 1;
								for (int k = 0; k < clients[found].dim_unsent; k++) {
									ret = send(clients[found].socket, &clients[found].unsent[k],
										sizeof(msg_tcp), 0);
									if (ret < 0)
										exit(0);
								}
								clients[found].dim_unsent = 0;
							}
							break;
						}
					}

					if (found < 0) {
						/* un nou client */
						FD_SET(socket, &fd);
						if (socket > maxim)
							maxim = socket;

						strcpy(clients[maxim].id, buffer);
						printf("New client %s connected from %s:%d\n", clients[maxim].id,
							inet_ntoa(new_tcp.sin_addr), ntohs(new_tcp.sin_port));

						clients[maxim].online = 1;
						clients[maxim].socket = socket;
					}
				}
				else if (i == udp_sock) {
					/* s-a primit mesaj de pe socket-ul udp */
					ret = recvfrom(udp_sock, buffer, 1551, 0, (struct sockaddr*)&udp_addr,
						&udp_len);
					if (ret < 0)
						exit(0);

					msg_tcp send_to_tcp;
					memset(&send_to_tcp, 0, sizeof(msg_tcp));

					send_to_tcp.port = htons(udp_addr.sin_port);
					strcpy(send_to_tcp.ip, inet_ntoa(udp_addr.sin_addr));
					send_to_tcp.topic[50] = 0;

					msg_udp* send_to_udp = (msg_udp*)buffer;
					strcpy(send_to_tcp.topic, send_to_udp->topic);

					double real;
					uint32_t num;

					switch (send_to_udp->type)
					{
					case 0:
						num = ntohl(*(uint32_t*)(send_to_udp->content + 1));

						if (send_to_udp->content[0] == 1)
							num = 0 - num;

						sprintf(send_to_tcp.content, "%d", num);
						strcpy(send_to_tcp.type, "INT");
						break;
					case 1:
						real = ntohs(*(uint16_t*)(send_to_udp->content));
						real /= 100;

						sprintf(send_to_tcp.content, "%.2f", real);
						strcpy(send_to_tcp.type, "SHORT_REAL");

						break;
					case 2:
						real = ntohl(*(uint32_t*)(send_to_udp->content + 1));
						int n = 1;
						int j;

						for (j = 0; j < send_to_udp->content[5]; j++)
							n *= 10;

						real /= n;
						strcpy(send_to_tcp.type, "FLOAT");

						if (send_to_udp->content[0] == 1)
							real = 0 - real;
						sprintf(send_to_tcp.content, "%lf", real);

						break;
					default:
						strcpy(send_to_tcp.content, send_to_udp->content);
						strcpy(send_to_tcp.type, "STRING");

						break;
					}

					for (int j = 5; j <= maxim; j++) {
						for (int k = 0; k < clients[j].dim_topics; k++) {
							if (!strcmp(clients[j].topics[k].name, send_to_tcp.topic)) {

								if (!clients[j].online && clients[j].topics[k].sf == 1) {
											clients[j].unsent[clients[j].dim_unsent] = send_to_tcp;
											clients[j].dim_unsent++;
								}
								else if (clients[j].online) {
									ret = send(clients[j].socket, &send_to_tcp,
										sizeof(msg_tcp), 0);
									if (ret < 0)
										exit(0);
								}
								break;
							}
						}
					}
				}
				else {
					/* s-a primit mesaj de pe socket-ul tcp */
					memset(buffer, 0, PACKET_LEN);

					ret = recv(i, buffer, PACKET_LEN, 0);
					if (ret < 0)
						exit(0);
					else if (ret == 0) {
						for (int j = 5; j <= maxim; j++) {
							if (clients[j].socket == i) {
								clients[j].online = 0;
								clients[j].socket = -1;
								printf("Client %s disconnected.\n", clients[j].id);

								FD_CLR(i, &fd);
								close(i);
								break;
							}
						}
					}
					else {
						client* client = NULL;
						Packet* input = (Packet*)buffer;

						for (int j = 5; j <= maxim; j++) {
							if (i == clients[j].socket) {
								client = &clients[j];
								break;
							}
						}

						int topicIndex = -1;
						/* se verifica tipul de pachet primit */
						switch (input->type)
						{
						case 'e':
							for (int j = 5; j <= maxim; j++) {
								if (clients[j].socket == i) {
									clients[j].socket = -1;
									clients[j].online = 0;
									printf("Client %s disconnected.\n", clients[j].id);
									FD_CLR(i, &fd);
									close(i);
									break;
								}
							}
							break;
						case 's':
							for (int k = 0; k < client->dim_topics; k++) {
								if (!strcmp(client->topics[k].name, input->topic)) {
									topicIndex = k;
									break;
								}
							}
							if (topicIndex < 0) {
								int index = client->dim_topics;
								strcpy(client->topics[index].name, input->topic);
								client->topics[index].sf = input->data_type;
								client->dim_topics++;
							}
							break;
						case 'u':
							for (int k = 0; k < client->dim_topics; k++)
								if (!strcmp(client->topics[k].name, input->topic)) {
									topicIndex = k;
									break;
								}

							if (topicIndex >= 0) {
								int index = topicIndex;

								while (index < client->dim_topics) {
									client->topics[index] = client->topics[index + 1];
									index++;
								}									
								client->dim_topics--;
							}
							break;
						}
					}
				}
			}
		}
	}

	for (int i = 3; i <= maxim; i++) {
		if (FD_ISSET(i, &fd))
			close(i);
	}

	return 0;
}