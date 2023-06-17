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

size_t num_inter(0);

std::uint32_t sysUpTime[] = {1,3,6,1,2,1,1,3,0};
std::uint32_t sysDescr[] = {1,3,6,1,2,1,1,1,0};

void recv(const snmp::Message& m,snmp::Message& sm)
{
    std::cout << "MessageType=" << (unsigned)m.getType() << "h,Len=" << std::dec <<  m.getLength() << ",Version=" << m.getVersion().getValue()
        << ",Community=" << m.getCommunity().getValue() << ",RequestID=" << std::hex << m.getPDU().getRequestID().getValue() << std::dec << ",Error=" << m.getPDU().getError().getValue() << ",ErrorID=" << m.getPDU().getErrorID().getValue()<< std::endl;
    if(m.getPDU().getError().getValue() > 0)
    {
    }
    else
    {
        snmp::Message send_m(snmp::v1,"public");
        snmp::Varbinds send_v;

        const std::list<snmp::Varbind>& l = m.getPDU().getVarbinds().getValue();
        for(std::list<snmp::Varbind>::const_iterator i = l.begin();i != l.end();i++)
        {
            const snmp::Varbind& rvar = *i;
            std::cout << "OID=" << rvar.getOid().asString() << std::endl;
            if(rvar.getValueType() == snmp::Primitive::tnull)
            {
                if(snmp::Oid(sysUpTime, sizeof(sysUpTime) / sizeof(std::uint32_t)) == rvar.getOid())
                    send_v.addVarbind(snmp::Varbind(rvar.getOid(),snmp::TimeTicks(11111)));
                else if(snmp::Oid(sysDescr, sizeof(sysDescr) / sizeof(std::uint32_t)) == rvar.getOid())
                    send_v.addVarbind(snmp::Varbind(rvar.getOid(),snmp::OctetString("Test agenta SNMP")));
                else
                {
                    send_v.addVarbind(snmp::Varbind(rvar.getOid()));
                    snmp::PDU send_pdu(snmp::Complex::get_response,snmp::Integer(m.getPDU().getRequestID().getValue()),2,std::distance(l.begin(),i),send_v);
                    send_m.setPDU(send_pdu);
                    sm = send_m;
                    return;
                }
            }
        }
        snmp::PDU send_pdu(snmp::Complex::get_response,snmp::Integer(m.getPDU().getRequestID().getValue()),0,0,send_v);
        send_m.setPDU(send_pdu);
        sm = send_m;
    }
}


int main(int argc,char* argv[])
{

    try
    {
        boost::asio::io_service io_service;

        udp::socket s(io_service, udp::endpoint(udp::v4(), 2001));

        
        udp::endpoint manager;
        std::vector<unsigned char> request(max_length);
        while(true)
        {
            size_t r_length = s.receive_from(boost::asio::buffer(request),manager);
            std::cout << "Manager : " << manager.address().to_string() << " Port: " <<  manager.port() << std::endl;
            for(size_t i =0;i < r_length;i++)
            {
                std::cout << std::hex << (unsigned)request[i] << ' ';
            }
            std::cout << std::endl;
            snmp::Message m,send_m;
            m.read(request.begin(),request.begin()+r_length);
            recv(m,send_m);
            std::vector<std::uint8_t> to_send;
            send_m.write(to_send);
            s.send_to(boost::asio::buffer(to_send),manager);
        }
        return 0;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}

