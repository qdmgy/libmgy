//
//  Var.hpp
//  libmgy
//
//  Created by 牟光远 on 2015-4-29.
//  Copyright (c) 2015年 牟光远. All rights reserved.
//

#ifndef __Var__hpp__
#define __Var__hpp__


#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>


class Var
{
public:
	using number_t = double;
	struct TypeError;

	friend std::hash<Var>;
	friend TypeError;
	friend bool operator==(Var const &, Var const &) noexcept;
	friend bool operator!=(Var const & lhs, Var const & rhs) noexcept
		{return!(lhs==rhs);}
	friend std::ostream & operator<<(std::ostream &, Var const &);

	explicit operator bool() noexcept;

	auto begin()const;
	auto end()const;

	void print()
		{std::cout<<*this;}

	std::string type()
		{return to_string(m_type);}

private:
	union{
		bool m_bool;
		number_t m_num;
		std::shared_ptr<std::string const> m_str;
		std::shared_ptr<std::function<Var(Var)>const> m_fn;
		std::shared_ptr<std::unordered_map<Var,Var>> m_tbl;
	};
	enum class Type
	{
		NIL = 0,
		BOOLEAN,
		NUMBER,
		STRING,
		FUNCTION,
		TABLE
	} const m_type;

	static std::string to_string(Type);

public:
	Var(std::nullptr_t v = nullptr) noexcept
		: m_type(Type::NIL) {}

	Var(bool v) noexcept
		: m_bool(v), m_type(Type::BOOLEAN) {}

	Var(int v) noexcept
		: m_num(v), m_type(Type::NUMBER) {}

	Var(number_t v) noexcept
		: m_num(v), m_type(Type::NUMBER) {}

	Var(char const * v)
		: m_str(new std::string (v)), m_type(Type::STRING) {}

	Var(std::string const & v)
		: m_str(new std::string (v)), m_type(Type::STRING) {}

	Var(std::function<Var(Var)> v)
		: m_fn(new decltype(v) (std::move(v))), m_type(Type::FUNCTION) {}

	Var(std::initializer_list<Var>);

	Var(Var const &) noexcept;
	Var(Var &&) noexcept;
	~Var() noexcept;
	Var & operator=(Var rhs) noexcept
		{this->~Var();return*new(this)Var(std::move(rhs));}
};


template<>
struct std::hash<Var>
{
	using argument_type = Var;
	using result_type = std::size_t;

	result_type operator()(argument_type const & var)const noexcept
	{
		switch (var.m_type) {
			case Var::Type::NIL:
				return std::hash<void*>{}(nullptr);
			case Var::Type::BOOLEAN:
				return std::hash<decltype(var.m_bool)>{}(var.m_bool);
			case Var::Type::NUMBER:
				return std::hash<decltype(var.m_num)>{}(var.m_num);
			case Var::Type::STRING:
				return std::hash<decltype(var.m_str)>{}(var.m_str);
			case Var::Type::FUNCTION:
				return std::hash<decltype(var.m_fn)>{}(var.m_fn);
			case Var::Type::TABLE:
				return std::hash<decltype(var.m_tbl)>{}(var.m_tbl);
		}
	}
};


struct Var::TypeError
	: public std::runtime_error
{
	explicit TypeError(Type);
};


#endif /* defined(__Var__hpp__) */
