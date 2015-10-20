//
//  Var.cpp
//  libmgy
//
//  Created by 牟光远 on 2015-4-29.
//  Copyright (c) 2015年 牟光远. All rights reserved.
//

#include "Var.hpp"

#include <iomanip>
using namespace std;


Var::TypeError::TypeError(Type t)
	: runtime_error(to_string(t) + " type has no such operation\n")
{
}


auto Var::begin()const
{
	if (m_type != Type::TABLE) {
		throw TypeError{m_type};
	}
	return m_tbl->begin();
}

auto Var::end()const
{
	if (m_type != Type::TABLE) {
		throw TypeError{m_type};
	}
	return m_tbl->end();
}

bool operator==(Var const & lhs, Var const & rhs) noexcept
{
	if (lhs.m_type != rhs.m_type) {
		return false;
	}
	else {
		switch (rhs.m_type) {
			case Var::Type::NIL:
				return true;
			case Var::Type::BOOLEAN:
				return lhs.m_bool == rhs.m_bool;
			case Var::Type::NUMBER:
				return lhs.m_num == rhs.m_num;
			case Var::Type::STRING:
				return lhs.m_str == rhs.m_str || *lhs.m_str == *rhs.m_str;
			case Var::Type::FUNCTION:
				return lhs.m_fn == rhs.m_fn;
			case Var::Type::TABLE:
				return lhs.m_tbl == rhs.m_tbl;
		}
	}
}

ostream & operator<<(ostream & os, Var const & rhs)
{
	switch (rhs.m_type) {
		case Var::Type::BOOLEAN:
			os<<boolalpha<<rhs.m_bool;
			break;
		case Var::Type::NUMBER:
			os<<rhs.m_num;
			break;
		case Var::Type::STRING:
			os<<"\""<<*rhs.m_str<<"\"";
			break;
		case Var::Type::TABLE:
			os<<"{\n";
			for (auto & pair : rhs) {
				os<<pair.first<<"="<<pair.second<<",\n";
			}
			os<<"}";
			break;
		default:
			os<<Var::to_string(rhs.m_type);
	}
	return os;
}

Var::operator bool() noexcept
{
	switch (m_type) {
		case Type::NIL:
			return false;
		case Type::BOOLEAN:
			return m_bool;
		case Type::FUNCTION:
			return (bool)*m_fn;
		default:
			return true;
	}
}

string Var::to_string(Type t)
{
	switch (t) {
		case Type::NIL:
			return "nil";
		case Type::BOOLEAN:
			return "boolean";
		case Type::NUMBER:
			return "number";
		case Type::STRING:
			return "string";
		case Type::FUNCTION:
			return "function";
		case Type::TABLE:
			return "table";
	}
}

Var::Var(initializer_list<Var> list)
	: m_tbl(new unordered_map<Var, Var> (list.size()))
	, m_type(Type::TABLE)
{
	int idx = 1;
	for (auto & var : list) {
		if (var != nullptr) {
			m_tbl->emplace(idx++, var);
		}
	}
}

Var::Var(Var const & rhs) noexcept
	: m_type(rhs.m_type)
{
	switch (m_type) {
		case Type::NIL:
			break;
		case Type::BOOLEAN:
			m_bool = rhs.m_bool;
			break;
		case Type::NUMBER:
			m_num = rhs.m_num;
			break;
		case Type::STRING:
			new(&m_str) decltype(m_str) {rhs.m_str};
			break;
		case Type::FUNCTION:
			new(&m_fn) decltype(m_fn) {rhs.m_fn};
			break;
		case Type::TABLE:
			new(&m_tbl) decltype(m_tbl) {rhs.m_tbl};
			break;
	}
}

Var::Var(Var && rhs) noexcept
	: m_type(rhs.m_type)
{
	switch (m_type) {
		case Type::NIL:
			break;
		case Type::BOOLEAN:
			m_bool = rhs.m_bool;
			break;
		case Type::NUMBER:
			m_num = rhs.m_num;
			break;
		case Type::STRING:
			new(&m_str) decltype(m_str) {std::move(rhs.m_str)};
			break;
		case Type::FUNCTION:
			new(&m_fn) decltype(m_fn) {std::move(rhs.m_fn)};
			break;
		case Type::TABLE:
			new(&m_tbl) decltype(m_tbl) {std::move(rhs.m_tbl)};
			break;
	}
}

Var::~Var() noexcept
{
	switch (m_type) {
		case Type::STRING:
			m_str = nullptr;
			break;
		case Type::FUNCTION:
			m_fn = nullptr;
			break;
		case Type::TABLE:
			m_tbl = nullptr;
			break;
		default:;
	}
}
