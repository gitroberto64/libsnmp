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
#pragma once
#include <vector>
#include <list>
#include <string>
#include <cstdint>

namespace snmp
{
    class Abstract;
    class Except : public std::exception
    {
    public:
        enum Code { bad_type, proto_error, bad_oid };
        Except(const Abstract* _id,Code code) throw();
        ~Except() throw() {}
        const char* what() const throw();
    private:
        std::string str;
        static const std::string message[3];
    };


    class Abstract
    {
    public:
        Abstract() : _size(0) {}
        size_t getSize() const { return _size; }
    protected:
        virtual std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e) = 0;
        virtual void write(std::vector<std::uint8_t>& d) const = 0;
        size_t _size;
    };

    class MultibyteLen : public Abstract
    {
    public:
        MultibyteLen() : value(0) {}
        MultibyteLen(std::uint32_t val);
        std::uint32_t getValue() const { return value; }
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
        operator std::uint32_t() const { return value; }
    protected:
        std::uint32_t value;
    };

    class MultibyteValue : public Abstract
    {
    public:
        MultibyteValue() : value(0) {}
        MultibyteValue(std::uint64_t val);
        std::uint64_t getValue() const { return value; }
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
    protected:
        std::uint64_t value;
    };

    class Middle : public Abstract
    {
    public:
        std::uint8_t getType() const { return type; }
        const MultibyteLen& getLength() const { return length; }
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
    protected:
        std::uint8_t type;
        MultibyteLen length;
    };

    class Primitive : public Middle
    {
    public:
        enum Type { tinteger = 0x02, tocted_string = 0x04, tnull = 0x05, tobject_identifier = 0x06, tcounter=0x41, tgauge=0x42, ttime_ticks = 0x43 };
        Primitive() { type = 0; length = 0; }
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
    protected:
    };
    
    class Null : public Primitive
    {
    public:
        Null() { type = tnull; length = 0; _size = 2; }
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
    protected:
    };

    class Integer : public Primitive
    {
    public:
        Integer() {}
        Integer(int32_t val);
        int32_t getValue() const { return value; }
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
        operator int32_t() const { return value; }
    protected:
        int32_t value;
    };

    class Counter : public Primitive
    {
    public:
        Counter() {}
        Counter(std::uint32_t val);
        std::uint32_t getValue() const { return value; }
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
        operator std::uint32_t() const { return value; }
    protected:
        std::uint32_t value;
    };

    class Gauge : public Primitive
    {
    public:
        Gauge() {}
        Gauge(std::uint32_t val);
        std::uint32_t getValue() const { return value; }
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
        operator std::uint32_t() const { return value; }
    protected:
        std::uint32_t value;
    };

    class TimeTicks : public Primitive
    {
    public:
        TimeTicks() {}
        TimeTicks(std::uint32_t v);
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
        std::uint32_t getValue() const { return value; }
        int16_t days() const;
        int16_t hours() const;
        int16_t minutes() const;
        int16_t seconds() const;
        int16_t miliseconds() const;
    protected:
        std::uint32_t value;
    };
    

    class OctetString : public Primitive
    {
    public:
        OctetString() {}
        OctetString(const char* val);
        OctetString(const std::string& val);
        std::string getValue() const;
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
        operator const char*() const { return value.c_str(); }
    protected:
        std::string value;
    private:
        void setValue(const std::string& str);
    };

    class Unknow : public Primitive
    {
    public:
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
    };

    class Complex : public Middle
    {
    public:
        enum Type { sequence = 0x30, get_request = 0xa0, get_next_request = 0xa1, get_response = 0xa2, set_request = 0xa3, trap = 0xa4 };
        Complex() { type = 0; length = 0; }
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
    protected:
    };

    class Oid : public Primitive
    {
    public:
        Oid() {}
        Oid(const std::uint32_t *oid, size_t n);
        Oid(const std::string& oid);
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
        std::uint32_t getBack(size_t n = 0) const;
        const std::uint32_t operator[](size_t n) const;
        std::string asString() const;
        size_t getValueSize() const { return value.size(); }
        Oid operator+(std::uint32_t v) const;
    private:
        std::vector<MultibyteValue> value;
    };

    bool operator==(const Oid& oid1,const Oid& oid2);
    bool operator!=(const Oid& oid1,const Oid& oid2);
    bool operator<(const Oid& oid1,const Oid& oid2);

    class Varbind : public Complex
    {
    public:
        Varbind() : value(&null) {}
        Varbind(const Varbind& vb);
        Varbind(const Oid& _oid,const Integer& _int);
        Varbind(const Oid& _oid,const Counter& ct);
        Varbind(const Oid& _oid,const Gauge& ga);
        Varbind(const Oid& _oid,const TimeTicks& tt);
        Varbind(const Oid& _oid,const OctetString& os);
        Varbind(const Oid& _oid,const Oid& _oidv);
        Varbind(const Oid& _oid);
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
        const Oid& getOid() const { return oid; }
        std::uint8_t getValueType() const;
        const Primitive& getValue() const;
        const Integer& getInteger() const { return integer; }
        const Counter& getCounter() const { return counter; }
        const Gauge& getGauge() const { return gauge; }
        const TimeTicks& getTimeTicks() const { return time_ticks; }
        const OctetString& getOctetString() const { return octet_string; }
        const Oid& getOidValue() const { return oidv; }
        const Unknow& getUnknow() const { return unknow; }
        Varbind& operator=(const Varbind& vb);
    protected:
        void copy(const Varbind& vb);
        Oid oid;
        Integer integer;
        Counter counter;
        Gauge gauge;
        TimeTicks time_ticks;
        OctetString octet_string;
        Oid oidv;
        Null null;
        Unknow unknow;
        Primitive* value;
    };

    class Varbinds : public Complex
    {
    public:
        Varbinds();
        void addVarbind(const Varbind& vb);
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
        const std::list<Varbind>& getValue() const { return value; }
    protected:
        std::list<Varbind> value;
    };

    class PDU : public Complex
    {
    public:
        enum Error { noError=0, tooLarge=1, noSuchName=2, noType=3, readOnly=4, generalError=5 };
        PDU() {}
        PDU(Complex::Type t,const Integer& req_id,const Integer& e,const Integer& e_id,const Varbinds& vs);
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
        const Integer& getRequestID() const { return request_id; }
        const Integer& getError() const { return error; }
        const Integer& getErrorID() const { return error_id; }
        const Varbinds& getVarbinds() const { return varbinds; }
    protected:
        Integer request_id;
        Integer error;
        Integer error_id;
        Varbinds varbinds;
    };

    enum Type { v1 = 0, v2c = 1, v3 = 2 };

    class Message : public Complex
    {
    public:
        Message() {}
        Message(const Integer& ver,const OctetString& comm);
        void set(const Integer& ver,const OctetString& comm);
        void setPDU(const PDU& _pdu);
        std::vector<std::uint8_t>::const_iterator read(std::vector<std::uint8_t>::const_iterator b,const std::vector<std::uint8_t>::const_iterator e);
        void write(std::vector<std::uint8_t>& d) const;
        const Integer& getVersion() const { return version; }
        const OctetString& getCommunity() const { return community; }
        const PDU& getPDU() const { return pdu; }
    protected:
        Integer version;
        OctetString community;
        PDU pdu;
    };
}

