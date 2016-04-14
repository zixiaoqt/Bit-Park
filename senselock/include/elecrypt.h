#ifndef __ELECRYPT_H__
#define __ELECRYPT_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined WIN32 || defined _WIN32 || defined _WIN64
#include <windows.h>
#else
typedef unsigned char  UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
#define WINAPI
#endif

// define ses error code
#define SES_SUCCESS                 0x0000          // success
#define SES_ERROR_PARAM             0x9204          // parameter error

// define aes calculate flag
#define AES_CRYPT_MASK				0x02			// bit mask of the aes crypt
#define AES_CRYPT_ENC				0x00			// AES encrypt
#define AES_CRYPT_DEC				0x02			// AES decrypt

// define des calculate flag
#define DES_ALGO_MASK               0x01            // bit mask of the des algorithm
#define DES_ALGO_SINGLE             0x00            // single des
#define DES_ALGO_TRIPLE             0x01            // triple des
#define DES_CRYPT_MASK              0x02            // bit mask of the des crypt
#define DES_CRYPT_ENC               0x00            // DES encrypt
#define DES_CRYPT_DEC               0x02            // DES decrypt

// define hash algorithm flag
#define HASH_ALGO_MASK              0x02            // bit mask of the hash algorithm
#define HASH_ALGO_SHA1              0x00            // sha1
#define HASH_ALGO_MD5               0x02            // md5

// define rsa calculate mode
#define RSA_MODE_NORMAL             0x00            // crypt directly
#define RSA_MODE_PKCS               0x01            // pkcs standard
#define RSA_MODE					0x01			// mode mask

// define digist context for hash calculate
typedef struct __DIGIST_CONTEXT
{
    ULONG   aulState[5];                            // store the result of each block calculate
    ULONG   aulCount[2];                            // store the bit length of the data
    UCHAR   aucBuffer[64];                          // store the remainder data of the last block
}DIGIST_CONTEXT;

// define hash context, do not modify the the value in of the structure
typedef struct __HASH_CONTEXT
{
    UCHAR ucAlgorithm;                              // store the hash algorithm
    DIGIST_CONTEXT DgtCtx[1];                          // digist context
}HASH_CONTEXT;

// define HMAC context, do not modify the the value in of the structure
typedef struct __HMAC_CONTEXT
{
    UCHAR ucAlgorithm;                              // store the hash algorithm
    DIGIST_CONTEXT DgtCtx[2];                       // digist context
}HMAC_CONTEXT;

// define RSA public key struct
typedef struct __RSA_PUB_KEY
{
	UCHAR	aucN[128];								// the modulus
	UCHAR	aucE[4];								// the exponent
}RSA_PUB_KEY;

// define RSA private key struct
typedef struct __RSA_PRI_KEY
{
	UCHAR	aucP[64];								// the larger prime
	UCHAR	aucQ[64];								// the smaller prime
	UCHAR	aucDp[64];								// (E^-1) mod (P-1)
	UCHAR	aucDq[64];								// (E^-1) mod (Q-1)
	UCHAR	aucQinv[64];							// (Q^-1) mod P
}RSA_PRI_KEY;

// AES crypt function, [ucFlag] referent aes flag definition
USHORT WINAPI aes_ecb(UCHAR *pucKey, UCHAR *pucIn, UCHAR *pucOut, USHORT usLen, UCHAR ucFlag);               // AES calculate ECB mode
USHORT WINAPI aes_cbc(UCHAR *pucKey, UCHAR *pucIn, UCHAR *pucOut, USHORT usLen, UCHAR *pucIV, UCHAR ucFlag); // AES calculate CBC mode

// DES crypt function, [ucFlag] referent des flag definition
USHORT WINAPI des_ecb(UCHAR *pucKey, UCHAR *pucIn, UCHAR *pucOut, USHORT usLen, UCHAR ucFlag);                // DES calculate ECB mode
USHORT WINAPI des_cbc(UCHAR *pucKey, UCHAR *pucIn, UCHAR *pucOut, USHORT usLen, UCHAR *pucIV, UCHAR ucFlag);  // DES calculate CBC mode
USHORT WINAPI des_mac(UCHAR *pucKey, UCHAR *pucIn, UCHAR *pucOut, USHORT usLen, UCHAR *pucIV, UCHAR ucFlag);  // DES mac, return 8 bytes

// standard hash interface, [ucAlgo] see hash algorithm definition
USHORT WINAPI hash_init(HASH_CONTEXT *pCtx, UCHAR ucAlgo);                                                    // sha1 initialize context
USHORT WINAPI hash_update(HASH_CONTEXT *pCtx, UCHAR *pucData, USHORT usLen);                                  // sha1 update data
USHORT WINAPI hash_final(HASH_CONTEXT *pCtx, UCHAR *pucData);                                                 // sha1 final, getting hash result

// HMAC function, [ucAlgo] see hash mode define
USHORT WINAPI hash_mac_init(HMAC_CONTEXT *pCtx, UCHAR *pucKey, USHORT usLen, UCHAR ucAlgo);                   // initial key
USHORT WINAPI hash_mac_update(HMAC_CONTEXT *pCtx, UCHAR *pucData, USHORT usLen);                              // update date
USHORT WINAPI hash_mac_final(HMAC_CONTEXT *pCtx, UCHAR *pucData);                                             // get result

// RSA calculation function, [ucMode] see rsa mode define
USHORT WINAPI rsa_pub_encrypt(RSA_PUB_KEY *pKey, UCHAR *pucIn, UCHAR *pucOut, USHORT usLen, UCHAR ucMode);        // RSA public key encryption
USHORT WINAPI rsa_pub_decrypt(RSA_PUB_KEY *pKey, UCHAR *pucIn, UCHAR *pucOut, USHORT *pusLen, UCHAR ucMode);      // RSA public key decryption
USHORT WINAPI rsa_pri_encrypt(RSA_PRI_KEY *pKey, UCHAR *pucIn, UCHAR *pucOut, USHORT usLen, UCHAR ucMode);        // RSA private key encryption
USHORT WINAPI rsa_pri_decrypt(RSA_PRI_KEY *pKey, UCHAR *pucIn, UCHAR *pucOut, USHORT *pusLen, UCHAR ucMode);      // RSA private key decryption

// RSA key generate functions
USHORT WINAPI rsa_genkey(RSA_PUB_KEY *pPubKey, RSA_PRI_KEY *pPriKey);

#ifdef __cplusplus
}
#endif


#endif //__ELECRYPT_H__
