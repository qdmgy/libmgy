//
//  util.hpp
//  libmgy
//
//  Created by 牟光远 on 2014-8-29.
//  Copyright (c) 2014年 牟光远. All rights reserved.
//

#ifndef __UTIL__HPP__
#define __UTIL__HPP__


#include <cstddef>


namespace util {
	using std::nullptr_t;
    using std::size_t;

    inline
    bool IsBigEndian()
    {
        constexpr int I = 1;
        auto p = (char*)&I;
        return !*p;
    }

    inline
    size_t RoundUp(size_t size, size_t to)
    {
        //TODO: assert "to" is power of 2...
        --to;
        return (size + to) & ~to;
    }
}


#endif
