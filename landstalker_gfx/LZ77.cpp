#include "LZ77.h"
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <wx/wx.h>

LZ77::LZ77()
{
}

LZ77::~LZ77()
{
}

class BitBarrel
{
public:
    BitBarrel()
    : m_val(0),
      m_pos(0)
    {}
    bool empty()
    {
        return(m_pos == 0);
    }
    void newByte(uint8_t byte)
    {
            m_val = byte;
            m_pos = 8;
    }
    operator bool()
    {
        bool retval = false;
        if(m_pos == 0)
        {
            throw std::runtime_error("Out of bits!");
        }
        else
        {
            retval = (m_val >= 0x80);
            m_val <<= 1;
            --m_pos;
        }
        return retval;
    }
    bool full()
    {
        return (m_pos == 8);
    }
    bool operator()(bool rhs)
    {
        if(m_pos < 8)
        {
            m_val <<= 1;
            m_val |= (rhs ? 1 : 0);
            m_pos++;
        }
        return rhs;
    }
    uint8_t out()
    {
        m_pos = 0;
        return m_val;
    }
private:
    uint8_t m_val;
    uint8_t m_pos;
};

size_t LZ77::Decode(const uint8_t* inbuf, size_t bufsize, uint8_t* outbuf)
{
    size_t dsize = 0;
    const uint8_t* inbufptr = inbuf;
    BitBarrel cmd;
    
    for(;;)
    {
        if(cmd.empty())
        {
            cmd.newByte(*inbufptr++);
        }
        if(cmd)
        {
            *outbuf++ = *inbufptr++;
            dsize++;
        }
        else
        {
            if(dsize > 0x17F)
            {
                int a = dsize / 2;
                a++;
            }
            uint16_t offset = (*inbufptr & 0xF0) << 4 | *(inbufptr + 1);
            uint8_t length = 18 - (*inbufptr & 0x0F);
            inbufptr += 2;
            if(offset)
            {
                dsize += length;
                for(; length != 0; --length)
                {
                    *outbuf = *(outbuf - offset);
                    outbuf++;
                }
            }
            else
            {
                break;
            }
        }
    }
    
    return dsize;
}

struct Entry
{
    enum Type {T_END, T_BYTE, T_RUN} type;
    uint8_t entry1;
    uint16_t entry2;
    Entry(const Type& ptype, uint8_t pentry1, uint16_t pentry2)
    : type(ptype), entry1(pentry1), entry2(pentry2)
    {}
};

size_t LZ77::Encode(const uint8_t* inbuf, size_t bufsize, uint8_t* outbuf)
{
    size_t esize = 0;
    std::vector<Entry> entries;
    BitBarrel bb;
    for(size_t i = 0; i < bufsize; i++)
    {
        if((i > 0) && (i < (bufsize - 3)) && (*(inbuf - 1) == *inbuf) && (*(inbuf - 1) == *(inbuf + 1)) && (*(inbuf - 1) == *(inbuf + 2)))
        {
            entries.push_back(Entry(Entry::T_RUN, 3, 1));
            inbuf += 3;
        }
        else
        {
            entries.push_back(Entry(Entry::T_BYTE, *inbuf++, 0));
        }
    }
    entries.push_back(Entry(Entry::T_END,0,0));
    std::vector<Entry>::const_iterator it;
    std::vector<Entry>::const_iterator bstart = entries.begin();
    for(it = bstart; it != entries.end(); ++it)
    {
        if(bb.full())
        {
            *outbuf++ = bb.out();
            esize++;
            for( ; bstart != it; ++bstart)
            {
                switch(bstart->type)
                {
                    case Entry::T_BYTE:
                        *outbuf++ = bstart->entry1;
                        esize++;
                        break;
                    case Entry::T_RUN:
                        *outbuf++ = (bstart->entry2 & 0xF00) >> 8 | ((18 - bstart->entry1) & 0xF);
                        *outbuf++ = bstart->entry2 & 0xFF;
                        esize+=2;
                        break;
                    default:
                        *outbuf++ = 0x00;
                        *outbuf++ = 0x00;
                        esize+=2;
                }
            }
        }
        bb(it->type == Entry::T_BYTE);
    }
    return esize;
}

