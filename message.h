#ifndef MESSAGE_H
#define MESSAGE_H



// Message
typedef struct {
    uint8_t type;
    uint8_t ack;
    uint8_t op;
    uint8_t tx_id;
    uint16_t key_length;
    uint16_t value_length;
    uint16_t id;
    uint8_t ip[4];
    uint16_t port;
    char *key;
    char *value;
} message;



// FREEMESSAGE
void freeMessage(message *msg) {
	free(msg -> key);
	free(msg -> value);
	free(msg);
}



// SHOWMESSAGE
void showMessage(message *msg) {

	// Show type
	printf("Internal: %d\n", msg -> type == INTERNAL ? 1 : 0);

	// Show transaction ID
	printf("Transaction: %d\n", msg -> tx_id);

	// Show ACK
	printf("ACK: %d\n", msg -> ack == ACK ? 1 : 0);

	// Show op code
	printf("GET: %d\n", msg -> op == GET ? 1 : 0);
	printf("SET: %d\n", msg -> op == SET ? 1 : 0);
	printf("DELETE: %d\n", msg -> op == DELETE ? 1 : 0);

	// Show key length
	printf("Key length: %d\n", msg -> key_length);

	// Show key
	printf("Key: ");
	showString(msg -> key, msg -> key_length);

	// Show value length
	printf("Value length: %d\n", msg -> value_length);

	// Show value
	printf("Value: ");
	showString(msg -> value, msg -> value_length);
}



// BYTESTOMESSAGE
message * bytesToMessage(uint8_t *buffer) {

	// Initialize header size
	int header_size;

	// Generate empty internal message
	message *msg = (message *) malloc(sizeof(message));

	// Get message type
	msg -> type = buffer[0] & INTERNAL;

	// Internal
	if (msg -> type == INTERNAL) {

		// Define header size
		header_size = HEADER_SIZE_INTERNAL;
	}

	// External
	else {

		// Define header size
		header_size = HEADER_SIZE_EXTERNAL;
	}

	// Build message components
	msg -> ack = buffer[0] & ACK;
	msg -> op = buffer[0] & (GET | SET | DELETE);
	msg -> tx_id = buffer[1];

	// Parse length bytes
	msg -> key_length = unpackBytes(buffer + 2, 2);
	msg -> value_length = unpackBytes(buffer + 4, 2);

	// Internal
	if (msg -> type == INTERNAL) {

		// Set ID
		msg -> id = unpackBytes(buffer + 6, 2);

		// Copy IP
		memcpy(msg -> ip, buffer + 8, 4);

		// Set port
		msg -> port = unpackBytes(buffer + 12, 2);
	}

	// Define key pointer
	msg -> key = malloc(msg -> key_length);
	memcpy(msg -> key, buffer + header_size, msg -> key_length);

	// Define value pointer
	msg -> value = malloc(msg -> value_length);
	memcpy(msg -> value, buffer + header_size + (msg -> key_length), msg -> value_length);

	// Return pointer to message
	return msg;
}



#endif