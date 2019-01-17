#include "hex_utils.h"
#include "stream.h"
#include "utils.h"
#include "assert.h"
#include "messageSigning.h"
#include "stream.h"
#include "endian.h"

void signMessage(privateKey_t* privateKey,
                 const uint8_t* messageBuffer, size_t messageSize,
                 uint8_t* outBuffer, size_t outSize)
{
	uint8_t signature[64];
	ASSERT(messageSize < BUFFER_SIZE_PARANOIA);
	ASSERT(outSize == SIZEOF(signature));

	size_t signatureSize = cx_eddsa_sign(privateKey,
	                                     0,
	                                     CX_SHA512,
	                                     messageBuffer,
	                                     messageSize,
	                                     NULL,
	                                     0,
	                                     &signature,
	                                     SIZEOF(signature),
	                                     0);

	ASSERT(signatureSize == 64);

	os_memmove(outBuffer, &signature, signatureSize);
}

void getTxWitness(path_spec_t* pathSpec,
                  const uint8_t* txHashBuffer, size_t txHashSize,
                  uint8_t* outBuffer, size_t outSize)
{
	ASSERT(txHashSize == 32);

	chain_code_t chainCode;
	privateKey_t privateKey;

	derivePrivateKey(pathSpec, &chainCode, &privateKey);

	uint8_t messageBuffer[32 + 8];
	// magic
	u8be_write(messageBuffer, 0x011a2d964a095820);
	os_memmove(messageBuffer + 8, txHashBuffer, txHashSize);

	signMessage(&privateKey, &messageBuffer, 8 + txHashSize, outBuffer, outSize);
}