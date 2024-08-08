#include <landstalker/misc/include/LZ77.h>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <iomanip>

#include <landstalker/misc/include/BitBarrel.h>

size_t LZ77::Decode(const uint8_t* inbuf, size_t bufsize, uint8_t* outbuf, size_t& esize)
{
    size_t dsize = 0;
    const uint8_t* inbufptr = inbuf;
    BitBarrel cmd;
    int a = 0;
    
    while(static_cast<size_t>(inbufptr - inbuf) < bufsize)
    {
        if(cmd.empty())
        {
            cmd.newByte(*inbufptr++);
        }
        if(cmd)
        {
            *outbuf++ = *inbufptr++;
            dsize++;
            a++;
        }
        else
        {
            uint16_t offset = (*inbufptr & 0xF0) << 4 | *(inbufptr + 1);
            uint8_t length = 18 - (*inbufptr & 0x0F);
            inbufptr += 2;
#ifndef NDEBUG
            if (a)
            {
                std::cout << "C[" << a << "]; ";
                for (int z = a; z > 0; --z)
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)*(outbuf - z) << std::dec;
                std::cout << std::endl;
                a = 0;
            }
#endif
            if(offset)
            {
                dsize += length;
                for(; length != 0; --length)
                {
                    *outbuf = *(outbuf - offset);
                    outbuf++;
                }
#ifndef NDEBUG
                int l = length;
                std::cout << "O[" << offset << "," << (int)l << "]; ";
                for (int z = l; z > 0; --z)
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)*(outbuf - offset - z) << std::dec;
                std::cout << std::endl;
#endif
            }
            else
            {
#ifndef NDEBUG
                std::cout << "END;" << std::endl;
#endif
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

static uint8_t find_best_match(const uint8_t* inbuf, size_t bufsize, size_t curpos, uint16_t& offset)
{
    const uint8_t LEN_MAX_LIMIT = 18;
    const uint8_t LEN_MIN_LIMIT = 3;
    const uint16_t MAX_OFFSET = 4095;
    uint8_t best_len = 0;
    if((inbuf != NULL) && (bufsize > 3) && (curpos > 0))
    {
        const uint16_t END_SEARCH = (curpos > MAX_OFFSET) ? static_cast<uint16_t>(curpos - MAX_OFFSET) : 0;
        const uint8_t MAX_LEN = static_cast<uint8_t>(std::min(static_cast<size_t>(LEN_MAX_LIMIT), bufsize - curpos));
        uint8_t len = 0;
        if(MAX_LEN < LEN_MAX_LIMIT)
        {
            len = 1;
        }
        if(MAX_LEN >= LEN_MIN_LIMIT)
        {
            for(size_t i = curpos; i > END_SEARCH; --i)
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

size_t LZ77::Encode(const uint8_t* inbuf, size_t bufsize, uint8_t* outbuf)
{
    size_t esize = 0;
    std::vector<Entry> entries;
    BitBarrel bb;
    size_t i = 0;
    while(i < bufsize)
    {
        uint16_t match_offset = 0;
        uint8_t match_len = find_best_match(inbuf, bufsize, i, match_offset);
        
        if(match_len >= 3)
        {
            uint16_t tmatch_offset = 0;
            // Do the non-greedy optimisation - look at next offset and see if we find a longer run.
            uint8_t tmatch_len = find_best_match(inbuf, bufsize, i + 1, tmatch_offset);
            if (tmatch_len > match_len)
            {
                match_offset = tmatch_offset;
                match_len = tmatch_len;
                entries.push_back(Entry(Entry::T_BYTE, inbuf[i++], 0));
            }
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
    int a = 0;
    for(it = bstart; ; ++it)
    {
        if(bb.full() || it == entries.end())
        {
            while (!bb.full())
            {
                bb(0);
            }
            *outbuf++ = bb.out();
            esize++;
            for( ; bstart != it; ++bstart)
            {
                switch(bstart->type)
                {
                    case Entry::T_BYTE:
                        *outbuf++ = bstart->entry1;
                        esize++;
                        a++;
                        break;
                    case Entry::T_RUN:
#ifndef NDEBUG
                        if (a)
                        {
                            std::cout << "C[" << a << "];" << std::endl;
                            a = 0;
                        }
                        std::cout << "O[" << bstart->entry2 << "," << (int)bstart->entry1 << "];" << std::endl;
#endif
                        assert((bstart->entry2 & 0xFFF) != 0);
                        *outbuf++ = static_cast<uint8_t>((bstart->entry2 & 0xF00) >> 4 | ((18 - bstart->entry1) & 0xF));
                        *outbuf++ = bstart->entry2 & 0xFF;
                        esize+=2;
                        break;
                    default:
#ifndef NDEBUG
                        if (a)
                        {
                            std::cout << "C[" << a << "];" << std::endl;
                            a = 0;
                        }
                        std::cout << "END;" << std::endl;
#endif
                        *outbuf++ = 0x00;
                        *outbuf++ = 0x00;
                        esize+=2;
                        return esize;
                }
            }
        }
        bb(it->type == Entry::T_BYTE);
    }
}

