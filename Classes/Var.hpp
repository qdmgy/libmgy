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
		size_t operator()(Var const &)const noexcept;
	};
}


struct Var
{
	//类型
	enum class Type : char {
		nil, boolean, number, string, function, table
	};
	static std::string TypeName(Type) noexcept;

	using nil_t		= decltype(nullptr);
	using bool_t	= bool;
	using number_t	= double;
	using string_t	= const std::string;
	using function_t= const std::function<Var(Var)>;
	using table_t	= std::unordered_map<Var, Var>;
	class	Ref;
	struct	TypeError;

	//管理员：引用计数
	static std::unordered_map<void const*, int> mrc;
	static std::recursive_mutex mrcm;
	static const Var nil;

	//成员
	union {
		bool_t		b;
		number_t	n;
		string_t	*s;
		function_t	*f;
		table_t		*t	= 0;
	};
	mutable Type type	= Type::nil;
	mutable bool strong	= false;

	//构造
	constexpr Var() noexcept					{}
	constexpr Var(nil_t) noexcept				{}
	constexpr Var(bool_t val) noexcept			: b(val), type(Type::boolean) {}
	constexpr Var(number_t val) noexcept		: n(val), type(Type::number) {}
	constexpr Var(int val) noexcept				: n(val), type(Type::number) {}
	constexpr Var(unsigned val) noexcept		: n(val), type(Type::number) {}
	Var(char const *);
	Var(std::string&&);
	Var(std::string const &);
	static Var function(function_t &);
	static Var table();
	Var(std::initializer_list<Var>);

	//Special Member Function
	~Var() noexcept;
	Var(Var const &);
	Var & operator=(Var const &);
	Var(Var&&) noexcept;
	Var & operator=(Var && rhs) noexcept		{ swap(rhs); return *this; }

	//全类型
	void swap(Var &) noexcept;
	explicit operator bool()const noexcept;
	bool operator!()const noexcept				{ return !(bool)*this; }

	//数字
	Var operator-()const;

	//函数
	Var operator()(Var const &)const;
	Var operator()(Var&&)const;

	//表
	Ref operator[](Var)const;
	table_t::iterator begin()const;
	table_t::iterator end()const;
	table_t::const_iterator cbegin()const;
	table_t::const_iterator cend()const;

	//强弱转换
	bool setWeak(bool weak = true)const;
	bool setKeyWeak(Var, bool weak = true)const;
};

class Var::Ref
{
	Var _key;
	Var const * _tbl;

	friend Var;
	Ref(Var && k, Var const * t) noexcept			: _key(std::move(k)), _tbl(t) {}
	Ref(Ref&&) noexcept								= default;
	Var * get();

public:
	Ref(Ref const &)								= delete;
	Ref & operator=(Ref const &)					= delete;

	Var & operator=(Ref && rhs);
	Var & operator=(Var const &);
	Var & operator=(Var&&);
	operator Var &();

	void swap(Ref && rhs);
	void swap(Var & rhs);
	explicit operator bool();
	bool operator!()								{ return !(bool)*this; }

	Var operator-();
	Var operator()(Var const &);
	Var operator()(Var&&);
	Ref operator[](Var const &);
	Ref operator[](Var&&);
	table_t::iterator begin();
	table_t::iterator end();
	table_t::const_iterator cbegin();
	table_t::const_iterator cend();
	bool setWeak(bool);
	bool setKeyWeak(Var const &, bool);
	bool setKeyWeak(Var&&, bool);
};

//全类型
bool operator==(Var const &, Var const &);
inline bool operator!=(Var const & lhs, Var const & rhs)	{ return !(lhs == rhs); }
std::ostream & operator<<(std::ostream &, Var const &);
inline std::string type(Var const & var)					{ !var; return Var::TypeName(var.type); }

//数字、字符串
bool operator<(Var const &, Var const &);
bool operator>(Var const &, Var const &);
inline bool operator>=(Var const & lhs, Var const & rhs)	{ return !(lhs < rhs); }
inline bool operator<=(Var const & lhs, Var const & rhs)	{ return !(lhs > rhs); }
Var operator+(Var const &, Var const &);

//数字
Var operator-(Var const &, Var const &);
Var operator*(Var const &, Var const &);
Var operator/(Var const &, Var const &);
Var operator%(Var const &, Var const &);
Var operator^(Var const &, Var const &);
Var toNumber(Var const &);
template<typename Ty>
Ty toNumber(Var const &);

//字符串
Var toString(Var const &);
char const * toCString(Var const &);

//表
std::ostream & printTable(Var const &, std::ostream & rtn = std::cout);


//-------------------------------Implementation---------------------------------

struct Var::TypeError : std::runtime_error
{
	TypeError(Type, std::string const &);
	TypeError(Type, Type, std::string const &);
};


template<typename Ty>
Ty toNumber(Var const & var)
{
	Var n = toNumber(var);
	if (n)
		return (Ty)n.n;
	throw Var::TypeError(var.type, __FUNCTION__);
}


#endif
