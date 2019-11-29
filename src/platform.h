/**
 * Platform architecture string for printing purposes only
 * 
 * Source: https://sourceforge.net/p/predef/wiki/Architectures/
 */
#if __amd64 || __amd64__ || __x86_64 || __x86_64__ || _M_AMD64 || _M_X64
	#define PLATFORM "amd64"
#elif i386 || __i386 || __i386__ || _M_IX86 || __X86__ || __THW_INTEL__ || __I86__ || __INTEL__ || __386
	#define PLATFORM "x86"
#elif __aarch64__
	#define PLATFORM "arm64"
#elif __arm__ || __TARGET_ARCH_ARM || _ARM || _M_ARM || __arm
	#if __thumb__ || __TARGET_ARCH_THUMB || _M_ARMT
		#define PLATFORM "armthumb"
	#else
		#define PLATFORM "arm"
	#endif
#elif _ARCH_PPC64 || __powerpc64__
	#define PLATFORM "powerpc64"
#elif _ARCH_PPC || __powerpc
	#define PLATFORM "powerpc"
#elif __ia64__ || __ia64 || _M_IA64 || __itanium__
	#define PLATFORM "ia64"
#elif __370__ || __THW_370__
	#define PLATFORM "system/370"
#elif __s390__ || __s390x__
	#define PLATFORM "system/390"
#elif __zarch__ || __SYSC_ZARCH__
	#define PLATFORM "z/arch"
#elif __sparc__ || __sparc
	#define PLATFORM "sparc"
#elif __sh__
	#define PLATFORM "superh"
#elif __alpha__ || __alpha || _M_ALPHA
	#define PLATFORM "alpha"
#elif __mips__ || __mips || __MIPS__
	#define PLATFORM "mips"
#elif __m68k__ || M68000 || __MC68K__
	#define PLATFORM "m68k"
#elif __bfin || __BFIN__
	#define PLATFORM "blackfin"
#elif __epiphany__
	#define PLATFORM "epiphany"
#elif __hppa__ || __HPPA__ || __hppa
	#define PLATFORM "hp/pa"
#else
	#define PLATOFMR "unknown"
#endif