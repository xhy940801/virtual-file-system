#ifndef _GUARD_H
#define _GUARD_H

template<typename T, typename OP>
class Guard
{
	T d;
	OP op;
public:
	Guard(T _d, OP _op) : d(_d), op(_op) {}
	~Guard() { op(d); }
	
};

#endif