//
//  Base64.cpp
//  libmgy
//
//  Created by 牟光远 on 2013-5-14.
//  Copyright (c) 2013年 牟光远. All rights reserved.
//

#include "Base64.h"
#include "util.hpp"

#include <cctype>
#include <stdexcept>



namespace {
    using std::string;
    using base64::UINT32;


    char const CR_LF[] = {'\r', '\n'};
    bool const k_isBigEndian = util::IsBigEndian();


    union UHigh3Bytes
    {
        typedef string::value_type BYTE;

        UINT32 all;
        BYTE bytes[4];

        UHigh3Bytes()
            : all(0)
        {
        }

        void init(BYTE high, BYTE middle, BYTE low)
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
    inline
    std::runtime_error BadCoder()
    {
        return std::runtime_error("base64 - Corrupted coder.");
    }

    inline
    std::runtime_error BadBase64String()
    {
        return std::runtime_error("base64 - Decoding a corrupted string.");
    }


    string const Coder::FIRST_62_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    void Coder::toContraryCoder()
    {
        switch(m_table.size()) {
            case ENCODE_TABLE_SIZE:
                return initDecodeTable(the63rdChar(), the64thChar());
            case DECODE_TABLE_SIZE:
                return initEncodeTable(the63rdChar(), the64thChar());
            default:
                throw BadCoder();
        }
    }

    string Coder::code(void const * p, size_t nByte)const
    {
        switch(m_table.size()) {
            case ENCODE_TABLE_SIZE:
                return encodeBuffer((char const*)p, nByte);
            case DECODE_TABLE_SIZE:
                return decodeString((unsigned char const*)p, nByte);
            default:
                throw BadCoder();
        }
    }

    char Coder::the63rdChar()const
    {
        switch(m_table.size()) {
            case ENCODE_TABLE_SIZE:
                return m_table[62];
            case DECODE_TABLE_SIZE:
                return char(m_table.find(62));
            default:
                throw BadCoder();
        }
    }

    void Coder::the63rdChar(char value)
    {
        switch(m_table.size()) {
            case ENCODE_TABLE_SIZE:
                m_table[62] = value;
                break;
            case DECODE_TABLE_SIZE:
                m_table[m_table.find(62)] = DECODE_UNKNOWN;
                m_table[(unsigned char)value] = 62;
                break;
            default:
                throw BadCoder();
        }
    }

    char Coder::the64thChar()const
    {
        switch(m_table.size()) {
            case ENCODE_TABLE_SIZE:
                return m_table[63];
            case DECODE_TABLE_SIZE:
                return char(m_table.find(63));
            default:
                throw BadCoder();
        }
    }

    void Coder::the64thChar(char value)
    {
        switch(m_table.size()) {
            case ENCODE_TABLE_SIZE:
                m_table[63] = value;
                break;
            case DECODE_TABLE_SIZE:
                m_table[m_table.find(63)] = DECODE_UNKNOWN;
                m_table[(unsigned char)value] = 63;
                break;
            default:
                throw BadCoder();
        }
    }

    void Coder::pad(string const & value)
    {
        auto n = value.length();
        if(n > 3) {
            throw std::length_error("base64 - Length of pad exceeds 3.");
        }
        value.copy(m_szPad, n, 0);
        m_szPad[n] = '\0';
    }

    void Coder::lineLengthMax(int value)
    {
        if(value < 0 || 76 < value) {
            throw std::out_of_range("base64 - Max length of line is out of [0,76].");
        }
        m_iLineMax = onlyDecodeKnownChars() ? value : ~value;
    }

    Coder::Coder(unsigned tableSize, char ch63rd, char ch64th,
                 string const & pad, int lineMax, bool only64Chars)
        : m_iLineMax(only64Chars ? lineMax : ~lineMax)
    {
        this->pad(pad);
        switch(tableSize) {
            case ENCODE_TABLE_SIZE:
                initEncodeTable(ch63rd, ch64th);
                break;
            case DECODE_TABLE_SIZE:
                initDecodeTable(ch63rd, ch64th);
                break;
            default:
                throw std::invalid_argument("base64 - Invalid table size.");
        }
    }

    void Coder::initEncodeTable(char ch63rd, char ch64th)
    {
        if(m_table.size() == ENCODE_TABLE_SIZE) {
            return;
        }
        m_table.reserve(ENCODE_TABLE_SIZE);
        m_table = FIRST_62_CHARS;
        m_table.push_back(ch63rd);
        m_table.push_back(ch64th);
    }

    void Coder::initDecodeTable(unsigned char ch63rd, unsigned char ch64th)
    {
        if(m_table.size() == DECODE_TABLE_SIZE) {
            return;
        }
        string table(DECODE_TABLE_SIZE, DECODE_UNKNOWN);
        char i = 0;
        for(auto ch : FIRST_62_CHARS) {
            table[ch] = i++;
        }
        table[ch63rd] = i++;
        table[ch64th] = i;
        m_table.swap(table);
    }

    string Coder::encodeBuffer(char const * bytes, size_t nByte)const
    {
        auto endingSize = nByte % 3;
        if(endingSize) {
            ++endingSize;//encoded ending size
            //Add pad size:
            auto nPad = m_szPad[0] ? 4-endingSize : 0;
            (nPad)&&
                (endingSize += nPad * pad().length());
        }

        auto resultSize = nByte/3*4 + endingSize;
        size_t const k_lineMax = lineLengthMax();
        if(k_lineMax) {
            //Add CR_LF size:
            auto nCR_LF = resultSize / k_lineMax;
            (resultSize % k_lineMax)||
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
            if(k_lineMax && lineLength > k_lineMax) {
                lineLength -= k_lineMax;
                rtn.insert(rtn.length()-lineLength, CR_LF, sizeof(CR_LF));
            }
        }
        if(rtn.size() != resultSize) {
            throw std::logic_error("base64 - Wrong encoder algorithm.");
        }
        return rtn;
    }

    string Coder::encode3Bytes(char const * bytes, size_t nByte)const
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

    string Coder::decodeString(unsigned char const * bytes, size_t nByte)const
    {
        string rtn;
        rtn.reserve(nByte * 3 / 4);

        for( ; nByte; rtn += decode4Chars(bytes, nByte) );

        return rtn;
    }

    string Coder::decode4Chars(unsigned char const* & bytes, size_t & nByte)const
    {
        unsigned char ch, nCh = 0;
        UHigh3Bytes tribyte;

        for( int i = 0; i != 4; ++i ) {
            //Skip white space:
            for( ; std::isspace(*bytes); ++bytes ) {
                nByte && --nByte;
            }
            if(!nByte) {
                tribyte.all <<= 6;
                continue;
            }
            //Get char:
            ch = *bytes++;
            --nByte;
            if(ch == m_szPad[0]) {
                for( int j = 1; m_szPad[j]; ++j ) {
                    if(*bytes == m_szPad[j]) {
                        ++bytes;
                        nByte && --nByte;
                    }
                }
                tribyte.all <<= 6;
                continue;
            }
            //Decode char:
            ch = m_table[ch];
            if(ch != (unsigned char)DECODE_UNKNOWN) {
                tribyte.all <<= 6;
                tribyte.all |= ch;
                ++nCh;
            }
            else if(onlyDecodeKnownChars()) {
                throw BadBase64String();
            }
            else {
                --i;
            }
        }
        tribyte.all <<= 8;
        if(nCh == 1) {
            throw BadBase64String();
        }
        return (nCh)?
            tribyte.toString().substr(0, --nCh):
            string();
    }
}
