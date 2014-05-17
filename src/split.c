#include "kernel.h"

static char space[] = " \t\r\n\f";
static const char *curfs = space;

static bool
isfs(char c)
{
	return strchr(curfs, c) != NULL;
}

static bool
is_space(char c)
{
	return strchr(space, c) != NULL;
}

// setfs - Establece el separador de campos para split.

const char *
setfs(const char *fs)
{
	const char *p;

	if (fs == NULL)
		fs = space;
	p = curfs;
	curfs = fs;
	return p;
}

// split - Divide la entrada en campos separados por secuencias de caracteres
// delimitadores. Por defecto los delimitadores son espacios en blanco.

unsigned
split(char *s, char *field[], unsigned nfields)
{
	char *p;
	unsigned i, state;

	for (p = s + strlen(s) - 1; p >= s && isfs(*p);)
		*p-- = 0;

	for (p = s, i = 0, state = 0; *p && i < nfields; ++p)
	{
		switch (state)
		{
			case 0:
				if (isfs(*p))
					break;
				field[i++] = p;
				state = 1;
				break;

			case 1:
				if (!isfs(*p))
					break;
				*p = 0;
				state = 0;
				break;
		}
	}
	return i;
}

static char *
getstr(char *s, unsigned *len)
{
	char *p;

	*len = 0;
	if (s == NULL)
		return "";
	while (*s && is_space(*s))
		++s;
	if (*(p = s) == '\"')
	{
		while (*++p && (*p != '\"' || *(p - 1) == '\\'))
			;
		if (*p)
			++p;
	}
	else
		while (*p && !is_space(*p))
			++p;
	*len = p - s;
	return s;
}

// separate -- Separa la entrada en campos delimitados por espacios o encerrados entre
// comillas. Permite escapar las comillas mediante '\' para poder incluirlas en los 
// campos de entrada.

unsigned 
separate(char *s, char *field[], unsigned nfields)
{
	unsigned i, n, len;
	char *end;

	for (end = NULL, n = i = 0; i < nfields; ++i, end = s + (*s == '\"' ? len - 1 : len), s += len)
	{
		if (*(s = getstr(s, &len)))
			++n;
		field[i] = (*s == '"') ? s + 1 : s;
		if (i)
			*end = 0;
	}
	if (n)
		*end = 0;
	for (i = 0; i < n; i++)
	{
		s = field[i];
		while ((s = strchr(s, '\\')) != NULL)
			if (s[1] == '"')
				strcpy(s, s + 1);
			else
				s += 2;
	}
	return n;
}
