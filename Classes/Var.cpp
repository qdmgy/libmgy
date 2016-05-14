#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif // _MSC_VER

#include "Var.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace std;


decltype(Var::nil)	Var::nil;
decltype(Var::mrc)	Var::mrc;
decltype(Var::mrcm)	Var::mrcm;
#define lockGuard(name) std::lock_guard<decltype(Var::mrcm)>name(Var::mrcm)


size_t std::hash<Var>::operator()(Var const & var)const noexcept
{
	switch (var.type) {
		case Var::Type::nil:
			return hash<void*>{}(nullptr);
		case Var::Type::boolean:
			return hash<Var::bool_t>{}(var.b);
		case Var::Type::number:
			return hash<Var::number_t>{}(var.n);
		default:
			return hash<void*>{}(var.t);
	}
}


string Var::TypeName(Type type) noexcept
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

Var::~Var() noexcept
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

Var::Var(Var const & rhs)
{
	switch (rhs.type) {
		case Type::string:
		case Type::function:
		case Type::table:{
			lockGuard(lg);
			if (rhs) {
				++mrc[t = rhs.t];
				type = rhs.type;
				strong = true;
			}
			break;
		}
		default:
			memcpy(this, &rhs, sizeof(Var));
			break;
	}
}

Var & Var::operator=(Var const & rhs)
{
	if (this != &rhs) {
		if (rhs.strong) {
			lockGuard(lg);
			++mrc[rhs.t];
		}
		Var self;
		memcpy(&self, this, sizeof(Var));
		memcpy(this, &rhs, sizeof(Var));
	}
	return *this;
}

Var::Var(Var && rhs) noexcept
{
	memcpy(this, &rhs, sizeof(Var));
	new(&rhs)Var;
}

void Var::swap(Var & rhs) noexcept
{
	if (this == &rhs) return;

	char mem[sizeof(Var)];
	memcpy(&mem, this, sizeof(Var));
	memcpy(this, &rhs, sizeof(Var));
	memcpy(&rhs, &mem, sizeof(Var));
}

Var::operator bool()const noexcept
{
	switch (type) {
		case Type::nil:
			return false;
		case Type::boolean:
			return b;
		case Type::function:
		case Type::table:
			if (!strong && !mrc.count(t)) {
				type = Type::nil;
				return false;
			}
		default:
			return true;
	}
}

Var Var::operator-()const
{
	if (Type::number == type)
		return -n;
	throw TypeError(type, __FUNCTION__);
}

Var Var::operator()(Var const & args)const
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

Var Var::operator()(Var && args)const
{
	if (Type::function != type)
		throw TypeError(type, __FUNCTION__);
	if (strong)
		return (*f)(std::move(args));
	lockGuard(lg);
	if (*this)
		return (*f)(std::move(args));
	throw TypeError(type, __FUNCTION__);
}

Var::Ref Var::operator[](Var k)const
{
	if (Type::table == type && *this)
		return Ref(std::move(k), this);
	throw TypeError(type, __FUNCTION__);
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

bool Var::setWeak(bool weak)const
{
	if (type < Type::function || strong != weak)
		return !strong;

	lockGuard(lg);
	if (weak) {
		if (--mrc[t] < 1) {
			type = Type::nil;
			mrc.erase(t);
			Type::function == type ? delete f : delete t;
		}
		return strong = false;
	}
	else if (*this) {
		++mrc[t];
		return strong = true;
	}
	return true;
}

bool Var::setKeyWeak(Var k, bool weak)const
{
	if (Type::table != type)
		throw TypeError(type, __FUNCTION__);

	auto staff = [t=this->t, &k, weak] {
		auto it = t->find(k);
		return t->end() == it || it->first.setWeak(weak);
	};
	if (strong)
		return staff();
	lockGuard(lg);
	if (*this)
		return staff();
	throw TypeError(type, __FUNCTION__);
}


Var * Var::Ref::get()
{
	auto staff = [t=this->_tbl->t, &k=this->_key]() -> Var* {
		auto it = t->find(k);
		return it != t->end() ? &it->second : 0;
	};
	if (_tbl->strong)
		return staff();
	lockGuard(lg);
	if (*_tbl)
		return staff();
	throw TypeError(_tbl->type, __FUNCTION__);
}

Var & Var::Ref::operator=(Ref && rhs)
{
	auto staff = [this, &rhs]() -> Var& {
		auto p = rhs.get();
		return *this = p ? *p : nil;
	};
	if (rhs._tbl->strong)
		return staff();
	lockGuard(lg);
	if (*rhs._tbl)
		return staff();
	throw TypeError(rhs._tbl->type, __FUNCTION__);
}

Var & Var::Ref::operator=(Var const & v)
{
	if (nil == _key)
		return _key;

	auto & t = *_tbl->t;
	if (_tbl->strong)
		return t[std::move(_key)] = v;
	lockGuard(lg);
	if (*_tbl)
		return t[std::move(_key)] = v;
	throw TypeError(_tbl->type, __FUNCTION__);
}

Var & Var::Ref::operator=(Var && v)
{
	if (nil == _key)
		return _key;

	auto & t = *_tbl->t;
	if (_tbl->strong)
		return t[std::move(_key)] = std::move(v);
	lockGuard(lg);
	if (*_tbl)
		return t[std::move(_key)] = std::move(v);
	throw TypeError(_tbl->type, __FUNCTION__);
}

Var::Ref::operator Var &()
{
	if (nil == _key)
		return _key;

	auto & t = *_tbl->t;
	if (_tbl->strong)
		return t[std::move(_key)];
	lockGuard(lg);
	if (*_tbl)
		return t[std::move(_key)];
	throw TypeError(_tbl->type, __FUNCTION__);
}

void Var::Ref::swap(Ref && rhs)
{
	auto staff = [this, &rhs] {
		if (auto p = this->get())
			p->swap(rhs);
		else if (auto p = rhs.get())
			p->swap(*this);
	};
	if (_tbl->strong && rhs._tbl->strong)
		return staff();
	lockGuard(lg);
	return staff();
}

void Var::Ref::swap(Var & rhs)
{
	if (_tbl->strong)
		return rhs.swap(*this);
	lockGuard(lg);
	if (*_tbl)
		return rhs.swap(*this);
	throw TypeError(_tbl->type, __FUNCTION__);
}

Var::Ref::operator bool()
{
	auto staff = [this] {
		auto p = this->get();
		return p && *p;
	};
	if (_tbl->strong)
		return staff();
	lockGuard(lg);
	return staff();
}

Var Var::Ref::operator-()
{
	auto staff = [this] {
		auto p = this->get();
		return p ? -*p : -nil;
	};
	if (_tbl->strong)
		return staff();
	lockGuard(lg);
	return staff();
}

Var Var::Ref::operator()(Var const & args)
{
	auto staff = [this, &args] {
		auto p = this->get();
		return p ? (*p)(args) : nil(args);
	};
	if (_tbl->strong)
		return staff();
	lockGuard(lg);
	return staff();
}

Var Var::Ref::operator()(Var && args)
{
	auto staff = [this, &args] {
		auto p = this->get();
		return p ? (*p)(std::move(args)) : nil(std::move(args));
	};
	if (_tbl->strong)
		return staff();
	lockGuard(lg);
	return staff();
}

Var::Ref Var::Ref::operator[](Var const & k)
{
	if (_tbl->strong) {
		auto p = get();
		return Ref(p ? (*p)[k] : nil[k]);
	}
	lockGuard(lg);
	auto p = get();
	return Ref(p ? (*p)[k] : nil[k]);
}

Var::Ref Var::Ref::operator[](Var && k)
{
	if (_tbl->strong) {
		auto p = get();
		return Ref(p ? (*p)[std::move(k)] : nil[std::move(k)]);
	}
	lockGuard(lg);
	auto p = get();
	return Ref(p ? (*p)[std::move(k)] : nil[std::move(k)]);
}

auto Var::Ref::begin() -> table_t::iterator
{
	auto staff = [this] {
		auto p = this->get();
		return p ? p->begin() : nil.begin();
	};
	if (_tbl->strong)
		return staff();
	lockGuard(lg);
	return staff();
}

auto Var::Ref::end() -> table_t::iterator
{
	auto staff = [this] {
		auto p = this->get();
		return p ? p->end() : nil.end();
	};
	if (_tbl->strong)
		return staff();
	lockGuard(lg);
	return staff();
}

auto Var::Ref::cbegin() -> table_t::const_iterator
{
	auto staff = [this] {
		auto p = this->get();
		return p ? p->cbegin() : nil.cbegin();
	};
	if (_tbl->strong)
		return staff();
	lockGuard(lg);
	return staff();
}

auto Var::Ref::cend() -> table_t::const_iterator
{
	auto staff = [this] {
		auto p = this->get();
		return p ? p->cend() : nil.cend();
	};
	if (_tbl->strong)
		return staff();
	lockGuard(lg);
	return staff();
}

bool Var::Ref::setWeak(bool w)
{
	auto staff = [this, w] {
		auto p = this->get();
		return p ? p->setWeak(w) : nil.setWeak(w);
	};
	if (_tbl->strong)
		return staff();
	lockGuard(lg);
	return staff();
}

bool Var::Ref::setKeyWeak(Var const & k, bool w)
{
	auto staff = [this, &k, w] {
		auto p = this->get();
		return p ? p->setKeyWeak(k, w) : nil.setKeyWeak(k, w);
	};
	if (_tbl->strong)
		return staff();
	lockGuard(lg);
	return staff();
}

bool Var::Ref::setKeyWeak(Var && k, bool w)
{
	auto staff = [this, &k, w] {
		auto p = this->get();
		return p ? p->setKeyWeak(std::move(k), w) : nil.setKeyWeak(std::move(k), w);
	};
	if (_tbl->strong)
		return staff();
	lockGuard(lg);
	return staff();
}


bool operator==(Var const & lhs, Var const & rhs)
{
	!lhs, !rhs;
	if (lhs.type != rhs.type)
		return false;

	switch (rhs.type) {
		case Var::Type::nil:
			return true;
		case Var::Type::boolean:
			return lhs.b == rhs.b;
		case Var::Type::number:
			return lhs.n == rhs.n;
		case Var::Type::string:
			return lhs.s == rhs.s || *lhs.s == *rhs.s;
		default:
			return lhs.t == rhs.t;
	}
}

ostream & operator<<(ostream & os, Var const & rhs)
{
	Var s = toString(rhs);
	switch (rhs.type) {
		default:
			return os << toCString(s);
		case Var::Type::string:
			return os << '\"' << toCString(s) << '\"';
		case Var::Type::table:
			return os << '{' << toCString(s) << '}';
	}
}

bool operator<(Var const & lhs, Var const & rhs)
{
	if (lhs.type != rhs.type)
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);

	switch (rhs.type) {
		case Var::Type::number:
			return lhs.n < rhs.n;
		case Var::Type::string:
			return lhs.s != rhs.s && *lhs.s < *rhs.s;
		default:
			throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
}

bool operator>(Var const & lhs, Var const & rhs)
{
	if (lhs.type != rhs.type)
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);

	switch (rhs.type) {
		case Var::Type::number:
			return lhs.n > rhs.n;
		case Var::Type::string:
			return lhs.s != rhs.s && *lhs.s > *rhs.s;
		default:
			throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
}

Var operator+(Var const & lhs, Var const & rhs)
{
	if (lhs.type != rhs.type)
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);

	switch (rhs.type) {
		case Var::Type::number:
			return lhs.n + rhs.n;
		case Var::Type::string:
			return *lhs.s + *rhs.s;
		default:
			throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	}
}

Var operator-(Var const & lhs, Var const & rhs)
{
	if (Var::Type::number != lhs.type || Var::Type::number != rhs.type)
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	return lhs.n - rhs.n;
}

Var operator*(Var const & lhs, Var const & rhs)
{
	if (Var::Type::number != lhs.type || Var::Type::number != rhs.type)
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	return lhs.n * rhs.n;
}

Var operator/(Var const & lhs, Var const & rhs)
{
	if (Var::Type::number != lhs.type || Var::Type::number != rhs.type)
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	return lhs.n / rhs.n;
}

Var operator%(Var const & lhs, Var const & rhs)
{
	if (Var::Type::number != lhs.type || Var::Type::number != rhs.type)
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	return fmod(lhs.n, rhs.n);
}

Var operator^(Var const & lhs, Var const & rhs)
{
	if (Var::Type::number != lhs.type || Var::Type::number != rhs.type)
		throw Var::TypeError(lhs.type, rhs.type, __FUNCTION__);
	return pow(lhs.n, rhs.n);
}

Var toNumber(Var const & var)
{
	!var;
	switch (var.type) {
		default:
			return nullptr;
		case Var::Type::number:
			return var;
		case Var::Type::string:
			return atof(var.s->c_str());
	}
}

Var toString(Var const & var)
{
	!var;
	char s[24] = "";
	switch (var.type) {
		case Var::Type::nil:
			return type(var);
		case Var::Type::boolean:
			return var.b ? "true" : "false";
		case Var::Type::number:
			sprintf(s, "%g", var.n);
			return s;
		case Var::Type::string:
			return var;
		default:
			sprintf(s, "0x%p", var.t);
			return s;
	}
}

char const * toCString(Var const & var)
{
	if (Var::Type::string != var.type)
		throw Var::TypeError(var.type, __FUNCTION__);
	return var.s->c_str();
}

ostream & printTable(Var const & var, ostream & os)
{
	if (Var::Type::table != var.type)
		throw Var::TypeError(var.type, __FUNCTION__);
	os << "{\n";
	for (auto & pair : var)
		os << "\t[" << pair.first << "] = " << pair.second << '\n';
	os << "}\n";
	return os;
}


Var::TypeError::TypeError(Type type, string const & func)
	: runtime_error("Call "+func+" with a "+TypeName(type))
{
}

Var::TypeError::TypeError(Type lhs, Type rhs, string const & func)
	: runtime_error("Call "+func+" with "+TypeName(lhs)+" & "+TypeName(rhs))
{
}
