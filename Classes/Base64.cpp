//
//  Base64.cpp
//  libmgy
//
//  Created by 旭游 on 13-5-14.
//  Copyright (c) 2013年 牟光远. All rights reserved.
//

#include "Base64.h"

#include <cassert>
#include <stdexcept>



namespace {
    using std::string;
    using base64::UINT32;


    bool IsBigEndian()
    {
        UINT32 n = 0x12345678;
        auto p = reinterpret_cast<unsigned char*>(&n);
        return *p == 0x12;
    }


    char const CR_LF[] = {'\r', '\n'};
    bool const k_isBigEndian = IsBigEndian();


    union UHigh3Bytes
    {
        UINT32 all;
        unsigned char bytes[4];

        UHigh3Bytes()
            : all(0)
        {
        }

        void init(unsigned char high, unsigned char middle, unsigned char low)
        {
            if(k_isBigEndian) {
                bytes[0] = high;
                bytes[1] = middle;
                bytes[2] = low;
                bytes[3] = 0;
            }
            else {
                bytes[3] = high;
                bytes[2] = middle;
                bytes[1] = low;
                bytes[0] = 0;
            }
        }

        string toString()const
        {
            string rtn(3, '\0');
            if(k_isBigEndian) {
                rtn[0] = bytes[0];
                rtn[1] = bytes[1];
                rtn[2] = bytes[2];
            }
            else {
                rtn[0] = bytes[3];
                rtn[1] = bytes[2];
                rtn[2] = bytes[1];
            }
            return rtn;
        }
    };
}



namespace base64 {
    string const Coder::FIRST_62_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    void Coder::padding(string const & value)
    {
        auto n = value.length();
        if(n > 3) {
            throw std::length_error("base64 - Padding string is longer than 3.");
        }
        value.copy(m_szPad, n, 0);
        m_szPad[n] = '\0';
    }

    Coder::Coder(unsigned tableSize, unsigned lineMax, string const & pad)
        : m_table(tableSize, INVALID_NUMBER)
        , m_nLineMax(lineMax)
    {
        padding(pad);
    }

    Coder::~Coder()
    {
    }


    Encoder::Encoder(char ch64th, char ch63rd)
        : Coder(64)
    {
        m_table = FIRST_62_CHARS;
        m_table.push_back(ch63rd);
        m_table.push_back(ch64th);
    }

    Encoder::Encoder(Coder const & rhs)
        : Coder(64, rhs.lineLengthMax(), rhs.padding())
    {
        m_table = FIRST_62_CHARS;
        m_table.push_back(rhs.the63rdChar());
        m_table.push_back(rhs.the64thChar());
    }

    Coder & Encoder::toContraryCoder()
    {
        auto p = dynamic_cast<void*>(this);
        auto pad = padding();
        auto lineMax = lineLengthMax();
        auto ch63rd = the63rdChar();
        auto ch64th = the64thChar();
        this->~Encoder();
        auto rtn = new(p) Decoder(ch64th, ch63rd);
        rtn->lineLengthMax(lineMax);
        rtn->padding(pad);
        return *rtn;
    }

    char Encoder::the63rdChar()const
    {
        return m_table[62];
    }

    void Encoder::the63rdChar(char value)
    {
        m_table[62] = value;
    }

    char Encoder::the64thChar()const
    {
        return m_table[63];
    }

    void Encoder::the64thChar(char value)
    {
        m_table[63] = value;
    }

    string Encoder::code(string const & buffer)const
    {
        auto bytes = buffer.data();
        auto nByte = buffer.size();
        auto endingSize = nByte % 3;
        if(endingSize) {
            ++endingSize;//encoded ending size
            //Add pad size:
            auto nPad = m_szPad[0] ? 4-endingSize : 0;
            (nPad)&&
                (endingSize += nPad * padding().length());
        }

        auto resultSize = nByte/3*4 + endingSize;
        if(m_nLineMax) {
            //Add CR_LF size:
            auto nCR_LF = resultSize / m_nLineMax;
            (resultSize % m_nLineMax)||
                --nCR_LF;
            resultSize += nCR_LF * sizeof(CR_LF);
        }

        string rtn;
        rtn.reserve(resultSize);

        for( size_t lineLength = 0; nByte; ) {
            //Encode 3 bytes:
            if(nByte >= 3) {
                rtn += encode3Bytes(bytes, 3);
                lineLength += 4;
                bytes += 3;
                nByte -= 3;
            }
            else {
                rtn += encode3Bytes(bytes, nByte);
                lineLength += endingSize;
                nByte = 0;
            }
            //New line:
            if(m_nLineMax && lineLength > m_nLineMax) {
                lineLength -= m_nLineMax;
                rtn.insert(rtn.length()-lineLength, CR_LF, sizeof(CR_LF));
            }
        }
        assert(rtn.size() == resultSize);
        return rtn;
    }

    string Encoder::encode3Bytes(char const * bytes, size_t nByte)const
    {
        UHigh3Bytes tribyte;
        tribyte.init(bytes[0], nByte>1?bytes[1]:0, nByte>2?bytes[2]:0);
        auto nPad = m_szPad[0] ? 3-nByte : 0;
        string rtn;
        rtn.reserve(12);

        for( ++nByte; nByte; --nByte ) {
            //Encode 6 bits:
            rtn.push_back(m_table[tribyte.all >> 26]);
            tribyte.all <<= 6;
        }
        for( ; nPad; --nPad ) {
            rtn += m_szPad;
        }
        return rtn;
    }


    Decoder::Decoder(char ch64th, char ch63rd)
        : Coder(256)
    {
        char i = 0;
        for(auto ch : FIRST_62_CHARS) {
            m_table[ch] = i++;
        }
        m_table[ch63rd] = i++;
        m_table[ch64th] = i;
    }

    Decoder::Decoder(Coder const & rhs)
        : Coder(256, rhs.lineLengthMax(), rhs.padding())
    {
        char i = 0;
        for(auto ch : FIRST_62_CHARS) {
            m_table[ch] = i++;
        }
        m_table[rhs.the63rdChar()] = i++;
        m_table[rhs.the64thChar()] = i;
    }

    Coder & Decoder::toContraryCoder()
    {
        auto p = dynamic_cast<void*>(this);
        auto pad = padding();
        auto lineMax = lineLengthMax();
        auto ch63rd = the63rdChar();
        auto ch64th = the64thChar();
        this->~Decoder();
        auto rtn = new(p) Encoder(ch64th, ch63rd);
        rtn->lineLengthMax(lineMax);
        rtn->padding(pad);
        return *rtn;
    }

    char Decoder::the63rdChar()const
    {
        return static_cast<char>(m_table.find(62));
    }

    void Decoder::the63rdChar(char value)
    {
        m_table[m_table.find(62)] = INVALID_NUMBER;
        m_table[value] = 62;
    }

    char Decoder::the64thChar()const
    {
        return static_cast<char>(m_table.find(63));
    }

    void Decoder::the64thChar(char value)
    {
        m_table[m_table.find(63)] = INVALID_NUMBER;
        m_table[value] = 63;
    }

    string Decoder::code(string const & strEncoded)const
    {
        string rtn;
        rtn.reserve(strEncoded.size() * 3 / 4);

        for( istringstream iss(strEncoded); iss.good(); rtn += decode4Chars(iss) );

        return rtn;
    }

    string Decoder::decode4Chars(istringstream & iss)const
    {
        char ch, nCh = 0;
        UHigh3Bytes tribyte;

        for( int i = 0; i != 4; ++i ) {
            //Get char:
            iss>>ch;
            if(!iss.good()) {
                ch = '\0';
            }
            else if(ch == m_szPad[0]) {
                for( int j = 1; m_szPad[j]; ++j ) {
                    iss>>ch;
                }
                ch = '\0';
            }
            //Decode char:
            tribyte.all <<= 6;
            ch = m_table[ch];
            if(ch != INVALID_NUMBER) {
                tribyte.all |= ch;
                ++nCh;
            }
        }
        tribyte.all <<= 8;

        if(nCh == 1) {
            throw std::runtime_error("base64 - Decoding a corrupted string.");
        }
        return (nCh)?
            tribyte.toString().substr(0, --nCh):
            string();
    }
}
