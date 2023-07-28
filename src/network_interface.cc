#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "../util/socket.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
        : ethernet_address_(ethernet_address), ip_address_(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(ethernet_address_) << " and IP address "
         << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    uint32_t const next_hop_ipv4_num = next_hop.ipv4_numeric();
    auto iter1 = arp_table_.find(next_hop_ipv4_num);
    if (iter1 != arp_table_.end() && iter1->second.ttl > 0) { /* 缓存命中 */
        EthernetFrame ef;
        ef.header.type = EthernetHeader::TYPE_IPv4;
        ef.header.src = NetworkInterface::ethernet_address_;
        ef.header.dst = iter1->second.eth_addr;
        ef.payload = serialize(dgram);
        outbound_frames_.push(ef);
        return;
    }

    // 发送arp request
    auto iter2 = arp_requests_lifetime_.find(next_hop_ipv4_num);
    if (iter2 != arp_requests_lifetime_.end() && iter2->second > 0) {/* arp请求已经发送，但是还没有到5seconds，等待接收arp响应 */
        arp_requests_waiting_list_.push_back(std::pair<Address, InternetDatagram>(next_hop, dgram));
        return;
    }

    EthernetFrame ef;
    ef.header.type = EthernetHeader::TYPE_ARP;
    ef.header.src = NetworkInterface::ethernet_address_;
    ef.header.dst = ETHERNET_BROADCAST;

    ARPMessage arp_msg;
    arp_msg.opcode = ARPMessage::OPCODE_REQUEST;
    arp_msg.sender_ip_address = NetworkInterface::ip_address_.ipv4_numeric();
    arp_msg.sender_ethernet_address = NetworkInterface::ethernet_address_;
    arp_msg.target_ip_address = next_hop_ipv4_num;
    arp_msg.target_ethernet_address = {0, 0, 0, 0, 0, 0};

    ef.payload = serialize(arp_msg);
    outbound_frames_.push(ef);

    // 缓存arp 请求时间
    arp_requests_waiting_list_.emplace_back(next_hop, dgram);
    arp_requests_lifetime_.emplace(std::pair<uint32_t, size_t>(next_hop_ipv4_num, ARP_REQUEST_DEFAULT_TTL));
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // 不接收非广播地址或目标mac为自身mac地址的的mac帧
    if (frame.header.dst != NetworkInterface::ethernet_address_ || frame.header.dst != ETHERNET_BROADCAST) {
        return nullopt;
    }

    if (frame.header.type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram ip_dgram;
        auto ok = parse(ip_dgram, frame.payload);
        if (ok) {
            return ip_dgram;
        }

        return nullopt;
    }

    if (frame.header.type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp_msg;
        auto ok = parse(arp_msg, frame.payload);
        if (ok) {
            const bool is_arp_request =
                    arp_msg.opcode == ARPMessage::OPCODE_REQUEST &&
                    arp_msg.target_ip_address == NetworkInterface::ip_address_.ipv4_numeric();

            if (is_arp_request) {
                ARPMessage arp_reply_msg;
                arp_reply_msg.opcode = ARPMessage::OPCODE_REPLY;
                arp_reply_msg.sender_ip_address = ip_address_.ipv4_numeric();
                arp_reply_msg.sender_ethernet_address = NetworkInterface::ethernet_address_;
                arp_reply_msg.target_ip_address = arp_msg.sender_ip_address;
                arp_reply_msg.target_ethernet_address = arp_msg.sender_ethernet_address;

                EthernetFrame arp_reply_eth_frame;
                arp_reply_eth_frame.header.src = NetworkInterface::ethernet_address_;
                arp_reply_eth_frame.header.dst = arp_msg.sender_ethernet_address;
                arp_reply_eth_frame.header.type = EthernetHeader::TYPE_ARP;
                arp_reply_eth_frame.payload = serialize(arp_reply_msg);
                outbound_frames_.push(arp_reply_eth_frame);
            }

            const bool is_arp_response =
                    arp_msg.opcode == ARPMessage::OPCODE_REPLY && arp_msg.target_ethernet_address == ethernet_address_;

            // we can get arp info from either ARP request or ARP reply
            if (is_arp_request || is_arp_response) {
                arp_table_.emplace(std::pair(arp_msg.sender_ip_address,
                                             arp_t{arp_msg.sender_ethernet_address, ARP_DEFAULT_TTL}));

                // delete arp datagrams waiting list
                for (auto iter = arp_requests_waiting_list_.begin(); iter != arp_requests_waiting_list_.end();) {
                    const auto &[ipv4_addr, datagram] = *iter;
                    if (ipv4_addr.ipv4_numeric() == arp_msg.sender_ip_address) {
                        send_datagram(datagram, ipv4_addr);
                        iter = arp_requests_waiting_list_.erase(iter);
                    } else {
                        iter++;
                    }
                }
                arp_requests_lifetime_.erase(arp_msg.sender_ip_address);
            }
        }
    }

    return nullopt;
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    // clear arp cache when arp cache is out of date
    for (auto iter = arp_table_.begin(); iter != arp_table_.end();) {
        if (iter->second.ttl <= ms_since_last_tick) {
            iter = arp_table_.erase(iter);
        } else {
            iter->second.ttl -= ms_since_last_tick;
            iter++;
        }
    }

    // resend the arp request when it is time to resend. (over 5 seconds)
    for (auto &[ipv4_addr, arp_ttl]: arp_requests_lifetime_) {
        if (arp_ttl <= ms_since_last_tick) {
            arp_ttl = ARP_REQUEST_DEFAULT_TTL;
            // resend mac frame
            EthernetFrame ef;
            ef.header.dst = ETHERNET_BROADCAST;
            ef.header.src = NetworkInterface::ethernet_address_;
            ef.header.type = EthernetHeader::TYPE_ARP;

            ARPMessage arp_msg;
            arp_msg.opcode = ARPMessage::OPCODE_REQUEST;
            arp_msg.sender_ip_address = NetworkInterface::ip_address_.ipv4_numeric();
            arp_msg.sender_ethernet_address = NetworkInterface::ethernet_address_;
            arp_msg.target_ip_address = ipv4_addr;
            arp_msg.target_ethernet_address = {0, 0, 0, 0, 0, 0};

            ef.payload = serialize(arp_msg);;
            outbound_frames_.push(ef);
            continue;
        }
        arp_ttl -= ms_since_last_tick;
    }
}

optional<EthernetFrame> NetworkInterface::maybe_send() {
    if (!outbound_frames_.empty()) {
        auto res = outbound_frames_.front();
        outbound_frames_.pop();
        return res;
    }
    return nullopt;
}
