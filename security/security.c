#include "security.h"

// BIO's are OPENSSL's own implementation of I/O channels. Used in this case to store the private key in a temporary spot without needing to write out to a file
BIO* mem = NULL;

int initialize_BIO() {
    if (mem == NULL) {
        mem = BIO_new(BIO_s_mem());
        if (mem == NULL) {
            return -1;
        }
    }
    return 0;
}

int create_public_key(char** public_key_string) {
    EVP_PKEY_CTX* ctx;      // context holding info related to public key encryption
    EVP_PKEY* pkey = NULL;  // public key from a private/public key pair

    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (ctx == NULL) {
        return -1;
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        return -1;
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) {
        return -1;
    }

    /* Generate key */
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
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
    *public_key_string = malloc(500 * sizeof(char));
    if (*public_key_string == NULL) {
        free(*public_key_string);
        return -1;
    }
    fread(*public_key_string, 500, sizeof(char), read_from_key);
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);
    return 0;
}

int decrypt_login_info(char* encrypted_message, char* decrypted_message) {
    EVP_PKEY_CTX* ctx = NULL;
    EVP_PKEY* private_key = NULL;

    private_key = PEM_read_bio_PrivateKey(mem, NULL, NULL, NULL);

    if (private_key == NULL) {
        return -1;
    }

    ctx = EVP_PKEY_CTX_new(private_key, NULL);

    if (ctx == NULL) {
        return -1;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        return -1;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        return -1;
    }

    unsigned char* decryption_buffer;
    size_t decryption_buffer_len;
    size_t encrypted_message_len = strlen(encrypted_message);

    if (EVP_PKEY_decrypt(ctx, NULL, &decryption_buffer_len, (unsigned char*) encrypted_message, encrypted_message_len) <= 0) {
        return -1;
    }

    decryption_buffer = OPENSSL_malloc(decryption_buffer_len);

    if (decryption_buffer == NULL) {
        return -1;
    }

    if (EVP_PKEY_decrypt(ctx, decryption_buffer, &decryption_buffer_len, (unsigned char*) encrypted_message, encrypted_message_len) <= 0) {
        return -1;
    }

    decrypted_message = malloc(decryption_buffer_len);

    for (int i = 0; i < decryption_buffer_len - 1; i++) {
        decrypted_message[i] = (char) *(decryption_buffer + i);
    }

    decrypted_message[decryption_buffer_len - 1] = '\0';

    OPENSSL_free(decrypted_message);
    EVP_PKEY_free(private_key);
    EVP_PKEY_CTX_free(ctx);
    return 0;
}

int salt_string(char* message, char* salted_message) {
    FILE* salt_file = fopen("salt.txt", "r");
    fseek(salt_file, 0L, SEEK_END);
    size_t salt_len = ftell(salt_file);
    size_t message_length = strlen(message);
    salted_message = malloc(salt_len + message_length + 1);
    if (salted_message == NULL) {
        return -1;
    }
    for (int i = 0; i < message_length; i++) {
        salted_message[i] = message[i];
    }
    for (int i = message_length; i < message_length + salt_len; i++) {
        salted_message[i] = message[i - message_length];
    }
    fclose(salt_file);
    return 0;
}

int hash_message(char* salted_message, unsigned char* hashed_salted_message) {
    EVP_MD_CTX* hash_ctx = NULL;
    hash_ctx = EVP_MD_CTX_new();
    if (EVP_DigestInit_ex(hash_ctx, EVP_sha256(), NULL) != 1) {
        return -1;
    }
    hashed_salted_message = OPENSSL_malloc(EVP_MD_size(EVP_sha256()));
    if (hashed_salted_message == NULL) {
        OPENSSL_free(hashed_salted_message);
        return -1;
    }
    if (EVP_DigestUpdate(hash_ctx, salted_message, strlen(salted_message)) == 0) {
        return -1;
    }
    unsigned int max_hashed_len = EVP_MD_size(EVP_sha256());
    if (EVP_DigestFinal_ex(hash_ctx, hashed_salted_message, &max_hashed_len) == 0) {
        return -1;
    }
    EVP_MD_CTX_destroy(hash_ctx);
    return 0;
}

int compare_hashed_password(char* hashed_salted_message) {
    FILE* hashed_on_pi;
    hashed_on_pi = fopen("hashed.txt", "r");
    if (hashed_on_pi == NULL) {
        fclose(hashed_on_pi);
        return -1;
    }
    fseek(hashed_on_pi, 0L, SEEK_END);
    int bufsize = ftell(hashed_on_pi);
    fseek(hashed_on_pi, 0L, SEEK_SET);
    char hashed_pass[bufsize];
    size_t newLen = fread(hashed_pass, sizeof(char), bufsize, hashed_on_pi);
    hashed_pass[newLen++] = '\0';
    return strcmp(hashed_pass, hashed_salted_message);
}