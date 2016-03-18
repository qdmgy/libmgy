#include "Var.hpp"
// #include <cmath>
// #include <cstdio>
// #include <cstdlib>
#include <cstring>
using namespace std;


decltype(Var::nil)	Var::nil;
decltype(Var::mrc)	Var::mrc;
decltype(Var::mrcm)	Var::mrcm;
#define lockGuard(name) std::lock_guard<decltype(Var::mrcm)>name(Var::mrcm)


size_t std::hash<Var>::operator()(Var const & var)const
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

void std::swap(Var & lhs, Var & rhs)
{
	if (&lhs == &rhs)
		return;
	char mem[sizeof(Var)];
	memcpy(&mem, &lhs, sizeof(Var));
	memcpy(&lhs, &rhs, sizeof(Var));
	memcpy(&rhs, &mem, sizeof(Var));
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
	, strong(true)
{
	lockGuard(lg);
	mrc.emplace(s, 1);
}

Var::Var(string && val)
	: s(new string{std::move(val)})
	, type(Type::string)
	, strong(true)
{
	lockGuard(lg);
	mrc.emplace(s, 1);
}

Var::Var(string const & val)
	: s(new string{val})
	, type(Type::string)
	, strong(true)
{
	lockGuard(lg);
	mrc.emplace(s, 1);
}

Var Var::function(function_t & val)
{
	Var rtn;
	if (val) {
		rtn.f = new function_t(val);
		rtn.type = Type::function;
		rtn.strong = true;
		lockGuard(lg);
		mrc.emplace(rtn.f, 1);
	}
	return std::move(rtn);
}

Var Var::table()
{
	Var rtn;
	rtn.t = new table_t;
	rtn.type = Type::table;
	rtn.strong = true;
	{
		lockGuard(lg);
		mrc.emplace(rtn.t, 1);
	}
	return std::move(rtn);
}

Var::Var(initializer_list<Var> il)
	: t(new table_t{il.size()})
	, type(Type::table)
	, strong(true)
{
	auto k = 1;
	for (auto & v : il)
		if (v != nil)
			t->emplace(k++, v);
	lockGuard(lg);
	mrc.emplace(t, 1);
}

Var::~Var()
{
	if (strong) {
		lockGuard(lg);
		if (--mrc[t] < 1)
			mrc.erase(t);
		else
			return;
	}
	else {
		return;
	}
	switch (type) {
		case Type::string:
			delete s;
			break;
		case Type::function:
			delete f;
			break;
		case Type::table:
			delete t;
			break;
		default:
			break;
	}
}

Var::Var(Var && rhs)
{
	memcpy(this, &rhs, sizeof(Var));
	rhs.type = Type::nil;
	rhs.strong = false;
}

Var::Var(Var const & rhs)
{
	memcpy(this, &rhs, sizeof(Var));
	if (strong) {
		lockGuard(lg);
		++mrc[t];
	}
}

Var & Var::operator=(Var && rhs)
{
	swap(*this, rhs);
	return *this;
}

Var & Var::operator=(Var const & rhs)
{
	if (this != &rhs) {
		Var thiz = std::move(*this);
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
		case Type::function:
		case Type::table:
			if (!strong && mrc.find(t) == mrc.end()) {
				type = Type::nil;
				return false;
			}
		default:
			return true;
	}
}

Var Var::operator-()const
{
	if (Type::number != type)
		throw TypeError(type, __FUNCTION__);
	return -n;
}

Var Var::operator()(Var args)const
{
	if (Type::function != type)
		throw TypeError(type, __FUNCTION__);
	if (strong)
		return (*f)(args);
	lockGuard(lg);
	if (*this)
		return (*f)(args);
	throw TypeError(type, __FUNCTION__);
}

Var::Ref Var::operator[](Var k)const
{
	if (Type::table != type || !*this)
		throw TypeError(type, __FUNCTION__);
	return{t, strong, std::move(k)};
}

auto Var::begin()const -> table_t::iterator
{
	if (Type::table != type)
		throw TypeError(type, __FUNCTION__);
	if (strong)
		return t->begin();
	lockGuard(lg);
	if (*this)
		return t->begin();
	throw TypeError(type, __FUNCTION__);
}

auto Var::end()const -> table_t::iterator
{
	if (Type::table != type)
		throw TypeError(type, __FUNCTION__);
	if (strong)
		return t->end();
	lockGuard(lg);
	if (*this)
		return t->end();
	throw TypeError(type, __FUNCTION__);
}

auto Var::cbegin()const -> table_t::const_iterator
{
	if (Type::table != type)
		throw TypeError(type, __FUNCTION__);
	if (strong)
		return t->cbegin();
	lockGuard(lg);
	if (*this)
		return t->cbegin();
	throw TypeError(type, __FUNCTION__);
}

auto Var::cend()const -> table_t::const_iterator
{
	if (Type::table != type)
		throw TypeError(type, __FUNCTION__);
	if (strong)
		return t->cend();
	lockGuard(lg);
	if (*this)
		return t->cend();
	throw TypeError(type, __FUNCTION__);
}

void Var::setWeak(bool w)const
{
	if (!weak && w) {
		switch (type) {
			case Var::Type::function:
				if (--fRC[f] <= 0) {
					fRC.erase(f);
					delete f;
				}
				weak = w;
				return;
			case Var::Type::table:
				if (--tRC[t] <= 0) {
					tRC.erase(t);
					delete t;
				}
				weak = w;
				return;
			default:
				return;
		}
	}
	if (weak && !w) {
		switch (type) {
			case Var::Type::function:{
				auto ite = fRC.find(f);
				if (ite != fRC.end()) {
					++ite->second;
					weak = w;
				}
				return;
			}
			case Var::Type::table:{
				auto ite = tRC.find(t);
				if (ite != tRC.end()) {
					++ite->second;
					weak = w;
				}
				return;
			}
			default:
				return;
		}
	}
}

void Var::setKeyWeak(Var key, bool w)const
{
	if (type != Type::table)
		throw TypeError(type, __FUNCTION__);
	auto ite = t->find(key);
	if (ite != t->end())
		ite->first.setWeak(w);
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
