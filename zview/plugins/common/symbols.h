/*
 * symbols.h - internal header file.
 * Add (or not) underscores to external symbol names
 *
 * Copyright (C) 2018 Thorsten Otto
 */

#ifndef __USER_LABEL_PREFIX__
#  if defined(__ELF__)
#    define __USER_LABEL_PREFIX__
#  else
#    define __USER_LABEL_PREFIX__ _
#  endif
#endif

#ifdef __ASSEMBLER__
#  ifndef __REGISTER_PREFIX__
#    define __REGISTER_PREFIX__ %
#  endif
#  define CONCAT1(a, b) CONCAT2(a, b)
#  define CONCAT2(a, b) a ## b
#  define REG(r) CONCAT1(__REGISTER_PREFIX__, r)
#  define d0 REG(d0)
#  define d1 REG(d1)
#  define d2 REG(d2)
#  define d3 REG(d3)
#  define d4 REG(d4)
#  define d5 REG(d5)
#  define d6 REG(d6)
#  define d7 REG(d7)
#  define a0 REG(a0)
#  define a1 REG(a1)
#  define a2 REG(a2)
#  define a3 REG(a3)
#  define a4 REG(a4)
#  define a5 REG(a5)
#  define a6 REG(a6)
#  define a7 REG(a7)
#  define fp REG(fp)
#  define sp REG(sp)
#  define pc REG(pc)
#  define sr REG(sr)
#endif

#undef __STRING
#undef __STRINGIFY
#define __STRING(x)	#x
#define __STRINGIFY(x)	__STRING(x)

#ifndef __SYMBOL_PREFIX
# define __SYMBOL_PREFIX __STRINGIFY(__USER_LABEL_PREFIX__)
# define __ASM_SYMBOL_PREFIX __USER_LABEL_PREFIX__
#endif

#ifndef C_SYMBOL_NAME
# ifdef __ASSEMBLER__
#   define C_SYMBOL_NAME2(pref, name) pref##name
#   define C_SYMBOL_NAME1(pref, name) C_SYMBOL_NAME2(pref, name)
    /* avoid empty macro arguments */
#   define _ 2
#   if (__ASM_SYMBOL_PREFIX + 0) > 1
#     define C_SYMBOL_NAME(name) C_SYMBOL_NAME1(__ASM_SYMBOL_PREFIX, name)
#   else
#     define C_SYMBOL_NAME(name) name
#   endif
#   undef _
# else
#   define C_SYMBOL_NAME(name) __SYMBOL_PREFIX #name
# endif
#endif

#ifndef __ASSEMBLER__
# ifdef __GNUC__
#   define ASM_NAME(x) __asm__(x)
# else
#   define ASM_NAME(x)
# endif
#endif
