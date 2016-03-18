#ifndef VAR_HPP
#define VAR_HPP


#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

struct Var;

namespace std
{
	template<>
	struct hash<Var>
	{
		using argument_type = Var;
		using result_type = size_t;
		size_t operator()(Var const &)const;
	};
	void swap(Var &, Var &);
}


struct Var
{
	//类型
	enum class Type : char {
		nil, boolean, number, string, function, table
	};
	static std::string typeName(Type);

	using nil_t		= decltype(nullptr);
	using bool_t	= bool;
	using number_t	= double;
	using string_t	= const std::string;
	using function_t= const std::function<Var(Var)>;
	using table_t	= std::unordered_map<Var, Var>;
	class	Ref;
	class	Ite;
	struct	TypeError;

	//管理员：引用计数
	static std::unordered_map<const void*, int> mrc;
	static std::recursive_mutex mrcm;
	static const Var nil;

	//成员
	union {
		bool_t		b;
		number_t	n;
		string_t	*s;
		function_t	*f;
		table_t		*t;
	};
	mutable Type type	= Type::nil;
	mutable bool strong	= false;

	//构造
	Var() = default;
	Var(nil_t);
	Var(bool_t);
	Var(int);
	Var(unsigned);
	Var(number_t);
	Var(char const *);
	Var(std::string&&);
	Var(std::string const &);
	static Var function(function_t &);
	static Var table();
	Var(std::initializer_list<Var>);

	//Special Member Function
	~Var();
	Var(Var&&);
	Var(Var const &);
	Var & operator=(Var&&);
	Var & operator=(Var const &);

	//全类型
	explicit operator bool()const;
	bool operator!()const					{ return !(bool)*this; }

	//数字
	Var operator-()const;

	//函数
	Var operator()(Var)const;

	//表
	Ref operator[](Var)const;
	table_t::iterator begin()const;
	table_t::iterator end()const;
	table_t::const_iterator cbegin()const;
	table_t::const_iterator cend()const;

	//强弱转换
	void setWeak(bool val = true)const;
	void setKeyWeak(Var, bool val = true)const;
};

class Var::Ref
{
	table_t	*_tbl;
	Var		_key;
	bool	_strong;
	Var & get()const;
public:
	Ref(table_t *, bool, Var&&);
	Ref(Ref&&);
	Ref(Ref const &) = delete;
	Ref & operator=(Ref&&) = delete;
	Ref & operator=(Ref const &) = delete;
	Var & operator=(Var const &)const;
// 	operator Var &()const							{ return get(); }
// 	void setWeak(bool w)const						{ return get().setWeak(w); }
// 	void setKeyWeak(Var const & k, bool w)const		{ return get().setKeyWeak(k, w); }
// 	bool operator!()const							{ return !get(); }
// 	explicit operator bool()const					{ return (bool)get(); }
// 	Var operator-()const								{ return -get(); }
// 	Var operator()(Var const & v)const				{ return get()(v); }
// 	Ref operator[](Var const & k)const				{ return get()[k]; }
// 	Ite begin()const;
// 	Ite end()const;
};

class Var::Ite
	: public table_t::iterator
{
	table_t	*_tbl;
	bool	_strong;
public:
	using base = std::unordered_map<Var, Var>::iterator;
	Ite(base&&, table_t);
	Var k()const									{ return (*this)->first; }
	Var::Ref v()const										{ return{_tbl, _strong, k()}; }
	std::pair<Var const, Var::Ref> operator*()const		{ return {k(), v()}; }
};

//全类型
bool operator==(Var, Var);
inline bool operator!=(Var lhs, Var rhs)				{ return !(lhs == rhs); }
std::ostream & operator<<(std::ostream &, Var);
inline std::string type(Var var)						{ return Var::typeName(var.type); }

//数字、字符串
bool operator<(Var, Var);
bool operator>(Var, Var);
inline bool operator>=(Var lhs, Var rhs)		{ return !(lhs < rhs); }
inline bool operator<=(Var lhs, Var rhs)		{ return !(lhs > rhs); }
Var operator+(Var, Var);

//数字
Var operator-(Var, Var);
Var operator*(Var, Var);
Var operator/(Var, Var);
Var operator%(Var, Var);
Var operator^(Var, Var);
Var toNumber(Var);
template<typename Ty>
Ty toNumber(Var);

//字符串
Var toString(Var);
char const * toCString(Var);

//表
std::ostream & printTable(Var, std::ostream & os = std::cout);


//-------------------------------Implementation---------------------------------

struct Var::TypeError : std::runtime_error
{
	TypeError(Type, std::string const &);
	TypeError(Type, Type, std::string const &);
};


Var Var::function(function_t& fn)
{
	Var rtn;
	rtn.f = new std::function<Var(Var)>(fn);
	if (*rtn.f) {
		rtn.type = Type::function;
	} 
	else {
		delete rtn.f;
	}
	return rtn;
}


template<typename Ty>
Ty toNumber(Var var)
{
	Var n = toNumber(var);
	if (!n)
		throw Var::TypeError(var.type, __FUNCTION__);
	return (Ty)n.n;
}


#endif
