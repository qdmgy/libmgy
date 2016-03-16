#include "Var.hpp"

// #include <cmath>
#include <cstdio>
// #include <cstdlib>
#include <cstring>
using namespace std;

#define LOCK_GUARD(name,lock) std::lock_guard<decltype(lock)>name(lock)


decltype(Var::mrc)	Var::mrc;
decltype(Var::mrcm)	Var::mrcm;
decltype(Var::nil)	Var::nil;


auto hash<Var>::operator()(argument_type const & var)const -> result_type
{
	switch (var.type) {
		case Var::Type::nil:
			return hash < void* > {}(nullptr);
		case Var::Type::boolean:
			return hash < Var::bool_t > {}(var.b);
		case Var::Type::number:
			return hash < Var::number_t > {}(var.n);
		default:
			return hash < void* > {}(var.t);
	}
}


string Var::typeName(Type type)
{
	switch (type) {
		default:
			return "nil";
		case Type::boolean:
			return "boolean";
		case Type::number:
			return "number";
		case Type::string:
			return "string";
		case Type::function:
			return "function";
		case Type::table:
			return "table";
	}
}

Var::Var(nil_t)
	: Var()
{
}

Var::Var(bool_t val)
	: b(val)
	, type(Type::boolean)
{
}

Var::Var(int val)
	: Var{(number_t)val}
{
}

Var::Var(unsigned val)
	: Var{(number_t)val}
{
}

Var::Var(number_t val)
	: n(val)
	, type(Type::number)
{
}

Var::Var(char const * val)
	: s(new string{val})
	, type(Type::string)
{
	LOCK_GUARD(lg, mrcm);
	mrc.emplace(s, 1);
}

Var::Var(string && val)
	: s(new string{std::move(val)})
	, type(Type::string)
{
	LOCK_GUARD(lg, mrcm);
	mrc.emplace(s, 1);
}

Var::Var(string const & val)
	: s(new string{val})
	, type(Type::string)
{
	LOCK_GUARD(lg, mrcm);
	mrc.emplace(s, 1);
}

Var Var::function(function_t & val)
{
	Var rtn;
	if (val) {
		rtn.f = new function_t(val);
		rtn.type = Type::function;
		LOCK_GUARD(lg, mrcm);
		mrc.emplace(rtn.f, 1);
	}
	return rtn;
}

Var Var::table()
{
	Var rtn;
	rtn.t = new table_t;
	rtn.type = Type::table;
	LOCK_GUARD(lg, mrcm);
	mrc.emplace(rtn.t, 1);
	return rtn;
}

Var::Var(initializer_list<Var> il)
	: t(new table_t{il.size()})
	, type(Type::table)
{
	auto k = 1;
	for (auto & v : il) {
		if (v != nil) {
			t->emplace(k++, v);
		}
	}
	LOCK_GUARD(lg, mrcm);
	mrc.emplace(t, 1);
}

Var::~Var()
{
	switch (type) {
		case Type::string:{
			LOCK_GUARD(lg, mrcm);
			if (--mrc[s] < 1) {
				mrc.erase(s);
				delete s;
			}
			break;
		}
		case Type::function:
			if (!weak) {
				LOCK_GUARD(lg, mrcm);
				if (--mrc[f] < 1) {
					mrc.erase(f);
					delete f;
				}
			}
			break;
		case Type::table:
			if (!weak) {
				LOCK_GUARD(lg, mrcm);
				if (--mrc[t] < 1) {
					mrc.erase(t);
					delete t;
				}
			}
			break;
		default:
			break;
	}
}

Var::Var(Var && rhs)
{
	memcpy(this, &rhs, sizeof(Var));
	rhs.type = Type::nil;
}

Var & Var::operator=(Var && rhs)
{
	if (this != &rhs) {
		Var tmp;
		memcpy(&tmp, this, sizeof(Var));
		memcpy(this, &rhs, sizeof(Var));
		memcpy(&rhs, &tmp, sizeof(Var));
	}
	return *this;
}

Var::Var(Var const & rhs)
	: type(rhs.type)
	, weak(false)
{
	memcpy(&n, &rhs.n, sizeof(n));
	switch (type) {
		case Type::string:
		case Type::function:
		case Type::table:{
			LOCK_GUARD(lg, mrcm);
			auto it = mrc.find(t);
			if (it != mrc.end()) {
				++it->second;
			}
			else {
				type = Type::nil;
			}
			break;
		}
		default:
			break;
	}
}

Var & Var::operator=(Var const & rhs)
{
	if (this != &rhs) {
		this->~Var();
		new(this) Var(rhs);
	}
	return *this;
}

Var::operator bool()const
{
	switch (type) {
		case Type::nil:
			return false;
		case Type::boolean:
			return b;
		default:
			return true;
	}
}

Var Var::operator-()const
{
	if (Type::number != type) {
		throw TypeError(type, __FUNCTION__);
	}
	return -n;
}

Var Var::operator()(Var args)const
{
	if (Type::function != type) {
		throw TypeError(type, __FUNCTION__);
	}
	if (fRC.find(f) == fRC.end()) {
		throw TypeError(Type::nil, __FUNCTION__);
	}
	return (*f)(args);
}

Var::Ref Var::operator[](Var & k)const
{
	if (Type::table != type) {
		throw TypeError(type, __FUNCTION__);
	}
	if (tRC.find(t) == tRC.end()) {
		throw TypeError(Type::nil, __FUNCTION__);
	}
	Var & v = (*t)[std::move(k)];
	switch (v.type) {
		case Type::function:
			if (fRC.find(v.f) == fRC.end()) {
				v.type = Type::nil;
			}
			break;
		case Type::table:
			if (tRC.find(v.t) == tRC.end()) {
				v.type = Type::nil;
			}
			break;
		default:
			break;
	}
	return (Ref)v;
}

Var::Ref Var::operator[](Var && k)const
{
	if (Type::table != type) {
		throw TypeError(type, __FUNCTION__);
	}
	if (tRC.find(t) == tRC.end()) {
		throw TypeError(Type::nil, __FUNCTION__);
	}
	Var & v = (*t)[k];
	switch (v.type) {
		case Type::function:
			if (fRC.find(v.f) == fRC.end()) {
				v.type = Type::nil;
			}
			break;
		case Type::table:
			if (tRC.find(v.t) == tRC.end()) {
				v.type = Type::nil;
			}
			break;
		default:
			break;
	}
	return (Ref)v;
}

Var::Ref Var::operator[](Var const & k)const
{
	return k.weak ? operator[]((Var&)k) : operator[]((Var&&)k);
}

Var::Ite Var::begin()const
{
	if (Type::table != type) {
		throw TypeError(type, __FUNCTION__);
	}
	if (tRC.find(t) == tRC.end()) {
		throw TypeError(Type::nil, __FUNCTION__);
	}
	return (Ite)t->begin();
}

Var::Ite Var::end()const
{
	if (Type::table != type) {
		throw TypeError(type, __FUNCTION__);
	}
	if (tRC.find(t) == tRC.end()) {
		throw TypeError(Type::nil, __FUNCTION__);
	}
	return (Ite)t->end();
}


Var::Ref::Ref(Var & var)
	: _cp(&var)
{
}

Var::Ref Var::Ref::operator=(Var & v)const
{
	if (_cp != &v) {
		_cp->~Var();
		new(_cp)Var(std::move(v));
	}
	return *this;
}

Var::Ref Var::Ref::operator=(Var && v)const
{
	return Ref(*_cp = v);
}

Var::Ref Var::Ref::operator=(Var const & v)const
{
	return v.weak ? operator=((Var&)v) : Ref(*_cp = v);
}


Var::Ite::Ite(base && rr)
	: base(std::move(rr))
{
}


bool operator==(Var lhs, Var rhs)
{
	if (lhs.type != rhs.type) {
		return false;
	}
	switch (rhs.type) {
		default:
			return true;
		case Var::Type::boolean:
			return lhs.b == rhs.b;
		case Var::Type::number:
			return lhs.n == rhs.n;
		case Var::Type::string:
			return lhs.s == rhs.s || *lhs.s == *rhs.s;
		case Var::Type::function:
			return lhs.f == rhs.f;
		case Var::Type::table:
			return lhs.t == rhs.t;
	}
}

ostream & operator<<(ostream & os, Var rhs)
{
	Var s = toString(rhs);
	switch (rhs.type) {
		case Var::Type::string:
			return os << '\"' << toCString(s) << '\"';
		case Var::Type::table:
			return os << '{' << toCString(s) << '}';
		default:
			return os << toCString(s);
	}
}

bool operator<(Var lhs, Var rhs)
{
	if (lhs.type != rhs.type) {
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
	switch (rhs.type) {
		case Var::Type::number:
			return lhs.n < rhs.n;
		case Var::Type::string:
			return lhs.s != rhs.s && *lhs.s < *rhs.s;
		default:
			throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
}

bool operator>(Var lhs, Var rhs)
{
	if (lhs.type != rhs.type) {
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
	switch (rhs.type) {
		case Var::Type::number:
			return lhs.n > rhs.n;
		case Var::Type::string:
			return lhs.s != rhs.s && *lhs.s > *rhs.s;
		default:
			throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
}

Var operator+(Var lhs, Var rhs)
{
	if (lhs.type != rhs.type) {
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
	switch (rhs.type) {
		case Var::Type::number:
			return lhs.n + rhs.n;
		case Var::Type::string:
			return *lhs.s + *rhs.s;
		default:
			throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
}

Var operator-(Var lhs, Var rhs)
{
	if (Var::Type::number != lhs.type || Var::Type::number != rhs.type) {
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
	return lhs.n - rhs.n;
}

Var operator*(Var lhs, Var rhs)
{
	if (Var::Type::number != lhs.type || Var::Type::number != rhs.type) {
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
	return lhs.n * rhs.n;
}

Var operator/(Var lhs, Var rhs)
{
	if (Var::Type::number != lhs.type || Var::Type::number != rhs.type) {
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
	return lhs.n / rhs.n;
}

Var operator%(Var lhs, Var rhs)
{
	if (Var::Type::number != lhs.type || Var::Type::number != rhs.type) {
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
	return fmod(lhs.n, rhs.n);
}

Var operator^(Var lhs, Var rhs)
{
	if (Var::Type::number != lhs.type || Var::Type::number != rhs.type) {
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
	return pow(lhs.n, rhs.n);
}

Var toNumber(Var var)
{
	switch (var.type) {
		case Var::Type::number:
			return var;
		case Var::Type::string:
			return atof(var.s->c_str());
		default:
			return nullptr;
	}
}

Var toString(Var var)
{
	char s[20] = "";
	switch (var.type) {
		case Var::Type::nil:
			return type(var);
		case Var::Type::boolean:
			return var.b ? "true" : "false";
		case Var::Type::number:
			return to_string(var.n);//sprintf(s, "%g", var.n);
		case Var::Type::string:
			return var;
		default:
			sprintf(s, "0x%p", var.f);
			return s;
	}
}

char const * toCString(Var var)
{
	if (Var::Type::string != var.type) {
		throw Var::TypeError(var.type, __FUNCTION__);
	}
	return var.s->c_str();
}


Var::TypeError::TypeError(Type type, string const & func)
	: runtime_error("Call "+func+" with a "+typeName(type))
{
}

Var::TypeError::TypeError(Type lhs, Type rhs, string const & func)
	: runtime_error("Call "+func+" with "+typeName(lhs)+" and "+typeName(rhs))
{
}
