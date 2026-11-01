/*
 * The interface the cipher sources in this directory offer to MCryptPP:
 * libmcrypt's module functions (_mcrypt_set_key, _mcrypt_encrypt, ...),
 * collected into one descriptor per algorithm. See README.md.
 */

#ifndef MCRYPT_ALGORITHM_H
#define MCRYPT_ALGORITHM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mcrypt_algorithm {
	/* the name steghide and libmcrypt use, e.g. "rijndael-128" */
	const char *name;
	int (*is_block_algorithm) (void);
	/* 1 for stream algorithms */
	int (*get_block_size) (void);
	/* the maximum key size */
	int (*get_key_size) (void);
	/* NULL (and *len 0) if every size up to the maximum is supported */
	const int *(*get_supported_key_sizes) (int *len);
	/* the size of the key schedule (the context the other functions take) */
	int (*get_size) (void);
	int (*self_test) (void);

	/* block algorithms: en-/decrypt one block in place */
	int (*block_set_key) (void *ctx, const void *key, int len);
	void (*block_encrypt) (void *ctx, void *block);
	void (*block_decrypt) (void *ctx, void *block);

	/* stream algorithms: en-/decrypt len bytes in place */
	int (*stream_get_iv_size) (void);
	int (*stream_set_key) (void *ctx, const void *key, int len, const void *iv, int ivlen);
	void (*stream_encrypt) (void *ctx, void *buf, int len);
	void (*stream_decrypt) (void *ctx, void *buf, int len);
} mcrypt_algorithm;

/*
 * Used once at the end of each cipher source, where _mcrypt_set_key and the
 * others are macros for that module's own functions. The wrappers convert
 * between the generic signatures above and the module's parameter types.
 */
#define MCRYPT_BLOCK_ALGORITHM(id, algoname) \
	static int id##_set_key(void *ctx, const void *key, int len) \
		{ return _mcrypt_set_key(ctx, (void *) key, len); } \
	static void id##_encrypt(void *ctx, void *block) { _mcrypt_encrypt(ctx, block); } \
	static void id##_decrypt(void *ctx, void *block) { _mcrypt_decrypt(ctx, block); } \
	static int id##_is_block_algorithm(void) { return _is_block_algorithm(); } \
	static int id##_get_block_size(void) { return _mcrypt_get_block_size(); } \
	static int id##_get_key_size(void) { return _mcrypt_get_key_size(); } \
	static int id##_get_size(void) { return _mcrypt_get_size(); } \
	static int id##_self_test(void) { return _mcrypt_self_test(); } \
	const mcrypt_algorithm mcrypt_##id = { algoname, id##_is_block_algorithm, \
		id##_get_block_size, id##_get_key_size, _mcrypt_get_supported_key_sizes, \
		id##_get_size, id##_self_test, id##_set_key, id##_encrypt, id##_decrypt, \
		NULL, NULL, NULL, NULL };

#define MCRYPT_STREAM_ALGORITHM(id, algoname) \
	static int id##_set_key(void *ctx, const void *key, int len, const void *iv, int ivlen) \
		{ return _mcrypt_set_key(ctx, (void *) key, len, (void *) iv, ivlen); } \
	static void id##_encrypt(void *ctx, void *buf, int len) { _mcrypt_encrypt(ctx, buf, len); } \
	static void id##_decrypt(void *ctx, void *buf, int len) { _mcrypt_decrypt(ctx, buf, len); } \
	static int id##_is_block_algorithm(void) { return _is_block_algorithm(); } \
	static int id##_get_block_size(void) { return _mcrypt_get_block_size(); } \
	static int id##_get_key_size(void) { return _mcrypt_get_key_size(); } \
	static int id##_get_size(void) { return _mcrypt_get_size(); } \
	static int id##_self_test(void) { return _mcrypt_self_test(); } \
	static int id##_get_iv_size(void) { return _mcrypt_get_algo_iv_size(); } \
	const mcrypt_algorithm mcrypt_##id = { algoname, id##_is_block_algorithm, \
		id##_get_block_size, id##_get_key_size, _mcrypt_get_supported_key_sizes, \
		id##_get_size, id##_self_test, NULL, NULL, NULL, \
		id##_get_iv_size, id##_set_key, id##_encrypt, id##_decrypt };

/* in the order libmcrypt 2.5.8 lists them, which is the order of steghide's encinfo */
extern const mcrypt_algorithm mcrypt_cast_128, mcrypt_gost, mcrypt_rijndael_128,
	mcrypt_twofish, mcrypt_arcfour, mcrypt_cast_256, mcrypt_loki97,
	mcrypt_rijndael_192, mcrypt_saferplus, mcrypt_wake, mcrypt_des,
	mcrypt_rijndael_256, mcrypt_serpent, mcrypt_xtea, mcrypt_blowfish,
	mcrypt_enigma, mcrypt_rc2, mcrypt_tripledes;

#ifdef __cplusplus
}
#endif

#endif /* MCRYPT_ALGORITHM_H */
