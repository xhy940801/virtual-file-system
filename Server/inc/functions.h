#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H

template <typename T>
inline void xswap(T& a, T& b)
{
	T c = a;
	a = b;
	b = c;
}

inline bool xstrequ(const char* str1, size_t len, const char* str2)
{
	while(len--)
	{
		if(*str1 == '\0')
			return false;
		if(*str1 != *str2)
			return false;
		++str1;
		++str2;
	}
	if(*str2 != '\0')
		return false;
	return true;
}

#endif