/*
 * Data Encryption Standard
 * An approach to DES algorithm
 *
 * By: Daniel Huertas Gonzalez
 * Email: huertas.dani@gmail.com
 * Version: 0.1
 *
 * Based on the document FIPS PUB 46-3
 *
 * Source: https://github.com/dhuertas/DES
 */
#ifndef DES_H
#define DES_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The DES function
 * input: 64 bit message
 * key: 64 bit key for encryption/decryption
 * mode: 'e' = encryption; 'd' = decryption
 */
uint64_t des(uint64_t input, uint64_t key, char mode);

#ifdef __cplusplus
}
#endif
#endif // DES_H
