#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix, /* 目标ip */
                       const uint8_t prefix_length, /* 子网掩码 */
                       const optional<Address> next_hop, /* 如果不是直连路由器，那么依然需要next hop，否则直接走路由器网口出去就是目的地址了 */
                       const size_t interface_num /* 发送数据报的索引 */
) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/"
         << static_cast<int>( prefix_length ) << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)")
         << " on interface " << interface_num << "\n";
    // route rule不存在，新增
    routing_table_.emplace_back(route_t{route_prefix, prefix_length, next_hop, interface_num});
}

void Router::route() {
    // 遍历每一个网口
    for (auto &ani: interfaces_) {
        // 拿到每个网口的数据
        while (auto dgram = ani.maybe_receive()) {
            if (dgram) {
                auto ip_datagram = dgram.value();
                if (ip_datagram.header.ttl-- <= 0) continue;/*不转发ttl过期的ip数据报*/

                // 找到next hop & interface id
                int8_t max_prefix_length = -1;
                route_t ans_r{};
                for (route_t const &r: routing_table_) {
                    if (r.prefix_length==0){/*32位无符号数无法移位32位，另外处理*/
                        max_prefix_length = static_cast<int8_t>(r.prefix_length);
                        ans_r = r;
                        continue;
                    }
                    uint32_t mask = ~0U;
                    mask <<= static_cast<uint32_t>(32-r.prefix_length);

                    if ((r.route_prefix & mask) == (ip_datagram.header.dst & mask)) {/*前缀匹配*/
                        if (max_prefix_length < r.prefix_length) {/*取最长前缀*/
                            max_prefix_length = static_cast<int8_t>(r.prefix_length);
                            ans_r = r;
                        }
                    }
                }

                if (max_prefix_length >=0){
                    // 发送
                    ip_datagram.header.compute_checksum();/*重新计算校验和*/
                    AsyncNetworkInterface &out_interface = interface(ans_r.interface_id);
                    out_interface.send_datagram(ip_datagram, ans_r.next_hop.has_value() ? ans_r.next_hop.value(): Address::from_ipv4_numeric(ip_datagram.header.dst));
                }
            }
        }
    }
}
