#ifndef TILEATTRIBUTES_H
#define TILEATTRIBUTES_H

class TileAttributes
{
public:
    TileAttributes();
    TileAttributes(bool hflip, bool vflip, bool priority);
    
    enum class Attribute {ATTR_HFLIP, ATTR_VFLIP, ATTR_PRIORITY};

    void setAttribute(const Attribute& attr);
    void clearAttribute(const Attribute& attr);
    bool toggleAttribute(const Attribute& attr);
    bool getAttribute(const Attribute& attr) const;
    
private:
    bool hflip_;
    bool vflip_;
    bool priority_;
};

#endif // TILEATTRIBUTES_H
