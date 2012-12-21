/* ---- Unaligned access ------------------------------------ */

#include <stdint.h>
typedef int      bb__aliased_int      ;
typedef uint16_t bb__aliased_uint16_t ;
typedef uint32_t bb__aliased_uint32_t ;

/* NB: unaligned parameter should be a pointer, aligned one -
 *  * a lvalue. This makes it more likely to not swap them by mistake
 *   */
#if defined(i386) || defined(__x86_64__) || defined(__powerpc__)
# define move_from_unaligned_int(v, intp) ((v) = *(bb__aliased_int*)(intp))
# define move_from_unaligned16(v, u16p) ((v) = *(bb__aliased_uint16_t*)(u16p))
# define move_from_unaligned32(v, u32p) ((v) = *(bb__aliased_uint32_t*)(u32p))
# define move_to_unaligned16(u16p, v)   (*(bb__aliased_uint16_t*)(u16p) = (v))
# define move_to_unaligned32(u32p, v)   (*(bb__aliased_uint32_t*)(u32p) = (v))
/* #elif ... - add your favorite arch today! */
#else
/* performs reasonably well (gcc usually inlines memcpy here) */
# define move_from_unaligned_int(v, intp) (memcpy(&(v), (intp), sizeof(int)))
# define move_from_unaligned16(v, u16p) (memcpy(&(v), (u16p), 2))
# define move_from_unaligned32(v, u32p) (memcpy(&(v), (u32p), 4))
# define move_to_unaligned16(u16p, v) do { \
    uint16_t __t = (v); \
    memcpy((u16p), &__t, 4); \
} while (0)
# define move_to_unaligned32(u32p, v) do { \
    uint32_t __t = (v); \
    memcpy((u32p), &__t, 4); \
} while (0)
#endif

/* ---- Endian Detection ------------------------------------ */

#include <limits.h>
#if defined(__digital__) && defined(__unix__)
# include <sex.h>
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) \
   || defined(__APPLE__)
# include <sys/resource.h>  /* rlimit */
# include <machine/endian.h>
# define bswap_64 __bswap64
# define bswap_32 __bswap32
# define bswap_16 __bswap16
#else
# include <byteswap.h>
# include <endian.h>
#endif

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
# define BB_BIG_ENDIAN 1
# define BB_LITTLE_ENDIAN 0
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN
# define BB_BIG_ENDIAN 0
# define BB_LITTLE_ENDIAN 1
#elif defined(_BYTE_ORDER) && _BYTE_ORDER == _BIG_ENDIAN
# define BB_BIG_ENDIAN 1
# define BB_LITTLE_ENDIAN 0
#elif defined(_BYTE_ORDER) && _BYTE_ORDER == _LITTLE_ENDIAN
# define BB_BIG_ENDIAN 0
# define BB_LITTLE_ENDIAN 1
#elif defined(BYTE_ORDER) && BYTE_ORDER == BIG_ENDIAN
# define BB_BIG_ENDIAN 1
# define BB_LITTLE_ENDIAN 0
#elif defined(BYTE_ORDER) && BYTE_ORDER == LITTLE_ENDIAN
# define BB_BIG_ENDIAN 0
# define BB_LITTLE_ENDIAN 1
#elif defined(__386__)
# define BB_BIG_ENDIAN 0
# define BB_LITTLE_ENDIAN 1
#else
# error "Can't determine endianness"
#endif

# define bb_bswap_64(x) bswap_64(x)

/* SWAP_LEnn means "convert CPU<->little_endian by swapping bytes" */
#if BB_BIG_ENDIAN
# define SWAP_BE16(x) (x)
# define SWAP_BE32(x) (x)
# define SWAP_BE64(x) (x)
# define SWAP_LE16(x) bswap_16(x)
# define SWAP_LE32(x) bswap_32(x)
# define SWAP_LE64(x) bb_bswap_64(x)
# define IF_BIG_ENDIAN(...) __VA_ARGS__
# define IF_LITTLE_ENDIAN(...)
#else
# define SWAP_BE16(x) bswap_16(x)
# define SWAP_BE32(x) bswap_32(x)
# define SWAP_BE64(x) bb_bswap_64(x)
# define SWAP_LE16(x) (x)
# define SWAP_LE32(x) (x)
# define SWAP_LE64(x) (x)
# define IF_BIG_ENDIAN(...)
# define IF_LITTLE_ENDIAN(...) __VA_ARGS__
#endif


