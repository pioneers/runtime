#include "security.h"

#define HASHLEN 32
#define BUFLEN 1024

// BIO's are OPENSSL's own implementation of I/O channels. Used in this case to store the private key in a temporary spot without needing to write out to a file
BIO* mem = NULL;


/**
 * Initializes the memory BIO that will be used to store the private key
 * Skips the initialization process if already done
 * Returns: an int indicating if the BIO creation was successful or not
 *      0 if successfully initialized
 *     -1 if an error occured
 */
int initialize_BIO() {
    if (mem == NULL) {
        mem = BIO_new(BIO_s_mem());
        if (mem == NULL) {
            return -1;
        }
    }
    return 0;
}

/*
* Creates the public and private key pair.
* Stores the public key and private key as a PEM 
*/
int create_public_key(char** public_key_string) {
    EVP_PKEY_CTX* ctx;      // context holding info related to public key encryption
    EVP_PKEY* pkey = NULL;  // public key from a private/public key pair

    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL); // allocates public key algorithm context using the key type
    if (ctx == NULL) {
        return -1;
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0) { // key generation operation
        return -1;
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) { //RSA key length of 2048 bits
        return -1;
    }

    /* Generate key */
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) { //generated key written to pkey
        return -1;
    }

    // store the private key in OPENSSL's temporary memory buffer
    PEM_write_bio_PrivateKey(mem, pkey, NULL, NULL, 0, 0, NULL);

    FILE* public_key = fopen("keys.pem", "w+");
    PEM_write_PUBKEY(public_key, pkey);
    fclose(public_key);

    FILE* read_from_key = fopen("keys.pem", "r");
    if (read_from_key == NULL) {
        return -1;
    }

    *public_key_string = malloc(BUFLEN * sizeof(char));
    if (*public_key_string == NULL) {
        free(*public_key_string);
        return -1;
    }

    fread(*public_key_string, BUFLEN, sizeof(char), read_from_key);

    fclose(read_from_key);
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);
    return 0;
}

/**
 * Takes the encrypted message sent by Dawn and decrypts the message using the private key stored in the BIO
 * Returns: char* that was passed in containing the decrypted message and an int where:
 *      0 if successfully decrypted
 *     -1 if an error occured
 */
int decrypt_login_info(unsigned char* encrypted_message, size_t encrypted_message_len, char** decrypted_message) {
    EVP_PKEY_CTX* ctx = NULL;
    EVP_PKEY* private_key = NULL;
    unsigned char* decryption_buffer = NULL;
    size_t decryption_buffer_len;

    private_key = PEM_read_bio_PrivateKey(mem, NULL, NULL, NULL);
    if (private_key == NULL) {
        fprintf(stderr, "Error reading private key\n");
        return -1;
    }

    ctx = EVP_PKEY_CTX_new(private_key, NULL);
    if (ctx == NULL) {
        fprintf(stderr, "Error creating EVP_PKEY_CTX\n");
        EVP_PKEY_free(private_key);
        return -1;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        fprintf(stderr, "Error initializing decryption\n");
        EVP_PKEY_free(private_key);
        EVP_PKEY_CTX_free(ctx);
        return -1;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        fprintf(stderr, "Error setting RSA padding\n");
        EVP_PKEY_free(private_key);
        EVP_PKEY_CTX_free(ctx);
        return -1;
    }

    if (EVP_PKEY_decrypt(ctx, NULL, &decryption_buffer_len, encrypted_message, encrypted_message_len) <= 0) {
        fprintf(stderr, "Error determining decryption buffer size\n");
        EVP_PKEY_free(private_key);
        EVP_PKEY_CTX_free(ctx);
        return -1;
    }

    decryption_buffer = OPENSSL_malloc(decryption_buffer_len);
    if (decryption_buffer == NULL) {
        fprintf(stderr, "Error allocating memory for decryption buffer\n");
        EVP_PKEY_free(private_key);
        EVP_PKEY_CTX_free(ctx);
        return -1;
    }

    if (EVP_PKEY_decrypt(ctx, decryption_buffer, &decryption_buffer_len, encrypted_message, encrypted_message_len) <= 0) {
        fprintf(stderr, "Error decrypting message\n");
        ERR_print_errors_fp(stderr); // Print OpenSSL error stack
        OPENSSL_free(decryption_buffer);
        EVP_PKEY_free(private_key);
        EVP_PKEY_CTX_free(ctx);
        return -1;
    }


    *decrypted_message = malloc(decryption_buffer_len + 1);
    if (*decrypted_message == NULL) {
        fprintf(stderr, "Error allocating memory for decrypted message\n");
        OPENSSL_free(decryption_buffer);
        EVP_PKEY_free(private_key);
        EVP_PKEY_CTX_free(ctx);
        return -1;
    }

    memcpy(*decrypted_message, decryption_buffer, decryption_buffer_len);
    (*decrypted_message)[decryption_buffer_len] = '\0';

    OPENSSL_free(decryption_buffer);
    EVP_PKEY_free(private_key);
    EVP_PKEY_CTX_free(ctx);
    return 0;
}

/**
 * Appends the salt located on the raspberry pi to the passed in string.
 * Returns: char* that was passed in containing the salted message and an int where:
 *      0 if successfully salted
 *     -1 if an error occured
 */
int salt_string(char* message, char** salted_message) {
    FILE* salt_file = fopen("salt.txt", "r");
    char buf[BUFLEN];
    fgets(buf, BUFLEN, salt_file);

    int salt_len = strlen(buf);
    int message_length = strlen(message);
    
    *salted_message = malloc(salt_len + message_length + 1);
    if (*salted_message == NULL) {
        return -1;
    }
    for (int i = 0; i < message_length; i++) {
        (*salted_message)[i] = message[i];
    }
    for (int i = message_length; i < message_length + salt_len; i++) {
        (*salted_message)[i] = buf[i - message_length];
    }
    fclose(salt_file);
    return 0;
}

/**
 * Generates a random string to be appended based on the original message length.
 * Returns: char* to be appended.
 */
char* generateRandomString(int size) {
    char* randomString = malloc((size + 1) * sizeof(char)); // Allocate memory for the string (+1 for null terminator)
    const char characters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; // Pool of characters
    
    srand(time(NULL)); // Initialize the random number generator
    
    for (int i = 0; i < size; i++) {
        int randomIndex = rand() % (sizeof(characters) - 1); // Generate a random index within the character pool
        randomString[i] = characters[randomIndex]; // Select a random character
    }
    
    randomString[size] = '\0'; // Null-terminate the string
    
    return randomString;
}

/**
 * Generates a random string to be appended to the original message that is 
 * 32 - message length and writes to salt.txt located on the raspberry pi.
 * Returns: char* to be appended where:
 *      0 if successfully created file
 *      -1 otherwise
 */
int create_salt_string(char* message) {
    FILE* salt_file = fopen("salt.txt", "w+");
    int message_length = strlen(message);

    char* salt_string = generateRandomString(HASHLEN - message_length);
    fwrite(salt_string, sizeof(char), HASHLEN - message_length, salt_file);

    free(salt_string);
    fclose(salt_file);
    return 0;
}


/**
 * Hashes the passed in salted message using OPENSSL's SHA256 functions
 * Returns: a hashed salted message and an int where:
 *      0 if successfully hashed
 *     -1 if an error occured
 */
int hash_message(char* salted_message, unsigned char** hashed_salted_message) {
    EVP_MD_CTX* hash_ctx = NULL;
    hash_ctx = EVP_MD_CTX_new();
    if (EVP_DigestInit_ex(hash_ctx, EVP_sha256(), NULL) != 1) {
        return -1;
    }
    *hashed_salted_message = OPENSSL_malloc(EVP_MD_size(EVP_sha256()));
    if (*hashed_salted_message == NULL) {
        OPENSSL_free(*hashed_salted_message);
        return -1;
    }
    if (EVP_DigestUpdate(hash_ctx, salted_message, strlen(salted_message)) == 0) {
        return -1;
    }
    unsigned int max_hashed_len = EVP_MD_size(EVP_sha256());
    if (EVP_DigestFinal_ex(hash_ctx, *hashed_salted_message, &max_hashed_len) == 0) {
        return -1;
    }
    EVP_MD_CTX_destroy(hash_ctx);
    return 0;
}

/**
 * Prints the hashed binary data in a hexadecimal representation so we can put null terminator at the end.
 * Returns: hex representation of hashed_salted_message
 */
char* printable_hash(unsigned char* hashed_salted_message) {
    char* hex_hash = malloc((2 * EVP_MD_size(EVP_sha256()) + 1) * sizeof(char));
    for (int i = 0; i < EVP_MD_size(EVP_sha256()); i++) {
        sprintf(&hex_hash[i * 2], "%02x", hashed_salted_message[i]);
    }
    hex_hash[2 * EVP_MD_size(EVP_sha256())] = '\0';

    return hex_hash;
}

/**
 * Verifies the signature that is sent from dawn to runtime using the public key that matches the private key 
 * which dawn used to sign its password
 * Returns: verification result where:
 *      1 if signature verification successful
 *      0 if signature verification failed
 *      -1 if there was an error during signature verification
 */
int verify_signature(const unsigned char* message, size_t message_len, const unsigned char* signature, size_t signature_len, const char* public_key_file) {
    FILE* public_key_file_ptr = fopen(public_key_file, "r");
    if (public_key_file_ptr == NULL) {
        fprintf(stderr, "Error opening public key file");
        return -1;
    }

    EVP_PKEY* public_key = PEM_read_PUBKEY(public_key_file_ptr, NULL, NULL, NULL);
    fclose(public_key_file_ptr);

    if (public_key == NULL) {
        fprintf(stderr, "Error reading public key\n");
        return -1;
    }

    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    if (md_ctx == NULL) {
        fprintf(stderr, "Error creating EVP_MD_CTX\n");
        EVP_PKEY_free(public_key);
        return -1;
    }

    if (EVP_DigestVerifyInit(md_ctx, NULL, EVP_sha256(), NULL, public_key) <= 0) {
        fprintf(stderr, "Error initializing signature verification\n");
        EVP_PKEY_free(public_key);
        EVP_MD_CTX_free(md_ctx);
        return -1;
    }

    if (EVP_DigestVerifyUpdate(md_ctx, message, message_len) <= 0) {
        fprintf(stderr, "Error updating signature verification\n");
        EVP_PKEY_free(public_key);
        EVP_MD_CTX_free(md_ctx);
        return -1;
    }

    int verification_result = EVP_DigestVerifyFinal(md_ctx, signature, signature_len);

    if (verification_result == 1) {
        printf("Signature verification succeeded.\n");
    } else if (verification_result == 0) {
        printf("Signature verification failed.\n");
    } else {
        fprintf(stderr, "Error during signature verification\n");
    }

    EVP_PKEY_free(public_key);
    EVP_MD_CTX_free(md_ctx);

    return verification_result;
}