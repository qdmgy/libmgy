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
    public:
        virtual ~Coder();
        virtual Coder & toContraryCoder() = 0;
        virtual string code(string const &)const = 0;

        virtual char the63rdChar()const = 0;
        virtual void the63rdChar(char value) = 0;
        virtual char the64thChar()const = 0;
        virtual void the64thChar(char value) = 0;

        unsigned lineLengthMax()const;
        void lineLengthMax(unsigned value);
        string padding()const;
        void padding(string const & value);

    protected:
        static char const INVALID_NUMBER = -1;
        static string const FIRST_62_CHARS;

        string m_table;
        unsigned m_nLineMax;
        char m_szPad[4];

        explicit Coder(unsigned tableSize, unsigned lineMax = 76, string const & pad = "=");
    };


    class Encoder
        : public Coder
    {
    public:
        explicit Encoder(char ch64th = '/', char ch63rd = '+');
        Encoder(Coder const & rhs);

        virtual Coder & toContraryCoder();
        virtual string code(string const & buffer)const;

        virtual char the63rdChar()const;
        virtual void the63rdChar(char value);
        virtual char the64thChar()const;
        virtual void the64thChar(char value);

    private:
        string encode3Bytes(char const * bytes, size_t nByte)const;
    };


    class Decoder
        : public Coder
    {
    public:
        explicit Decoder(char ch64th = '/', char ch63rd = '+');
        Decoder(Coder const & rhs);

        virtual Coder & toContraryCoder();
        virtual string code(string const & strEncoded)const;

        virtual char the63rdChar()const;
        virtual void the63rdChar(char value);
        virtual char the64thChar()const;
        virtual void the64thChar(char value);

    private:
        string decode4Chars(istringstream & iss)const;
    };


    //----------Implementation------------

    inline
    unsigned Coder::lineLengthMax()const
    {
        return m_nLineMax;
    }

    inline
    void Coder::lineLengthMax(unsigned value)
    {
        m_nLineMax = value;
    }

    inline
    string Coder::padding()const
    {
        return m_szPad;
    }
}



#define __BASE64__H__
#endif
