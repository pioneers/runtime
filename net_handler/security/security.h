#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <string.h>
#include <openssl/pem.h>

/**
 * Initializes the memory BIO that will be used to store the private key
 * Skips the initialization process if already done
 * Returns: an int indicating if the BIO creation was successful or not
 *      0 if successfully initialized
 *     -1 if an error occured
 */
int initialize_BIO();

/*
* Creates the public and private key pair.
* Stores the public key and private key as a PEM 
*/
int create_public_key(char** public_key_string);

/**
 * Takes the encrypted message sent by Dawn and decrypts the message using the private key stored in the BIO
 * Returns: char* that was passed in containing the decrypted message and an int where:
 *      0 if successfully decrypted
 *     -1 if an error occured
 */
int decrypt_login_info(unsigned char* encrypted_message, size_t encrypted_message_len, char** decrypted_message);

/**
 * Appends the salt located on the raspberry pi to the passed in string.
 * Returns: char* that was passed in containing the salted message and an int where:
 *      0 if successfully salted
 *     -1 if an error occured
 */
int salt_string(char* message, char** salted_message);

/**
 * Generates a random string to be appended based on the original message length
 * Returns: char* to be appended
 */
char* generateRandomString(int size);


/**
 * Generates a random string to be appended to the original message 
 * and writes to salt.txt located on the raspberry pi
 * Returns: char* to be appended
 */
int create_salt_string(char* message);

/**
 * Hashes the passed in salted message using OPENSSL's SHA256 functions
 * Returns: a hashed salted message and an int where:
 *      0 if successfully hashed
 *     -1 if an error occured
 */
int hash_message(char* salted_message, unsigned char** hashed_salted_message);


/**
 * Prints the hashed binary data in a hexadecimal representation so we can put null terminator at the end.
 * Returns: hex representation of hashed_salted_message
 */
char* printable_hash(unsigned char* hashed_salted_message);

/**
 * Verifies the signature that is sent from dawn to runtime using the public key that matches the private key 
 * which dawn used to sign its password
 * Returns: verification result where:
 *      1 if signature verification successful
 *      0 if signature verification failed
 *      -1 if there was an error during signature verification
 */
int verify_signature(const unsigned char* message, size_t message_len, const unsigned char* signature, size_t signature_len, const char* public_key_file);