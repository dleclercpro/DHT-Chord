#ifndef LIB_H
#define LIB_H



// DJB2 (http://www.partow.net/programming/hashfunctions/#DJBHashFunction)
unsigned int hash(const char *str, unsigned int length) {
  
  unsigned int hash = 5381;
  unsigned int i = 0;

  for (i = 0; i < length; ++str, ++i) {
  	hash = ((hash << 5) + hash) + (*str);
  }

  return hash;
}



// Unpack bytes
unsigned long unpackBytes(uint8_t *bytes, int n_bytes) {

	// Initialize result
	unsigned long result = 0;

	// Loop on bytes
	for (int i = 0; i < n_bytes; i++) {

		// Add them to result
		result |= bytes[i] << (8 * (n_bytes - 1 - i));
	}

	// Return unpacked bytes
	return result;
}



// Pack bytes
void packBytes(uint8_t *bytes, int n_bytes, unsigned long x) {

	// Loop on bytes
	for (int i = 0; i < n_bytes; i++) {

		// Add them to result
		bytes[n_bytes - 1 - i] = (x & (0xFF << 8 * i)) >> 8 * i;
	}
}



// IPSTRTOBYTES
void ipStrToBytes(uint8_t *buffer, char *ip) {

	// Initialize characters
	char c, cc;

	// Initialize counting variables
	int i = 0; // ith character in IP string
	int b = 0; // nth byte in buffer
	int s = 0; // Size of IP section

	// Initialize byte value
	uint8_t byte;

	// Parse IP string
	while (1) {

		// Get current character
		c = ip[i++];

		// End-of-string or end of section
		if (c == 0 || c == '.') {

			// Reset byte
			byte = 0;

			// Build byte using size
			for (int j = 0; j < s; j++) {

				// Get current character
				cc = ip[i - (s + 1) + j];

				// Compute byte
				byte += (cc - '0') * ((int) pow(10, (s - 1 - j)));
			}

			// Stuff it in buffer
			buffer[b++] = byte;

			// Reset size
			s = 0;
		}

		// Otherwise
		else {

			// Update section size
			s++;
		}

		// End-of-string
		if (c == 0) {

			// Exit loop
			break;
		}
	}
}



// SHOWSTRING
void showString(char *str, int str_len)
{
	// Loop on characters
	for (int i = 0; i < str_len; i++) {
		
		// Show char
		printf("%c", str[i]);
	}

	// Newline
	printf("\n");
}



// FAIL
void fail(char *msg) {

	// Error message
	perror(msg);

	// Abort
	exit(EXIT_FAILURE);
}



#endif