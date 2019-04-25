#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <math.h>
#include "constants.h"
#include "lib.h"
#include "uthash.h"
#include "database.h"
#include "node.h"
#include "message.h"



// ISRESPONSIBLE
int isResponsible(node * peer_prev, node * peer, uint16_t hashed_id) {

	// Natural order: prev ID < ID
	if (peer_prev -> id < peer -> id) {

		// Peer ID is comprised between ID and prev ID
		return peer_prev -> id < hashed_id && hashed_id <= peer -> id;
	}
	// Reversed order (crossing 0): prev ID > ID
	else {

		// Peer ID is comprised between ID and prev ID
		return peer_prev -> id < hashed_id || hashed_id <= peer -> id;
	}
}



// ISQUERY
int isQuery(uint8_t ack) {

	// ACK bit not set: query
	return ack != ACK;
}



// ADDNODEINFOS
void addNodeInfos(uint8_t *buffer, node *peer) {

	// Add ID
	packBytes(buffer + 6, 2, peer -> id);

	// Add IP
	memcpy(buffer + 8, peer -> ip, 4);

	// Add port
	packBytes(buffer + 12, 2, peer -> port);
}



// EXTENDMESSAGE
void extendMessage(uint8_t *buffer_in, uint8_t *buffer_out, message *msg, node *peer) {

	// Set first byte with internal marker
	buffer_out[0] = buffer_in[0] | INTERNAL;

	// Copy transaction ID, key length, and value length bytes
	memcpy(buffer_out + 1, buffer_in + 1, 5);

	// Add node infos
	addNodeInfos(buffer_out, peer);

	// Copy key/value to buffer
	memcpy(buffer_out + HEADER_SIZE_INTERNAL, msg -> key, msg -> key_length);
	memcpy(buffer_out + HEADER_SIZE_INTERNAL + (msg -> key_length), msg -> value, msg -> value_length);
}



// PROCESSQUERY
int processQuery(uint8_t *buffer, message *msg, node *peer) {

    // Default: send header only
    int n_bytes_to_send = HEADER_SIZE_INTERNAL;

	// Internal message
	if (msg -> type == INTERNAL) {

	    // Acknowledge RPC
	    buffer[0] = INTERNAL | ACK;
	}

	// External message
	else {

	    // Acknowledge message
	    buffer[0] = ACK;
	}

    // Assign transaction ID
    buffer[1] = msg -> tx_id;

    // Default: key length is zero
    buffer[2] = 0;
    buffer[3] = 0;

    // Default: value length is zero
    buffer[4] = 0;
    buffer[5] = 0;

	// Add node infos
	addNodeInfos(buffer, peer);

    // Initialize pointer to database entry and corresponding properties
    struct entry *e;
    void *k;
    void *v;
    uint16_t k_len;
    uint16_t v_len;

    // Process RPC
    switch (msg -> op) {

    	// GET
    	case GET:

    		// Get entry from database
    		if ((e = get(msg -> key, msg -> key_length)) != NULL) {

	    		// Define header
	    		buffer[0] |= GET;

	    		// Get stored key/value
	    		k = e -> k;
	    		v = e -> val;

	    		// Get their length in bits
	    		k_len = e -> k_len;
	    		v_len = e -> val_len;

	    		// Define number of bytes to send
	    		n_bytes_to_send += k_len + v_len;

	    		// Define key length in bytes
	    		packBytes(buffer + 2, 2, k_len);

	    		// Define value length in bytes
	    		packBytes(buffer + 4, 2, v_len);

	    		// Copy bytes to buffer
	    		memcpy(buffer + HEADER_SIZE_INTERNAL, k, k_len);
	    		memcpy(buffer + HEADER_SIZE_INTERNAL + k_len, v, v_len);
    		}

    		// Exit
    		break;

    	// SET
    	case SET:

    		// Add new entry to database
    		if (set(msg -> key, msg -> key_length, msg -> value, msg -> value_length) != -1) {

	    		// Define header
	    		buffer[0] |= SET;
    		}

    		// Exit
    		break;

    	// DELETE
    	case DELETE:

    		// Get entry
    		if ((e = get(msg -> key, msg -> key_length)) != NULL) {

	    		// Define header
	    		buffer[0] |= DELETE;

	    		// Delete entry from database
	    		delete(e);
    		}

    		// Exit
    		break;
    }

	// Return number of bytes to send
	return n_bytes_to_send;
}



// RESPONDTOCLIENT
void respondToClient(uint8_t *buffer, int n_bytes_to_send, uint16_t tx_id) {

	// Initialize variable for client socket
	int s_client;

	// Get client
	if ((s_client = getClient(tx_id)) == -1) {

		// Client does not exist
		fail("Client error");
	}

	// Update number of bytes to send to fit external format
	n_bytes_to_send -= (HEADER_SIZE_INTERNAL - HEADER_SIZE_EXTERNAL);

	// Remove ID, IP and port when responding to client
	memmove(buffer + HEADER_SIZE_EXTERNAL, buffer + HEADER_SIZE_INTERNAL, n_bytes_to_send - HEADER_SIZE_EXTERNAL);

	// Send message to next peer
	if (send(s_client, buffer, n_bytes_to_send, 0) == -1) {

		// Describe error
		fail("Send error");
	}

	// Remove client
	deleteClient(tx_id);
}



// FORWARDMESSAGE
void forwardMessage(uint8_t *buffer, int n_bytes, node *peer) {

	// Initialize yes variable
	int y = 1;

	// Initialize socket variables
	int s;
	struct addrinfo hints, *ai;

	// Fill address hints to generate socket address structure
	memset(&hints, 0, sizeof(hints)); // Make sure the struct is empty
	hints.ai_family = AF_UNSPEC;      // Don't care if IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;  // Stream sockets

	// Parse source IP
	// MAX: 255.255.255.255
	char peer_ip[15] = {0};
	sprintf(peer_ip, "%d.%d.%d.%d", (peer -> ip)[0], (peer -> ip)[1], (peer -> ip)[2], (peer -> ip)[3]);

	// Parse source port
	// MAX: 65535
	char peer_port[5] = {0};
	sprintf(peer_port, "%d", peer -> port);

	// Get address of source node
	if (getaddrinfo(peer_ip, peer_port, &hints, &ai) == -1) {

		// Describe error
		fail("Address error");
    }

	// Define next peer socket
	if ((s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {

		// Describe error
		fail("Socket error");
	}

	// Configure socket
	// SO_REUSEADDR: avoid "address already in use" error
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int)) == -1) {

		// Describe error
		fail("Configure error");
	}

	// Connect socket
	if (connect(s, ai->ai_addr, ai->ai_addrlen) == -1) {

		// Describe error
		fail("Connect error");
	}

	// Send message to next peer
	if (send(s, buffer, n_bytes, 0) == -1) {

		// Describe error
		fail("Send error");
	}

	// Close socket
	close(s);

	// Free memory allocated for source address
	freeaddrinfo(ai);
}



// MAIN
int main(int argc, char *argv[]) {

	// Check number of arguments
	if (argc == 10) {

		// Initialize general variables
		int y = 1;
		struct timespec now;

		// Initialize socket variables
		fd_set sockets, read_sockets;
		int s_listener, new_s, s_max;
		struct addrinfo hints, *ai, *ai_prev, *ai_next;
		struct sockaddr remote_addr, ip_addr, ip_prev_addr, ip_next_addr;
		int remote_size;

		// Initialize nodes
		node * peer = malloc(sizeof(node));
		node * peer_prev = malloc(sizeof(node));
		node * peer_next = malloc(sizeof(node));
		node * peer_source = malloc(sizeof(node));

		// Define node IDs
		peer -> id = strtol(argv[1], NULL, 10);
		peer_prev -> id = strtol(argv[4], NULL, 10);
		peer_next -> id = strtol(argv[7], NULL, 10);

		// Define node IPs
		ipStrToBytes(peer -> ip, argv[2]);
		ipStrToBytes(peer_prev -> ip, argv[5]);
		ipStrToBytes(peer_next -> ip, argv[8]);

		// Define node ports
		peer -> port = strtol(argv[3], NULL, 10);
		peer_prev -> port = strtol(argv[6], NULL, 10);
		peer_next -> port = strtol(argv[9], NULL, 10);

		// Initialize buffer variables
		int n_bytes;
		int n_bytes_to_send;
		char *key;
		char *value;
		uint8_t buffer_in[BUFFER_SIZE] = {0};
		uint8_t buffer_out[BUFFER_SIZE] = {0};

		// Initialize responsible peer ID for given key
		uint16_t hashed_id;



		// Check IDs
		if (peer -> id == 0 || peer_prev -> id == 0 || peer_next -> id == 0 ||
			peer_prev -> id == peer -> id || peer -> id == peer_next -> id || peer_prev -> id == peer_next -> id) {

			// Error
			fail("Peer ID error");
		}



		// Fill address hints to generate socket address structure
		memset(&hints, 0, sizeof(hints)); // Make sure the struct is empty
		hints.ai_family = AF_UNSPEC;      // Don't care if IPv4 or IPv6
		hints.ai_socktype = SOCK_STREAM;  // Stream sockets



		// Get address structure list based on hints
		if(getaddrinfo(argv[2], argv[3], &hints, &ai) == -1 ||
		   getaddrinfo(argv[5], argv[6], &hints, &ai_prev) == -1 ||
		   getaddrinfo(argv[8], argv[9], &hints, &ai_next) == -1) {

			// Describe error
			fail("Address error");
	    }



		// Define listener socket
		if ((s_listener = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {

			// Describe error
			fail("Socket error");
		}



		// Configure socket
		// SO_REUSEADDR: avoid "address already in use" error
		if (setsockopt(s_listener, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int)) == -1) {

			// Describe error
			fail("Configure error");
		}



		// Bind socket
		if (bind(s_listener, ai->ai_addr, ai->ai_addrlen) == -1) {

			// Describe error
			fail("Bind error");
		}



		// Listen
		if (listen(s_listener, 0) == -1) {

			// Describe error
			fail("Listen error");
		}



		// Initialize file descriptor set for sockets
		FD_ZERO(&sockets);
		FD_ZERO(&read_sockets);

		// Add listener socket to set
		FD_SET(s_listener, &sockets);

		// Define max socket file descriptor
		s_max = s_listener;



		// Wait for reading operations on sockets
		while (1) {

			// Copy file descriptor set
			read_sockets = sockets;

			// Wait for event on sockets
			if (select(s_max + 1, &read_sockets, NULL, NULL, NULL) == -1) {

				// Describe error
				fail("Select error");
			}

			// Loop through existing sockets for data to read
			for (int s = 0; s <= s_max; s++) {

				// Find socket ready for reading
				if (FD_ISSET(s, &read_sockets)) {

					// Listener
					if (s == s_listener) {

						// Accept new connection
						if ((new_s = accept(s_listener, &remote_addr, &remote_size)) == -1) {

							// Describe error
							fail("Accept error");
						}

						// SO_REUSEADDR: avoid "address already in use" error
						if (setsockopt(new_s, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int)) == -1) {

							// Describe error
							fail("Configure error");
						}

						// Add socket to set
						FD_SET(new_s, &sockets);

						// If new socket fd bigger than previous max one
						if (new_s > s_max) {

							// Update it
							s_max = new_s;
						}
					}



					// Other connections
					else {

					    // Read bytes
						if ((n_bytes = recv(s, buffer_in, BUFFER_SIZE, 0)) == -1) {

							// Describe error
							fail("Receive error");
						}

						// Socket closed
						if (n_bytes == 0) {

							// Close new socket
							close(s);

							// Remove from socket set
							FD_CLR(s, &sockets);

							// Next socket
							continue;
						}



						// Parse message from bytes
						message *msg = bytesToMessage(buffer_in);

						// Take source node from received message
						fillNode(peer_source, msg -> id, msg -> ip, msg -> port);

						// Get current time
						clock_gettime(CLOCK_MONOTONIC_RAW, &now);

						// New line
						printf("\n");

						// Show current time
						printf("Time: %ld\n", now.tv_nsec);

						// Show previous node
						showNode("Current", peer);
						showNode("Previous", peer_prev);
						showNode("Next", peer_next);
						showNode("Source", peer_source);

						// New line
						printf("\n");

						// Show message
						showMessage(msg);

						// New line
						printf("\n");



						// Internal
						if (msg -> type != INTERNAL) {

							// Store client socket
							addClient(msg -> tx_id, s);
						}



						// Compute ID of node responsible for the transaction
						hashed_id = hash(msg -> key, msg -> key_length);



						// Internal message
						if (msg -> type == INTERNAL) {

							// Info
							printf("Internal message.\n");

							// Responsible
							if (isResponsible(peer_prev, peer, hashed_id)) {

								// Info
								printf("Node is responsible.\n");
								printf("Processing query and building response...\n");

								// Build response
								n_bytes_to_send = processQuery(buffer_out, msg, peer);

								// Info
								printf("Sending to source node...\n");

								// Forward message to source peer
								forwardMessage(buffer_out, n_bytes_to_send, peer_source);
							}

							// Not responsible
							else {

								// Info
								printf("Node is NOT responsible.\n");
								printf("Copying message from incoming to outgoing buffer...\n");

								// Forwarding message
								n_bytes_to_send = n_bytes;

								// Copy buffer in to out
								memcpy(buffer_out, buffer_in, n_bytes_to_send);

								// Query
								if (isQuery(msg -> ack)) {

									// Info
									printf("Query.\n");
									printf("Forwarding message to next node...\n");

									// Forward message
									forwardMessage(buffer_out, n_bytes_to_send, peer_next);
								}

								// Response
								else {

									// Info
									printf("Response.\n");
									printf("Responding to client...\n");

									// Respond to client
									respondToClient(buffer_out, n_bytes_to_send, msg -> tx_id);
								}
							}
						}

						// External message
						else {

							// Info
							printf("External message.\n");

							// Responsible
							if (isResponsible(peer_prev, peer, hashed_id)) {

								// Info
								printf("Node is responsible.\n");
								printf("Processing query and building response...\n");

								// Build response
								n_bytes_to_send = processQuery(buffer_out, msg, peer);

								// Info
								printf("Responding to client...\n");

								// Respond to client
								respondToClient(buffer_out, n_bytes_to_send, msg -> tx_id);
							}

							// Not responsible
							else {

								// Info
								printf("Node is NOT responsible.\n");
								printf("Extending message...\n");

								// Add ID, IP and port bytes
								n_bytes_to_send = n_bytes + (HEADER_SIZE_INTERNAL - HEADER_SIZE_EXTERNAL);

								// Extend message
								extendMessage(buffer_in, buffer_out, msg, peer);

								// Info
								printf("Forwarding message to next node...\n");

								// Forward message
								forwardMessage(buffer_out, n_bytes_to_send, peer_next);
							}
						}

						// Free memory allocated to received message
						freeMessage(msg);
					}
				}
			}
		}

		// Free memory allocated for server address (not useful anymore)
		freeaddrinfo(ai);
		freeaddrinfo(ai_prev);
		freeaddrinfo(ai_next);

		// Free memory allocated for nodes
		free(peer);
		free(peer_prev);
		free(peer_next);
		free(peer_source);
	}

	// Not right amount of arguments
	else {

		// Error
		fail("Not right amount of arguments");
	}

	// Delete hashtables
	deleteAllClients();
	delete_all();

	// Terminate
	return 0;
}
