#ifndef _CRYPTO_H_
#define _CRYPTO_H_

#include "utility/bytebuffer.h"
#include "base64.h"
#include <string>

#define BLOCK_SIZE 16

void CryptoInit();

void SHA256_digest(const string& data, string& digest);
void SHA256_digest(const char* data, int length,
                   string& digest);

int GenerateRandomKey(ByteBuffer &key, int size);
int GenerateRandomData(ByteBuffer &key, int size);

void Xor(ByteBuffer& data, const ByteBuffer& key);

int  AesEncrypt(const ByteBuffer &key,
               const ByteBuffer &plaintext,
               ByteBuffer &ciphertext);

int  AesDecrypt(const ByteBuffer &key,
               const ByteBuffer &ciphertext,
               ByteBuffer &plaintext);

int RsaEncrypt(const std::string key,
               const ByteBuffer &plaintext,
               ByteBuffer &ciphertext);

int RsaDecrypt(const std::string key_filename,
               const char* password,
               const ByteBuffer &ciphertext,
               ByteBuffer &plaintext);

bool RsaTest(const char* pem_file,
             const char* pub_file,
             const char* pwd);

#endif



