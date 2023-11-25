#if NF_DEBUG
#pragma GCC optimize "-fomit-frame-pointer"

static long __attribute__((noinline)) __CDECL _nf_get_id(const char *feature_name)
{
	register long ret __asm__ ("d0");
	(void)(feature_name);
	__asm__ volatile(
		"\t.word 0x7300\n"
	: "=g"(ret)  /* outputs */
	: /* inputs  */
	: __CLOBBER_RETURN("d0") "d1", "cc" AND_MEMORY /* clobbered regs */
	);
	return ret;
}

static long __attribute__((noinline)) __CDECL _nf_call(long id, ...)
{
	register long ret __asm__ ("d0");
	(void)(id);
	__asm__ volatile(
		"\t.word 0x7301\n"
	: "=g"(ret)  /* outputs */
	: /* inputs  */
	: __CLOBBER_RETURN("d0") "d1", "cc" AND_MEMORY /* clobbered regs */
	);
	return ret;
}

struct {
	long __CDECL (*nf_get_id)(const char *feature_name);
	long __CDECL (*nf_call)(long id, ...);
} ops = {
	_nf_get_id,
	_nf_call
};


__attribute__((format(__printf__, 1, 2)))
static void nf_debugprintf(const char *format, ...)
{
	long nf_stderr = ops.nf_get_id("NF_STDERR");
	va_list args;
	char buf[1024];
	
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	ops.nf_call(nf_stderr | 0, buf);
}
#else
#define nf_debugprintf(format, ...)
#endif
