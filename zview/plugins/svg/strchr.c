char *strchr(const char *str, int charwanted)
{
	const char *p = str;
	char c;

	do
	{
		if ((c = *p++) == (char) charwanted)
			return (char *)(--p);
	} while (c);

	return NULL;
}
