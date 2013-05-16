//
//  Base64.h
//  libmgy
//
//  Created by 旭游 on 13-5-14.
//  Copyright (c) 2013年 牟光远. All rights reserved.
//

#ifndef __BASE64__H__



#include <cstddef>
#include <string>
#include <sstream>



namespace base64 {
    using std::size_t;
    using std::string;
    using std::istringstream;


    typedef unsigned UINT32;
    static_assert(sizeof(UINT32) == 4, "Wrong definition of base64::UINT32!");


    class Coder
    {
        friend Coder Encoder();
        friend Coder Decoder();
        friend Coder * newEncoder();
        friend Coder * newDecoder();

    public:
        void toContraryCoder();
        string code(string const &)const;

        char the63rdChar()const;
        void the63rdChar(char value);
        char the64thChar()const;
        void the64thChar(char value);
        string pad()const;
        void   pad(string const & value);
        int  lineLengthMax()const;
        void lineLengthMax(int value);
        bool onlyDecodeKnownChars()const;
        void onlyDecodeKnownChars(bool value);

    private:
        string m_table;
        int m_iLineMax;
        char m_szPad[4];

        static string const FIRST_62_CHARS;
        static unsigned const ENCODE_TABLE_SIZE = 64;
        static unsigned const DECODE_TABLE_SIZE = 256;
        static char const DECODE_UNKNOWN = -1;

        Coder(unsigned tableSize, char ch63rd = '+', char ch64th = '/',
              string const & pad = "=", int lineMax = 64, bool only64Chars = true);
        void initEncodeTable(char ch63rd, char ch64th);
        void initDecodeTable(char ch63rd, char ch64th);
        string encodeBuffer(string const & buffer)const;
        string encode3Bytes(char const * bytes, size_t nByte)const;
        string decodeString(string const & strEncoded)const;
        string decode4Chars(istringstream & iss)const;
    };


    //----------Implementation------------

    inline
    Coder Encoder()
    {
        return Coder::ENCODE_TABLE_SIZE;
    }

    inline
    Coder Decoder()
    {
        return Coder::DECODE_TABLE_SIZE;
    }

    inline
    Coder * newEncoder()
    {
        return new Coder(Coder::ENCODE_TABLE_SIZE);
    }

    inline
    Coder * newDecoder()
    {
        return new Coder(Coder::DECODE_TABLE_SIZE);
    }

    inline
    string Coder::pad()const
    {
        return m_szPad;
    }

    inline
    int Coder::lineLengthMax()const
    {
        return onlyDecodeKnownChars() ? m_iLineMax : ~m_iLineMax;
    }

    inline
    bool Coder::onlyDecodeKnownChars()const
    {
        return m_iLineMax >= 0;
    }

    inline
    void Coder::onlyDecodeKnownChars(bool value)
    {
        value != onlyDecodeKnownChars() && (m_iLineMax = ~m_iLineMax);
    }
}



#define __BASE64__H__
#endif
