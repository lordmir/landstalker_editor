#include "LZ77.h"
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <wx/wx.h>

#include "BitBarrel.h"

std::size_t LZ77::Decode(const uint8_t* inbuf, std::size_t bufsize, uint8_t* outbuf, std::size_t& esize)
{
    std::size_t dsize = 0;
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
    esize = inbufptr - inbuf;
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

static uint8_t find_best_match(const uint8_t* inbuf, std::size_t bufsize, std::size_t curpos, uint16_t& offset)
{
    const uint8_t LEN_MAX_LIMIT = 18;
    const uint8_t LEN_MIN_LIMIT = 3;
    const uint16_t MAX_OFFSET = 4095;
    uint8_t best_len = 0;
    if((inbuf != NULL) && (bufsize > 3) && (curpos > 0))
    {
        const uint16_t END_SEARCH = (curpos > MAX_OFFSET) ? curpos - MAX_OFFSET : 0;
        const uint8_t MAX_LEN = static_cast<uint8_t>(std::min(static_cast<std::size_t>(LEN_MAX_LIMIT), bufsize - curpos));
        uint8_t len = 0;
        if(MAX_LEN < LEN_MAX_LIMIT)
        {
            len = 1;
        }
        if(MAX_LEN >= LEN_MIN_LIMIT)
        {
            for(std::size_t i = curpos; i > END_SEARCH; --i)
            {
                len = 0;
                while(inbuf[len + i - 1] == inbuf[curpos + len])
                {
                    len++;
                    if(len == MAX_LEN)
                    {
                        break;
                    }
                }
                if(len > best_len)
                {
                    best_len = len;
                    offset = static_cast<uint16_t>(curpos - i + 1);
                    if(best_len == MAX_LEN)
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            best_len = 1;
        }
    }
    return best_len;
}

std::size_t LZ77::Encode(const uint8_t* inbuf, std::size_t bufsize, uint8_t* outbuf)
{
    std::size_t esize = 0;
    std::vector<Entry> entries;
    BitBarrel bb;
    for(std::size_t i = 0; i < bufsize; )
    {
        uint16_t match_offset = 0;
        uint8_t match_len = find_best_match(inbuf, bufsize, i, match_offset);
        
        if(match_len >= 3)
        {
            entries.push_back(Entry(Entry::T_RUN, match_len, match_offset));
            i += match_len;
        }
        else
        {
            entries.push_back(Entry(Entry::T_BYTE, inbuf[i++], 0));
        }
    }
    entries.push_back(Entry(Entry::T_END,0,0));
    std::vector<Entry>::const_iterator it;
    std::vector<Entry>::const_iterator bstart = entries.begin();
    for(it = bstart; ; ++it)
    {
        if(bb.full() || ((it == entries.end()) && !bb.empty()))
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
                        assert((bstart->entry2 & 0xFFF) != 0);
                        *outbuf++ = (bstart->entry2 & 0xF00) >> 4 | ((18 - bstart->entry1) & 0xF);
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
        if(it == entries.end())
        {
            break;
        }
        bb(it->type == Entry::T_BYTE);
    }
    return esize;
}

