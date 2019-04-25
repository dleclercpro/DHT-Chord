#ifndef NODE_H
#define NODE_H



// Node
typedef struct {
    uint16_t id;
    uint8_t ip[4];
    uint16_t port;
} node;



// SHOWNODE
void showNode(char *name, node *peer) {

	// Show node
	printf("%s node: [%d] %d.%d.%d.%d:%d\n", name, peer -> id, (peer -> ip)[0], (peer -> ip)[1], (peer -> ip)[2], (peer -> ip)[3], peer -> port);
}



// FILLNODE
void fillNode(node * peer, uint16_t id, uint8_t *ip, uint16_t port) {

	// Set ID
	peer -> id = id;

	// Set IP
	memcpy(peer -> ip, ip, 4);

	// Set port
	peer -> port = port;
}



#endif