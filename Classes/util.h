//
//  util.h
//  libmgy
//
//  Created by 牟光远 on 2013-8-29.
//  Copyright (c) 2013年 牟光远. All rights reserved.
//

#ifndef __UTIL__H__



#include <cstddef>



namespace util {
    using std::size_t;


    inline
    bool IsBigEndian()
    {
        int const I = 1;
        auto p = (char const*)&I;
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



#define __UTIL__H__
#endif
