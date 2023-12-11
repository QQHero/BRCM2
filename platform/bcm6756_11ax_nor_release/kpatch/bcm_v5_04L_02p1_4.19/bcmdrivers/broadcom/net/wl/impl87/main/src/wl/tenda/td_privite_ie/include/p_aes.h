/**
 * \file aes.h
 */
#ifndef _AES_H_
#define _AES_H_


void IE_AesCbcDecrypt(char *input, char *output, int len,char *key , unsigned int *Decrypt_len);

unsigned int IE_AesCbcEncrypt(char *input, char *output, int len, char *key);


#endif /* aes.h */
