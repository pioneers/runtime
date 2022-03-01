#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <string.h>

/*
* Creates the public and private key pair.
* Stores the public key and private key as a PEM 
*/
int create_public_key(char** public_key_string);

/**
 * Initializes the memory BIO that will be used to store the private key
 * Skips the initialization process if already done
 * Returns: an int indicating if the BIO creation was successful or not
 *      0 if successfully initialized
 *     -1 if an error occured
 */
int initialize_BIO();

/**
 * Takes the encrypted message sent by Dawn and decrypts the message using the private key stored in the BIO
 * Returns: char* that was passed in containing the decrypted message and an int where:
 *      0 if successfully decrypted
 *     -1 if an error occured
 */
int decrypt_login_info(char* encrypted_message, char* decrypted_message);

/**
 * Appends the salt located on the raspberry pi to the passed in string.
 * Returns: char* that was passed in containing the salted message and an int where:
 *      0 if successfully salted
 *     -1 if an error occured
 */
int salt_string(char* message, char* salted_message);

/**
 * Hashes the passed in salted message using OPENSSL's SHA256 functions
 * Returns: a hashed salted message and an int where:
 *      0 if successfully hashed
 *     -1 if an error occured
 */
int hash_message(char* salted_message, unsigned char* hashed_salted_message);

/**
 * Compares the passed in hashed salted message to the hashed salted password on file
 * Returns: an int where:
 *      0 if the two are the same
 *     -1 if an error occured
 *      1 if the passwords are different
 */
int compare_hashed_password(char* hashed_salted_message);