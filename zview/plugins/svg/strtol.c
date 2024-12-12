/* Nonzero if we are defining `strtoul' or `strtoull', operating on
   unsigned integers.  */
#ifndef UNSIGNED
#define UNSIGNED 0
#define INT LONG int
#else
#define INT unsigned LONG int
#endif

/* Determine the name.  */
# if UNSIGNED
#   ifdef QUAD
#    define strtol strtoull
#   else
#    define strtol strtoul
#   endif
# else
#  ifdef QUAD
#    define strtol strtoll
#  endif
# endif

/* If QUAD is defined, we are defining `strtoll' or `strtoull',
   operating on `long long int's.  */
#ifdef QUAD
#define LONG long long
#define STRTOL_LONG_MIN LONG_LONG_MIN
#define STRTOL_LONG_MAX LONG_LONG_MAX
#define STRTOL_ULONG_MAX ULONG_LONG_MAX
#else
#define LONG long
#define STRTOL_LONG_MIN LONG_MIN
#define STRTOL_LONG_MAX LONG_MAX
#define STRTOL_ULONG_MAX ULONG_MAX
#endif

#define L_(Ch) Ch
#define UCHAR_TYPE unsigned char
#define STRING_TYPE char
#define ISSPACE(c) ((c) == ' ' || (c) == '\t')
#define ISALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define TOUPPER(c) ((c) & ~0x20)

#ifndef __set_errno
#  define __set_errno(e) errno = (e)
#endif

INT strtol(const STRING_TYPE *nptr, STRING_TYPE **endptr, int base);

/* Convert NPTR to an `unsigned long int' or `long int' in base BASE.
   If BASE is 0 the base is determined by the presence of a leading
   zero, indicating octal or a leading "0x" or "0X", indicating hexadecimal.
   If BASE is < 2 or > 36, it is reset to 10.
   If ENDPTR is not NULL, a pointer to the character after the last
   one converted is stored in *ENDPTR.  */

INT strtol(const STRING_TYPE *nptr, STRING_TYPE **endptr, int base)
{
	int negative;
	unsigned LONG int cutoff;
	unsigned int cutlim;
	unsigned LONG int i;
	const STRING_TYPE *s;
	UCHAR_TYPE c;
	const STRING_TYPE *save;
	const STRING_TYPE *end;
	int overflow;

	if (base < 0 || base == 1 || base > 36)
	{
		__set_errno(EINVAL);
		return 0;
	}

	save = s = nptr;

	/* Skip white space.  */
	while (ISSPACE(*s))
		++s;
	if (*s == L_('\0'))
		goto noconv;

	/* Check for a sign.  */
	if (*s == L_('-'))
	{
		negative = 1;
		++s;
	} else if (*s == L_('+'))
	{
		negative = 0;
		++s;
	} else
		negative = 0;

	/* Recognize number prefix and if BASE is zero, figure it out ourselves.  */
	if (*s == L_('0'))
	{
		if ((base == 0 || base == 16) && TOUPPER(s[1]) == L_('X'))
		{
			s += 2;
			base = 16;
		} else if (base == 0)
			base = 8;
	} else if (base == 0)
		base = 10;

	/* Save the pointer so we can check later if anything happened.  */
	save = s;

	end = NULL;

	cutoff = STRTOL_ULONG_MAX / (unsigned LONG int) base;
	cutlim = STRTOL_ULONG_MAX % (unsigned LONG int) base;

	overflow = 0;
	i = 0;
	c = *s;
#ifdef QUAD
	if (sizeof(long int) != sizeof(LONG int))
	{
		unsigned long int j = 0;
		unsigned long int jmax = ULONG_MAX / base;

		for (; c != L_('\0'); c = *++s)
		{
			if (s == end)
				break;
			if (c >= L_('0') && c <= L_('9'))
				c -= L_('0');
			else if (ISALPHA(c))
				c = TOUPPER(c) - L_('A') + 10;
			else
				break;
			if ((int) c >= base)
				break;
			/* Note that we never can have an overflow.  */
			else if (j >= jmax)
			{
				/* We have an overflow.  Now use the long representation.  */
				i = (unsigned LONG int) j;
				goto use_long;
			} else
				j = j * (unsigned long int) base + c;
		}

		i = (unsigned LONG int) j;
	} else
#endif
	{
		for (; c != L_('\0'); c = *++s)
		{
			if (s == end)
				break;
			if (c >= L_('0') && c <= L_('9'))
				c -= L_('0');
			else if (ISALPHA(c))
				c = TOUPPER(c) - L_('A') + 10;
			else
				break;
			if ((int) c >= base)
				break;
			/* Check for overflow.  */
			if (i > cutoff || (i == cutoff && c > cutlim))
				overflow = 1;
			else
			{
#ifdef QUAD
			  use_long:
#endif
				i *= (unsigned LONG int) base;
				i += c;
			}
		}
	}

	/* Check if anything actually happened.  */
	if (s == save)
		goto noconv;

	/* Store in ENDPTR the address of one character
	   past the last character we converted.  */
	if (endptr != NULL)
		*endptr = (STRING_TYPE *) s;

#if !UNSIGNED
	/* Check for a value that is within the range of
	   `unsigned LONG int', but outside the range of `LONG int'.  */
	if (overflow == 0
		&& i > (negative ? -((unsigned LONG int) (STRTOL_LONG_MIN + 1)) + 1 : (unsigned LONG int) STRTOL_LONG_MAX))
		overflow = 1;
#endif

	if (overflow)
	{
		__set_errno(ERANGE);
#if UNSIGNED
		return STRTOL_ULONG_MAX;
#else
		return negative ? STRTOL_LONG_MIN : STRTOL_LONG_MAX;
#endif
	}

	/* Return the result of the appropriate sign.  */
	return negative ? -i : i;

  noconv:
	/* We must handle a special case here: the base is 0 or 16 and the
	   first two characters are '0' and 'x', but the rest are no
	   hexadecimal digits.  This is no error case.  We return 0 and
	   ENDPTR points to the `x`.  */
	if (endptr != NULL)
	{
		if (save - nptr >= 2 && TOUPPER(save[-1]) == L_('X') && save[-2] == L_('0'))
			*endptr = (STRING_TYPE *) & save[-1];
		else
			/*  There was no number to convert.  */
			*endptr = (STRING_TYPE *) nptr;
	}

	return 0;
}

#undef INT
#undef LONG
#undef UNSIGNED
#undef QUAD
#undef strtol
#undef STRTOL_LONG_MIN
#undef STRTOL_LONG_MAX
#undef STRTOL_ULONG_MAX
#undef L_
#undef UCHAR_TYPE
#undef STRING_TYPE
#undef ISSPACE
#undef ISALPHA
#undef TOUPPER
