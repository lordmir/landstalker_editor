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

size_t LZ77::Encode(const uint8_t* inbuf, size_t bufsize, uint8_t* outbuf)
{
    size_t esize = 0;
    return esize;
}

