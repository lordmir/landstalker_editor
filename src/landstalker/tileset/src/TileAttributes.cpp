#include <landstalker/tileset/include/TileAttributes.h>

#include <stdexcept>

TileAttributes::TileAttributes()
: hflip_(false),
  vflip_(false),
  priority_(false)
{
}

TileAttributes::TileAttributes(bool hflip, bool vflip, bool priority)
: hflip_(hflip),
  vflip_(vflip),
  priority_(priority)
{
}

void TileAttributes::setAttribute(const TileAttributes::Attribute& attr)
{
    switch(attr)
    {
        case TileAttributes::Attribute::ATTR_HFLIP:
            hflip_ = true;
            break;
        case TileAttributes::Attribute::ATTR_VFLIP:
            vflip_ = true;
            break;
        case TileAttributes::Attribute::ATTR_PRIORITY:
            priority_ = true;
            break;
        default:
            throw(std::runtime_error("Bad Attribute Type!"));
    }
}

void TileAttributes::clearAttribute(const TileAttributes::Attribute& attr)
{
    switch(attr)
    {
        case TileAttributes::Attribute::ATTR_HFLIP:
            hflip_ = false;
            break;
        case TileAttributes::Attribute::ATTR_VFLIP:
            vflip_ = false;
            break;
        case TileAttributes::Attribute::ATTR_PRIORITY:
            priority_ = false;
            break;
        default:
            throw(std::runtime_error("Bad Attribute Type!"));
    }
}

bool TileAttributes::toggleAttribute(const Attribute& attr)
{
    bool retval = false;
    switch (attr)
    {
    case TileAttributes::Attribute::ATTR_HFLIP:
        hflip_ = !hflip_;
        retval = hflip_;
        break;
    case TileAttributes::Attribute::ATTR_VFLIP:
        vflip_ = !vflip_;
        retval = vflip_;
        break;
    case TileAttributes::Attribute::ATTR_PRIORITY:
        priority_ = !priority_;
        retval = priority_;
        break;
    default:
        throw(std::runtime_error("Bad Attribute Type!"));
    }
    return retval;
}

bool TileAttributes::getAttribute(const TileAttributes::Attribute& attr) const
{
    bool retval = false;
    switch(attr)
    {
        case TileAttributes::Attribute::ATTR_HFLIP:
            retval = hflip_;
            break;
        case TileAttributes::Attribute::ATTR_VFLIP:
            retval = vflip_;
            break;
        case TileAttributes::Attribute::ATTR_PRIORITY:
            retval = priority_;
            break;
        default:
            throw(std::runtime_error("Bad Attribute Type!"));
    }
    return retval;
}
