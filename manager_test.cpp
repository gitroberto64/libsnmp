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
#include <boost/asio.hpp>
#include <vector>
#include "snmp.h"


using boost::asio::ip::udp;

enum { max_length = 1024 };

void recv(const snmp::Message& m)
{
    //std::cout << "MessageType=" << (unsigned)m.getType() << "h,Len=" << std::dec <<  m.getLength() << ",Version=" << m.getVersion().getValue()
    //    << ",Community=" << m.getCommunity().getValue() << ",RequestID=" << std::hex << m.getPDU().getRequestID().getValue() << std::dec << ",Error=" << m.getPDU().getError().getValue() << ",ErrorID=" << m.getPDU().getErrorID().getValue()<< std::endl;
    if(m.getPDU().getError() != snmp::PDU::noError)
    {
        std::cout << "Error = " << m.getPDU().getError() << std::endl;
    }
    else
    {
        const std::list<snmp::Varbind>& l = m.getPDU().getVarbinds().getValue();
        for(std::list<snmp::Varbind>::const_iterator i = l.begin();i != l.end();i++)
        {
            const snmp::Varbind& rvar = *i;
            std::cout << "OID=" << rvar.getOid().asString();
            if(rvar.getValueType() == snmp::Primitive::ttime_ticks)
            {
                snmp::TimeTicks tt = rvar.getTimeTicks();
                std::cout << ": TimeTicks=" << tt.days() << ':' << tt.hours() << ':' << tt.minutes() << ':' << tt.seconds() << '.' << tt.miliseconds() << std::endl;
            }
            else if(rvar.getValueType() == snmp::Primitive::tocted_string)
            {
                std::cout << ": OctetString="<< rvar.getOctetString().getValue() << std::endl;
            }
            else if(rvar.getValueType() == snmp::Primitive::tobject_identifier)
            {
                std::cout << ": OID=" << rvar.getOidValue().asString() << std::endl;
            }
            else if(rvar.getValueType() == snmp::Primitive::tinteger)
            {
                std::cout << ": Integer=" << std::dec << rvar.getInteger().getValue() << std::endl;
            }
            else if(rvar.getValueType() == snmp::Primitive::tcounter)
            {
                std::cout << ": Counter=" << std::dec << rvar.getCounter().getValue() << std::endl;
            }
            else
            {
                std::cout << ": Unknow,Type=" << std::hex << (unsigned)rvar.getUnknow().getType() << "h" << std::endl;
            }
        }
    }
}

int main(int argc,char* argv[])
{

    try
    {
        boost::asio::io_service io_service;

        udp::socket s(io_service);
        udp::resolver resolver(io_service);
        udp::resolver::query query(udp::v4(), argv[1], argv[2]);
        udp::resolver::iterator iterator = resolver.resolve(query);
        s.connect(*iterator);
        
        using namespace std;

        snmp::Message send_m(snmp::v1,"public");

        std::uint32_t oid1[9] = {1,3,6,1,2,1,1,3,0};
        std::uint32_t oid2[9] = {1,3,6,1,2,1,1,1,0};
        snmp::Varbinds vsystem;
        vsystem.addVarbind(snmp::Varbind(snmp::Oid(oid1,sizeof(oid1) / sizeof(std::uint32_t))));
        vsystem.addVarbind(snmp::Varbind(snmp::Oid(oid2,sizeof(oid2) / sizeof(std::uint32_t))));
        //vsystem.addVarbind(snmp::Varbind(snmp::Oid(oid3,sizeof(oid3) / sizeof(std::uint32_t))));
        //vsystem.addVarbind(snmp::Varbind(snmp::Oid(oid4,sizeof(oid4) / sizeof(std::uint32_t))));
        //vsystem.addVarbind(snmp::Varbind(snmp::Oid(oid5,sizeof(oid5) / sizeof(std::uint32_t))));
        //vsystem.addVarbind(snmp::Varbind(snmp::Oid(oid6,sizeof(oid6) / sizeof(std::uint32_t))));

        
        snmp::PDU pdu_system(snmp::Complex::get_request,0x11223344,0,0,vsystem);
        send_m.setPDU(pdu_system);
        std::vector<std::uint8_t> to_send;
        send_m.write(to_send);
        s.send(boost::asio::buffer(to_send));


        std::vector<unsigned char> reply(max_length);
//        udp::endpoint sender_endpoint;
        size_t reply_length = s.receive(boost::asio::buffer(reply));
        snmp::Message m;
        m.read(reply.begin(),reply.begin()+reply_length);
        recv(m);
        return 0;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}

