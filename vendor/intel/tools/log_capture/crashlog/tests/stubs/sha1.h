#ifndef __SHA1STUB_H__
#define __SHA1STUB_H__

#define SHA1_DIGEST_LENGTH		20
#define SHA1_DIGEST_STRING_LENGTH	41

typedef struct {
	unsigned int state[5];
	unsigned int count[2];
	unsigned char buffer[64];
} SHA1_CTX;

void SHA1Update(void *sha, unsigned char* buf, int size);
void SHA1Final(unsigned char results[SHA1_DIGEST_LENGTH], void *sha);
void SHA1Init(void *sha);

#endif /* __SHA1STUB_H__ */
