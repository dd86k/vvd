/**
 * Defines __PLATFORM__, ENDIAN_BIG, and ENDIAN_LITTLE
 * 
 * Source: https://sourceforge.net/p/predef/wiki/Architectures/
 */
#if __amd64 || __amd64__ || __x86_64 || __x86_64__ || _M_AMD64 || _M_X64
	#define __PLATFORM__ "amd64"
#elif i386 || __i386 || __i386__ || _M_IX86 || __X86__ || __THW_INTEL__ || __I86__ || __INTEL__ || __386
	#define __PLATFORM__ "x86"
#elif __aarch64__
	#define __PLATFORM__ "arm64"
#elif __arm__ || __TARGET_ARCH_ARM || _ARM || _M_ARM || __arm
	#if __thumb__ || __TARGET_ARCH_THUMB || _M_ARMT
		#define __PLATFORM__ "armthumb"
	#else
		#define __PLATFORM__ "arm"
	#endif
#elif _ARCH_PPC64 || __powerpc64__
	#define __PLATFORM__ "powerpc64"
#elif _ARCH_PPC || __powerpc
	#define __PLATFORM__ "powerpc"
#elif __ia64__ || __ia64 || _M_IA64 || __itanium__
	#define __PLATFORM__ "ia64"
#elif __370__ || __THW_370__
	#define __PLATFORM__ "system/370"
#elif __s390__ || __s390x__
	#define __PLATFORM__ "system/390"
#elif __zarch__ || __SYSC_ZARCH__
	#define __PLATFORM__ "z/arch"
#elif __sparc__ || __sparc
	#define __PLATFORM__ "sparc"
#elif __sh__
	#define __PLATFORM__ "superh"
#elif __alpha__ || __alpha || _M_ALPHA
	#define __PLATFORM__ "alpha"
#elif __mips__ || __mips || __MIPS__
	#define __PLATFORM__ "mips"
#elif __m68k__ || M68000 || __MC68K__
	#define __PLATFORM__ "m68k"
#elif __bfin || __BFIN__
	#define __PLATFORM__ "blackfin"
#elif __epiphany__
	#define __PLATFORM__ "epiphany"
#elif __hppa__ || __HPPA__ || __hppa
	#define __PLATFORM__ "hp/pa"
#else
	#define __PLATFORM__ "unknown"
#endif

// Works on GCC and Clang
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	#define ENDIAN_LITTLE 1
#else
	#define ENDIAN_BIG 1
#endif