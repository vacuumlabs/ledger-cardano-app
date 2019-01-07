#include <os_io_seproxyhal.h>
#include <stdint.h>

#include "assert.h"
#include "os.h"
#include "errors.h"
#include "getExtendedPublicKey.h"

#define VALIDATE_PARAM(cond) if (!(cond)) THROW(ERR_INVALID_REQUEST_PARAMETERS)

get_ext_pub_key_data_t data;

void io_exchange_address();

void deriveBip32NodePrivateKey()
{
	STATIC_ASSERT(CX_APILEVEL >= 5, unsupported_api_level);
	os_memset(data.chainCode, 0, sizeof(data.chainCode));

	os_perso_derive_node_bip32(
	        CX_CURVE_Ed25519,
	        data.bip32Path,
	        data.pathLength,
	        data.privateKeyData,
	        data.chainCode);

	cx_ecfp_init_private_key(
	        CX_CURVE_Ed25519,
	        data.privateKeyData,
	        32,
	        &data.privateKey);
}

void ensureParametersAreCorrect(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength)
{
	VALIDATE_PARAM(p1 == 0);
	VALIDATE_PARAM(p2 == 0);
	VALIDATE_PARAM(dataLength >= 1);
	VALIDATE_PARAM(dataLength == dataBuffer[0] * 4 + 1 );
}

void initializePath(uint8_t *dataBuffer)
{
	STATIC_ASSERT((255-1)/4 > MAX_BIP32_PATH, __bad_length);

	data.pathLength = dataBuffer[0];

	uint8_t i = 0;
	for (i = 0; i < data.pathLength; i++) {
		uint8_t offset = 1 + 4 * i;

		data.bip32Path[i] = U4BE(dataBuffer, offset);
	}

	uint32_t bip44 = BIP_44 | HARDENED_BIP32;
	uint32_t adaCoinType = ADA_COIN_TYPE | HARDENED_BIP32;

	VALIDATE_PARAM(data.pathLength >= 3 && data.pathLength <= 10);

	VALIDATE_PARAM(data.bip32Path[0] ==  bip44);
	VALIDATE_PARAM(data.bip32Path[1] ==  adaCoinType);
	VALIDATE_PARAM(data.bip32Path[2] >= HARDENED_BIP32);
}

static void os_swap_bigint32(unsigned char *bigint)
{
	for (int i = 0; i<16; i++) {
		unsigned char x;
		x = bigint[i];
		bigint[i] = bigint[31-i];
		bigint[31-i] = x;
	}
}

// this is a workaround for SDK <= 1.5.4?, we use this instead of the cx_ecfp_generate_pair
// as these functions do not work with extended private key
void derivePublicKey()
{
	data.publicKey.W_len = 1 + 32 * 2;
	data.publicKey.W[0] = 0x04;
	os_memmove(data.publicKey.W + 1, C_cx_Ed25519_Bx, 32);
	os_memmove(data.publicKey.W + 1 + 32, C_cx_Ed25519_By, 32);
	os_swap_bigint32(data.privateKey.d);

	cx_ecfp_scalar_mult(
	        CX_CURVE_Ed25519,
	        data.publicKey.W,
	        data.publicKey.W_len,
	        data.privateKey.d, 32);
	os_swap_bigint32(data.privateKey.d);
}

void handleGetExtendedPublicKey(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength)
{
	ensureParametersAreCorrect(p1, p2, dataBuffer, dataLength);

	initializePath(dataBuffer);

	deriveBip32NodePrivateKey();
	derivePublicKey();

	os_memset(&data.privateKey, 0, sizeof(data.privateKey));
	os_memset(data.privateKeyData, 0, sizeof(data.privateKeyData));

	io_exchange_address();
}

void ui_idle();

void io_exchange_address()
{
	uint32_t tx = 0;

	G_io_apdu_buffer[tx++] = 32;

	uint8_t _publicKey[32];

	// copy public key little endian to big endian
	uint8_t i;
	for (i = 0; i < 32; i++) {
		_publicKey[i] = data.publicKey.W[64 - i];
	}

	if ((data.publicKey.W[32] & 1) != 0) {
		_publicKey[31] |= 0x80;
	}

	os_memmove(G_io_apdu_buffer + tx, _publicKey, 32);

	tx += 32;

	os_memmove(G_io_apdu_buffer + tx, data.chainCode, sizeof(data.chainCode));

	tx += sizeof(data.chainCode);

	G_io_apdu_buffer[tx++] = 0x90;
	G_io_apdu_buffer[tx++] = 0x00;

	io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
	ui_idle();
}
