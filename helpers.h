#ifndef _HELPERS_H
#define _HELPERS_H 1

#define PACKET_LEN sizeof(struct Packet)

typedef struct topic {
	char name[51];
	int sf;
} topic;

typedef struct udp_struct {
	char topic[50];
	uint8_t type;
	char content[1501];
} msg_udp;

typedef struct tcp_struct {
	char ip[16];
	char content[1501];
	char topic[51];
	char type[11];
	uint16_t port;
} msg_tcp;

/* 1 online, 0 nu */
typedef struct client {
	char id[10];
	int socket;
	int dim_unsent;
	int dim_topics;
	int online;
	struct topic topics[100];
	struct tcp_struct unsent[100];
} client;

/* exit = e, subscribe = s, unsubscribe = u */
typedef struct Packet {
	char ip[16];
	char type;
	char content[1501];
	char topic[51];
	uint16_t port;
	char data_type;
	int sf;
} Packet;

#endif