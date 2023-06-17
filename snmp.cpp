/*
 * Copyright (C) 2016  roberto64 <mju7ki89@outlook.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <iostream>
#include <algorithm>
#include <bitset>
#include <typeinfo>
#include <sstream>
#include <iterator>
#include <cassert>
#include "snmp.h"

namespace snmp
{

    Except::Except(const Abstract* _id,Code code) throw() : str(typeid(*_id).name())
    {
		str += ":";
		str += message[code]; 
    }
	
	const char* Except::what() const throw()
	{
		return str.c_str();
	}
    
	const std::string Except::message[3] = {"Bad type","Protocol error","Incorrect OID"};

    MultibyteLen::MultibyteLen(std::uint32_t val) : value(val) 
    {

        if(value <= 127)
        {
            _size = 1;
        }
        else if(value <= 0xff)
        {
            _size = 2;
        }
        else if(value <= 0xffff)
        {
            _size = 3;
        }
        else if(value <= 0xffffff)
        {
            _size = 4;
        }
        else if(value <= 0xffffff)
        {
            _size = 5;
        }
    }

    std::vector<std::uint8_t>::const_iterator MultibyteLen::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        if(e - b >= 1)
        {
            std::uint8_t l = *b;
            b++;
            std::bitset<8> b0(l);
            if(b0[7]) // multibyte
            {
                value = 0;
                b0.reset(7);
                l = std::uint8_t(b0.to_ulong());
                if(l > sizeof(value) || l > e - b)
                    throw Except(this,Except::proto_error);
                std::reverse_copy(b,b+l,reinterpret_cast<std::uint8_t*>(&value));
                b += l;
                _size = 1 + l;
            }
            else
            {
                value = l;
                _size = 1;
            }
        }
        else
            throw Except(this,Except::proto_error);
        return  b;
    }

    void MultibyteLen::write(std::vector<std::uint8_t>& d) const
    {

        if(_size == 1)
        {
            d.push_back(value);
        }
        else
        {
            size_t len = _size - 1;
            d.push_back(0x80 | len);
            std::reverse_copy(reinterpret_cast<const std::uint8_t*>(&value),reinterpret_cast<const std::uint8_t*>(&value)+len,std::inserter(d,d.end()));
        }
    }

    MultibyteValue::MultibyteValue(std::uint64_t val) : value(val) 
    {
        if(value <= 0x7f)
        {
            _size = 1;
        }
        else if(value <= 0x3fff)
        {
            _size = 2;
        }
        else if(value <= 0x3fffff)
        {
            _size = 3;
        }
        else if(value <= 0x3fffff)
        {
            _size = 4;
        }
        else if(value <= 0x3fffffff)
        {
            _size = 5;
        }
    }

    std::vector<std::uint8_t>::const_iterator MultibyteValue::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        _size = 0;
        bool is_set(false);
        std::vector<std::uint8_t> tmp;
        do
        {
            if(e - b < 1)
                throw Except(this,Except::proto_error);
            std::bitset<8> b0(*b);
            is_set = b0[7];
            b0.reset(7);
            tmp.push_back(b0.to_ulong());
            _size++;
            b++;
        } while(is_set);
        value = 0;
        for(std::vector<std::uint8_t>::const_iterator i = tmp.begin();i != tmp.end();i++)
        {
            value = (value * 128) + *i;
        }
        return  b;
    }

    void MultibyteValue::write(std::vector<std::uint8_t>& d) const
    {
        if(_size == 1)
        {
            d.push_back(value);
        }
        else if(_size == 2)
        {
            std::uint32_t v0 = value % 128;
            std::uint32_t v1 = (value - (value % 128)) / 128;
            d.push_back(v1 |= 0x80);
            d.push_back(v0);
        }
        else if(_size == 3)
        {
            std::uint32_t v0 = value % 128;
            std::uint32_t v1 = (value - (value % 128)) / 128;
            std::uint32_t v2 = (v1 - (v1 % 128)) / 128;
            d.push_back(v2 |= 0x80);
            d.push_back(v1 |= 0x80);
            d.push_back(v0);
        }
        else if(_size == 4)
        {
            std::uint32_t v0 = value % 128;
            std::uint32_t v1 = (value - (value % 128)) / 128;
            std::uint32_t v2 = (v1 - (v1 % 128)) / 128;
            std::uint32_t v3 = (v2 - (v2 % 128)) / 128;
            d.push_back(v3 |= 0x80);
            d.push_back(v2 |= 0x80);
            d.push_back(v1 |= 0x80);
            d.push_back(v0);
        }
        else if(_size == 5)
        {
            std::uint64_t v0 = value % 128;
            std::uint64_t v1 = (value - (value % 128)) / 128;
            std::uint64_t v2 = (v1 - (v1 % 128)) / 128;
            std::uint64_t v3 = (v2 - (v2 % 128)) / 128;
            std::uint64_t v4 = (v3 - (v3 % 128)) / 128;
            d.push_back(v4 |= 0x80);
            d.push_back(v3 |= 0x80);
            d.push_back(v2 |= 0x80);
            d.push_back(v1 |= 0x80);
            d.push_back(v0);
        }
    }

    std::vector<std::uint8_t>::const_iterator Middle::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        if(e - b > 1)
        {
            type = *b;
            b++;
            b = length.read(b,e);
            _size = 1 + length.getSize();
            if(length > size_t(e - b))
                throw Except(this,Except::proto_error);
        }
        else
            throw Except(this,Except::proto_error);
        return b;
    }

    void Middle::write(std::vector<std::uint8_t>& d) const
    {
        d.push_back(type);
        length.write(d);
    }

    std::vector<std::uint8_t>::const_iterator Primitive::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        return Middle::read(b,e);
    }
        
    void Primitive::write(std::vector<std::uint8_t>& d) const
    {
        Middle::write(d);
    }

    std::vector<std::uint8_t>::const_iterator Null::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        b = Primitive::read(b,e);
        if(type != tnull)
            throw Except(this,Except::bad_type);
        if(length != 0)
            throw Except(this,Except::proto_error);
        _size = 2;
        return b;
    }
    
    void Null::write(std::vector<std::uint8_t>& d) const
    {
        Primitive::write(d);
    }

    Integer::Integer(int32_t val)
    {
        type = tinteger;
        if((std::uint32_t)val <= 0xff)
            length = 1;
        else if((std::uint32_t)val <= 0xffff)
            length = 2;
        else if((std::uint32_t)val <= 0xffffff)
            length = 3;
        else
            length = 4;
        value = val;
        _size = sizeof(type) + length.getSize() + length;
    }

    std::vector<std::uint8_t>::const_iterator Integer::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        b = Primitive::read(b,e);
        if(type != tinteger)
            throw Except(this,Except::bad_type);
        value = 0;
        if(e - b >= (int32_t)length && length <= sizeof(value))
        {
            std::reverse_copy(b,b+length,reinterpret_cast<std::uint8_t*>(&value));
            b = b+length;
        }
        _size += length;
        return b;
    }
    
    void Integer::write(std::vector<std::uint8_t>& d) const
    {
        Primitive::write(d);
        std::reverse_copy(reinterpret_cast<const std::uint8_t*>(&value),reinterpret_cast<const std::uint8_t*>(&value)+length.getValue(),std::inserter(d,d.end()));
    }
    
    Counter::Counter(std::uint32_t val)
    {
        type = tcounter;
        if(val <= 0xff)
            length = 1;
        else if(val <= 0xffff)
            length = 2;
        else if(val <= 0xffffff)
            length = 3;
        else
            length = 4;
        value = val;
        _size = sizeof(type) + length.getSize() + length;
    }

    std::vector<std::uint8_t>::const_iterator Counter::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        b = Primitive::read(b,e);
        if(type != tcounter)
            throw Except(this,Except::bad_type);
        value = 0;
        if(e - b >= (int32_t)length)
        {
            size_t shift;
            if(length > sizeof(value))
                shift = length - sizeof(value);
            else
                shift = 0;
            std::reverse_copy(b+shift,b+length,reinterpret_cast<std::uint8_t*>(&value));
            b = b+length;
        }
        else
        {
            throw Except(this,Except::proto_error);
        }
        _size += length;
        return b;
    }
    
    void Counter::write(std::vector<std::uint8_t>& d) const
    {
        Primitive::write(d);
        std::reverse_copy(reinterpret_cast<const std::uint8_t*>(&value),reinterpret_cast<const std::uint8_t*>(&value)+length.getValue(),std::inserter(d,d.end()));
    }


    Gauge::Gauge(std::uint32_t val)
    {
        type = tgauge;
        if(val <= 0xff)
            length = 1;
        else if(val <= 0xffff)
            length = 2;
        else if(val <= 0xffffff)
            length = 3;
        else
            length = 4;
        value = val;
        _size = sizeof(type) + length.getSize() + length;
    }

    std::vector<std::uint8_t>::const_iterator Gauge::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        b = Primitive::read(b,e);
        if(type != tgauge)
            throw Except(this,Except::bad_type);
        value = 0;
        if(e - b >= (int32_t)length)
        {
            size_t shift;
            if(length > sizeof(value))
                shift = length - sizeof(value);
            else
                shift = 0;
            std::reverse_copy(b+shift,b+length,reinterpret_cast<std::uint8_t*>(&value));
            b = b+length;
        }
        else
        {
            throw Except(this,Except::proto_error);
        }
        _size += length;
        return b;
    }
    
    void Gauge::write(std::vector<std::uint8_t>& d) const
    {
        Primitive::write(d);
        std::reverse_copy(reinterpret_cast<const std::uint8_t*>(&value),reinterpret_cast<const std::uint8_t*>(&value)+length.getValue(),std::inserter(d,d.end()));
    }

    TimeTicks::TimeTicks(std::uint32_t v)
    {
        type = ttime_ticks;
        length = 3;
        value = v;
        _size = sizeof(type) + length.getSize() + length;
    }

    std::vector<std::uint8_t>::const_iterator TimeTicks::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        b = Primitive::read(b,e);
        if(type != ttime_ticks)
            throw Except(this,Except::bad_type);
        value = 0;
        if(e - b >= (int)length && length <= sizeof(value))
        {
            std::reverse_copy(b,b+length,reinterpret_cast<std::uint8_t*>(&value));
            b = b+length;
        }
        _size += length;
        return b;
    }
    
    void TimeTicks::write(std::vector<std::uint8_t>& d) const
    {
        Primitive::write(d);
        std::reverse_copy(reinterpret_cast<const std::uint8_t*>(&value),reinterpret_cast<const std::uint8_t*>(&value)+length.getValue(),std::inserter(d,d.end()));
    }

    int16_t TimeTicks::days() const
    {
        return (value / (100 * 60 * 60 * 24));
    }

    int16_t TimeTicks::hours() const
    {
        return (value / (100 * 60 * 60)) % 24;
    }

    int16_t TimeTicks::minutes() const
    {
        return (value / (100 * 60)) % 60;
    }

    int16_t TimeTicks::seconds() const
    {
        return (value / 100) % 60;
    }
        
    int16_t TimeTicks::miliseconds() const
    {
        return value % 100;
    }

    OctetString::OctetString(const char* val)
    {
        setValue(val);
    }

    OctetString::OctetString(const std::string& val)
    {
        setValue(val);
    }

    std::vector<std::uint8_t>::const_iterator OctetString::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        b = Primitive::read(b,e);
        if(type != tocted_string)
            throw Except(this,Except::bad_type);
        if(e - b >= (int)length)
        {
            value.clear();
            std::copy(b,b+length,std::inserter(value,value.end()));
            b = b+length;
        }
        else
            throw Except(this,Except::proto_error);
        _size += length;
        return b;
    }
    
    void OctetString::write(std::vector<std::uint8_t>& d) const
    {
        Primitive::write(d);
        std::copy(value.begin(),value.end(),std::inserter(d,d.end()));
    }

    std::string OctetString::getValue() const
    {
        return value;
    }

    void OctetString::setValue(const std::string& str)
    {
        type = tocted_string;
        value = str;
        length = value.size();
        _size = 2 + length;
    }

    std::vector<std::uint8_t>::const_iterator Unknow::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        b = Primitive::read(b,e);
        _size += length;
        return (b + length);
    }

    void Unknow::write(std::vector<std::uint8_t>& d) const
    {
        Primitive::write(d);
        d.insert(d.end(),length.getValue(),0);
    }

    std::vector<std::uint8_t>::const_iterator Complex::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        return Middle::read(b,e);
    }
    
    void Complex::write(std::vector<std::uint8_t>& d) const
    {
        Middle::write(d);
    }
    
    Oid::Oid(const std::uint32_t *oid, size_t n)
    {
        type = tobject_identifier;
        value.push_back(0x2b);
        length = length + value.back().getSize();
        for(size_t i = 2;i < n;i++)
        {
            value.push_back(oid[i]);
            length = length + value.back().getSize();
        }
        _size = 2 + length;
    }
    
    Oid::Oid(const std::string& oid)
    {
        type = tobject_identifier;
        std::uint32_t v,v1,v3; char z;
        std::istringstream s(oid);
        s >> v1 >> z >> v3 >> z;
        if(v1 != 1 || v3 != 3)
            throw Except(this,Except::bad_oid);
        value.push_back(0x2b);
        length = length + value.back().getSize();
        while(true)
        {
            if(s >> v)
            {
                value.push_back(v);
                length = length + value.back().getSize();
            }
            if(s >> z)
            {
                if(z != '.')
                    throw Except(this,Except::bad_oid);
            }
            else
                break;
        }
        _size = 2 + length;
    }

    std::vector<std::uint8_t>::const_iterator Oid::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        b = Primitive::read(b,e);
        if(type != tobject_identifier)
            throw Except(this,Except::bad_type);
        size_t l=length;
        MultibyteValue v;
        value.clear();
        while(l > 0)
        {
            b = v.read(b,e);
            l -= v.getSize();
            _size += v.getSize();
            value.push_back(v);
        }
        return b;
    }
    
    void Oid::write(std::vector<std::uint8_t>& d) const
    {
        Primitive::write(d);
        for(std::vector<MultibyteValue>::const_iterator i = value.begin();i != value.end();i++)
        {
            i->write(d);
        }
    }
    
    std::uint32_t Oid::getBack(size_t n) const
    {
        n = n + 1;
        assert(value.size() >= n);
        return value.at(value.size() - n).getValue();
    }
    
    const std::uint32_t Oid::operator[](size_t n) const
    {
            assert(value.size() > n);
            return value.at(n).getValue(); 
    }

    std::string Oid::asString() const
    {
        std::ostringstream str;
        for(std::vector<MultibyteValue>::const_iterator i = value.begin();i != value.end();i++)
        {
            str << i->getValue();
            if(i + 1 != value.end())
                str << '.';
        }
        return str.str();
    }

    Oid Oid::operator+(std::uint32_t v) const
    {
        Oid tmp(*this);
        tmp.length = tmp.length + tmp.value.back().getSize();
        tmp.value.push_back(v);
        tmp._size = 2 + tmp.length;
        return tmp;
    }

    bool operator==(const Oid& oid1,const Oid& oid2)
    {
        if(oid1.getType() == oid2.getType() && oid1.getValueSize() == oid2.getValueSize())
        {
            for(size_t i = 0;i < oid1.getValueSize();i++)
            {
                if(oid1[i] != oid2[i])
                {
                    return false;
                }
            }
            return true;
        }
        else
            return false;
    }
    
    bool operator!=(const Oid& oid1,const Oid& oid2)
    {
        return !(oid1 == oid2);
    }
    
    bool operator<(const Oid& oid1,const Oid& oid2)
    {
        size_t len;
        if(oid1.getValueSize() < oid2.getValueSize())
        {
            len = oid1.getValueSize();
        }
        else
        {
            len = oid2.getValueSize();
        }
        for(size_t i = 0;i < len;i++)
        {
            if(oid1[i] != oid2[i])
            {
                if(oid1[i] > oid2[i])
                {
                    return false;
                }
                return true;
            }
        }
        if(oid1.getValueSize() < oid2.getValueSize())
            return true;
        else
            return false;
        
    }
    
    Varbind::Varbind(const Varbind& vb)
    {
        copy(vb);
    }

    Varbind::Varbind(const Oid& _oid,const Integer& _int) : integer(_int),value(&integer)
    {
        type = sequence;
        length = _oid.getSize() + integer.getSize();
        oid = _oid;
        _size = 1 + length.getSize() + length;
    }

    Varbind::Varbind(const Oid& _oid,const Counter& ct) : counter(ct),value(&counter)
    {
        type = sequence;
        length = _oid.getSize() + counter.getSize();
        oid = _oid;
        _size = 1 + length.getSize() + length;
    }

    Varbind::Varbind(const Oid& _oid,const Gauge& ga) : gauge(ga),value(&gauge)
    {
        type = sequence;
        length = _oid.getSize() + counter.getSize();
        oid = _oid;
        _size = 1 + length.getSize() + length;
    }

    Varbind::Varbind(const Oid& _oid,const TimeTicks& tt) : time_ticks(tt),value(&time_ticks)
    {
        type = sequence;
        length = _oid.getSize() + time_ticks.getSize();
        oid = _oid;
        _size = 1 + length.getSize() + length;
    }

    Varbind::Varbind(const Oid& _oid,const OctetString& os) : octet_string(os),value(&octet_string)
    {
        type = sequence;
        length = _oid.getSize() + os.getSize();
        oid = _oid;
        _size = 1 + length.getSize() + length;
    }
    
    Varbind::Varbind(const Oid& _oid,const Oid& _oidv) : oidv(_oidv),value(&oidv)
    {
        type = sequence;
        length = _oid.getSize() + oidv.getSize();
        oid = _oid;
        _size = 1 + length.getSize() + length;
    }
    
    Varbind::Varbind(const Oid& _oid) : value(&null)
    {
        type = sequence;
        length = _oid.getSize() + null.getSize();
        oid = _oid;
        _size = 1 + length.getSize() + length;
    }

    std::vector<std::uint8_t>::const_iterator Varbind::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        b = Complex::read(b,e);
        if(type != sequence)
            throw Except(this,Except::bad_type);
        b = oid.read(b,e);
        _size += oid.getSize();
        if(*b == Primitive::ttime_ticks)
        {
            value = &time_ticks;
        }
        else if(*b == Primitive::tinteger)
        {
            value = &integer;
        }
        else if(*b == Primitive::tcounter)
        {
            value = &counter;
        }
        else if(*b == Primitive::tgauge)
        {
            value = &gauge;
        }
        else if(*b == Primitive::tocted_string)
        {
            value = &octet_string;
        }
        else if(*b == Primitive::tobject_identifier)
        {
            value = &oidv;
        }
        else if(*b == Primitive::tnull)
        {
            value = &null;
        }
        else
        {
            value = &unknow;
        }
        b = value->read(b,e);
        _size += value->getSize();
        return b;
    }
    
    void Varbind::write(std::vector<std::uint8_t>& d) const
    {
        Complex::write(d);
        oid.write(d);
        value->write(d);
    }
    
    std::uint8_t Varbind::getValueType() const
    {
        return value->getType();
    }
        
    const Primitive& Varbind::getValue() const
    {
        return *value;
    }

    Varbind& Varbind::operator=(const Varbind& vb)
    {
        copy(vb);
        return *this;
    }

    void Varbind::copy(const Varbind& vb)
    {
        type = vb.type;
        length = vb.length;
        oid = vb.oid;
        if(vb.getValueType() == Primitive::tinteger)
        {
            integer = vb.integer;
            value = &integer;
        }
        else if(vb.getValueType() == Primitive::tcounter)
        {
            counter = vb.counter;
            value = &counter;
        }
        else if(vb.getValueType() == Primitive::tgauge)
        {
            gauge = vb.gauge;
            value = &gauge;
        }
        else if(vb.getValueType() == Primitive::ttime_ticks)
        {
            time_ticks = vb.time_ticks;
            value = &time_ticks;
        }
        else if(vb.getValueType() == Primitive::tocted_string)
        {
            octet_string = vb.octet_string;
            value = &octet_string;
        }
        else if(vb.getValueType() == Primitive::tobject_identifier)
        {
            oidv = vb.oidv;
            value = &oidv;
        }
        else if(vb.getValueType() == Primitive::tnull)
        {
            null = vb.null;
            value = &null;
        }
        else
        {
            unknow = vb.unknow;
            value = &unknow;
        }
        _size = vb._size;
    }

    Varbinds::Varbinds()
    {
        type = sequence;
        length = 0;
        _size = 0;
    }

    void Varbinds::addVarbind(const Varbind& vb)
    {
        value.push_back(vb);
        length = length + vb.getSize();
        _size = 1 + length.getSize();
        for(std::list<Varbind>::const_iterator i = value.begin();i != value.end();i++)
        {
            _size += i->getSize();
        }
    }

    std::vector<std::uint8_t>::const_iterator Varbinds::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        b = Complex::read(b,e);
        if(type != sequence)
            throw Except(this,Except::bad_type);

        size_t len_tmp=0;
        if(length > 0)
        {
            Varbind varbind;
            while(len_tmp < length)
            {
                b = varbind.read(b,e);
                _size += varbind.getSize();
                len_tmp += varbind.getSize();
                value.push_back(varbind);
            }
        }
        return b;
    }

    void Varbinds::write(std::vector<std::uint8_t>& d) const
    {
        Complex::write(d);
        for(std::list<Varbind>::const_iterator i = value.begin();i != value.end();i++)
        {
            i->write(d);
        }
    }
    
    PDU::PDU(Complex::Type t,const Integer& req_id,const Integer& e,const Integer& e_id,const Varbinds& vs)
    {
        type = t;
        length = req_id.getSize() + e.getSize() + e_id.getSize() + vs.getSize();
        request_id = req_id;
        error = e;
        error_id = e_id;
        varbinds = vs;
        _size = 1 + length.getSize() + length;
    }
    
    
    std::vector<std::uint8_t>::const_iterator PDU::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        b = Complex::read(b,e);
        if(type != get_request && type != get_next_request && type != get_response && type != set_request)
            throw Except(this,Except::bad_type);
        b = request_id.read(b,e);
        _size += request_id.getSize();
        b = error.read(b,e);
        _size += error.getSize();
        b = error_id.read(b,e);
        _size += error_id.getSize();
        b = varbinds.read(b,e);
        _size += varbinds.getSize();
        return b;
    }

    void PDU::write(std::vector<std::uint8_t>& d) const
    {
        Complex::write(d);
        request_id.write(d);
        error.write(d);
        error_id.write(d);
        varbinds.write(d);
    }

    Message::Message(const Integer& ver,const OctetString& comm)
    {
        set(ver,comm);
    }
    
    void Message::set(const Integer& ver,const OctetString& comm)
    {
        type = sequence;
        version = ver;
        community = comm;
        length = version.getSize() + community.getSize();
        _size = 1 + length.getSize() + length;
    }

    void Message::setPDU(const PDU& _pdu)
    {
        pdu = _pdu;
        length = version.getSize() + community.getSize();
        length = length + pdu.getSize();
        _size = 1 + length.getSize() + length;
        _size += pdu.getSize();
    }

    std::vector<std::uint8_t>::const_iterator Message::read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e)
    {
        b = Complex::read(b,e);
        if(type != sequence)
            throw Except(this,Except::bad_type);
        b = version.read(b,e);
        _size += version.getSize();
        b = community.read(b,e);
        _size += community.getSize();
        b = pdu.read(b,e);
        _size += pdu.getSize();
        return b;
    }

    void Message::write(std::vector<std::uint8_t>& d) const
    {
		d.clear();
        Complex::write(d);
        version.write(d);
        community.write(d);
        pdu.write(d);
    }
}

