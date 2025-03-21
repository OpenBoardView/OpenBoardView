//
// Created by Thomas Lamy on 21.03.25.
//

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
