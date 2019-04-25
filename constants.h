#ifndef CONSTANTS_H
#define CONSTANTS_H

#define LOCALHOST            "127.0.0.1" // Localhost
#define HEADER_SIZE_EXTERNAL 6           // Number of bytes in header of each [external] packet
#define HEADER_SIZE_INTERNAL 14          // Number of bytes in header of each [internal] packet
#define KEY_LENGTH_SIZE      2           // Number of bytes for key length
#define VALUE_LENGTH_SIZE    2           // Number of bytes for value length
#define MAX_HEADER_SIZE      14
#define MAX_KEY_SIZE         65535       // Max key size in bytes (2 ** (KEY_LENGTH_SIZE * 8) - 1)
#define MAX_VALUE_SIZE       65535       // Max value size in bytes (2 ** (VALUE_LENGTH_SIZE * 8) - 1)
#define BUFFER_SIZE          131084      // Size of buffers in bytes MAX_HEADER_SIZE + MAX_KEY_SIZE + MAX_VALUE_SIZE
#define INTERNAL             (1 << 7)
#define ACK                  (1 << 3)
#define GET                  (1 << 2)
#define SET                  (1 << 1)
#define DELETE               (1 << 0)

#endif