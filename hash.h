// hash.h -- Paul Hsieh's super fast hash function (http://www.azillionmonkeys.com/qed/hash.html)
#ifndef MINT_HASH_H
#define MINT_HASH_H

#include <stdint.h>

uint32_t SuperFastHash(const char * data, int len);

#endif
