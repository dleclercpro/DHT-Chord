#ifndef DATABASE_H
#define DATABASE_H



// Client
struct client {
	int id;
	int s;
	UT_hash_handle hh;
};

// Database entry
struct entry {
    uint8_t *k;
    uint8_t *val;
    uint16_t k_len;   // Key length in bytes
    uint16_t val_len; // Value length in bytes
    UT_hash_handle hh;
};

// Internal hash table for clients
struct client *clients = NULL;

// Database
struct entry *db = NULL;



// SHOWENTRY
void showEntry(struct entry *e)
{
	// Define string pointers for key and value
	char *key = e -> k;
	char *value = e -> val;

	// (
	printf("%s", "(");

	// Show key
	showString(key, e -> k_len);

	// :
	printf("%s", ": ");

	// Show value
	showString(value, e -> val_len);

	// )
	printf(")\n");
}



// SHOWALL
void showAll(void)
{
	// Initialize entry pointers
	struct entry *e, *tmp;

	// Loop through entries in hashtable
	HASH_ITER(hh, db, e, tmp) {

		// Show entry
		showEntry(e);
	}

	// Newline
	printf("\n");
}



// ADDCLIENT
void addClient(int tx_id, int tx_s)
{
	// Info
	//printf("Adding client with TX ID: %d\n", tx_id);

	// Initialize client pointer
    struct client *c;

    // Try to find it in hash table
    HASH_FIND_INT(clients, &tx_id, c);

    // Not found
    if (c == NULL) {

    	// Allocate memory
        c = (struct client*) malloc(sizeof(struct client));
        
        // Set transaction ID
        c -> id = tx_id;

        // Add it to hash table
        HASH_ADD_INT(clients, id, c);
    }

    // Add/modify socket
    c -> s = tx_s;
}



// GETCLIENT
int getClient(int tx_id)
{
	// Info
	//printf("Getting client with TX ID: %d\n", tx_id);

	// Initialize client pointer
    struct client *c;

    // Try finding corresponding entry in hash table
    HASH_FIND_INT(clients, &tx_id, c);

    // Client does not exist
    if (c == NULL) {

    	// Return nothing
    	return -1;
    }

    // Otherwise
    else {

	    // Return socket descriptor
	    return c -> s;
    }
}



// DELETECLIENT
void deleteClient(int tx_id) {

	// Info
	//printf("Deleting client with TX ID: %d\n", tx_id);

	// Initialize transaction entry
    struct client *c;
    
    // Get it
    HASH_FIND_INT(clients, &tx_id, c);

    // Entry found
    if (c != NULL) {

		// Try getting transaction entry and delete it
	    HASH_DEL(clients, c);

	    // Free allocated memory
	    free(c);
    }
}



// DELETEALLCLIENTS
void deleteAllClients(void)
{
	// Initialize client pointers
    struct client *c, *tmp;

    // Iterate on client entries in hash table
    HASH_ITER(hh, clients, c, tmp) {

    	// Delete client from hash table
        HASH_DEL(clients, c);

        // Free its allocated memory
        free(c);
    }
}



// GET
struct entry *get(uint8_t *key, uint16_t key_length)
{
	// Info
	//printf("GET\n");

	// Create pointer to an entry
    struct entry *e;

    // Look for entry in hashtable
    HASH_FIND(hh, db, key, key_length, e);

    // Entry exists
    /*
    if (e != NULL) {

		// Show entry
		show(e);
    }
    */

    // Return pointer to value
    return e;
}



// SET
int set(uint8_t *key, uint16_t key_length, uint8_t *value, uint16_t value_length)
{
	// Info
	//printf("SET\n");

	// Create pointer to an entry
    struct entry *e;

    // Look for entry in hashtable
    HASH_FIND(hh, db, key, key_length, e);

    // If entry doesn't exist
    if (e == NULL) {

    	// Create new entry in database
        if ((e = (struct entry*) malloc(sizeof(struct entry))) == NULL) {

        	// Exit
        	return -1;
        }
        
        // Fill entry with key
        if ((e -> k = (uint8_t *) malloc(key_length)) == NULL) {

        	// Exit
        	return -1;
        }

        // Set key length
        e -> k_len = key_length;

        // Set its key
        memcpy(e -> k, key, key_length);

        // Add value to hashtable
        HASH_ADD_KEYPTR(hh, db, e -> k, e -> k_len, e);
    }

    // Fill entry with value
    if ((e -> val = (uint8_t *) malloc(value_length)) == NULL) {

    	// Exit
    	return -1;
    }

    // Set value length
    e -> val_len = value_length;

    // Set its value
    memcpy(e -> val, value, value_length);

    // Show all entries
    //show_all();

    // Exit
    return 1;
}



// DELETE
void delete(struct entry *e)
{
	// Info
	//printf("DELETE\n");

	// Delete value associated to key
    HASH_DEL(db, e);

    // Free memory allocated for entry
    free(e -> k);
    free(e -> val);
    free(e);
}



// DELETE_ALL
void delete_all(void) {

	// Initialize entry pointers
	struct entry *e, *tmp;

	// Loop through entries in hashtable
	HASH_ITER(hh, db, e, tmp) {

		// Free memory allocated for entry
		delete(e);
	}

	// Clear hashtable
	HASH_CLEAR(hh, db);
}



#endif