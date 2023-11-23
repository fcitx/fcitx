#ifndef __FCITX_ENDIAN_H
#define __FCITX_ENDIAN_H

/* from https://gist.github.com/yinyin/2027912 */

/**
 * A simple compatibility layer to convert
 * Linux endian macros to OS X equivalents
 * It is public domain.
 **/

#ifndef __APPLE__
#error "This file: endian.h is OS X specific.\n"
#endif /* __APPLE__ */

#include <architecture/byte_order.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
 
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
 
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#endif // __FCITX_ENDIAN_H
