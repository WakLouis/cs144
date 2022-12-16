const uint32_t &src_ip_addr = arp_msg.sender_ip_address;
const uint32_t &dst_ip_addr = arp_msg.target_ip_address;
const EthernetAddress &src_eth_addr = arp_msg.sender_ethernet_address;
const EthernetAddress &dst_eth_addr = arp_msg.target_ethernet_address;
// 如果是一个发给自己的 ARP 请求
bool is_valid_arp_request = arp_msg.opcode == ARPMessage::OPCODE_REQUEST && dst_ip_addr == _ip_address.ipv4_numeric();
bool is_valid_arp_response = arp_msg.opcode == ARPMessage::OPCODE_REPLY && dst_eth_addr == _ethernet_address;
if (is_valid_arp_request) {
    ARPMessage arp_reply;
    arp_reply.opcode = ARPMessage::OPCODE_REPLY;
    arp_reply.sender_ethernet_address = _ethernet_address;
    arp_reply.sender_ip_address = _ip_address.ipv4_numeric();
    arp_reply.target_ethernet_address = src_eth_addr;
    arp_reply.target_ip_address = src_ip_addr;

    EthernetFrame eth_frame;
    eth_frame.header() = {/* dst  */ src_eth_addr,
                          /* src  */ _ethernet_address,
                          /* type */ EthernetHeader::TYPE_ARP};
    eth_frame.payload() = arp_reply.serialize();
    _frames_out.push(eth_frame);
}
// 否则是一个 ARP 响应包
//! NOTE: 我们可以同时从 ARP 请求和响应包中获取到新的 ARP 表项
if (is_valid_arp_request || is_valid_arp_response) {
    _arp_Table[src_ip_addr] = {src_eth_addr, arp_entry_Timeout};
    // 将对应数据从原先等待队列里删除
    for (auto iter = _waiting_arp_ip_Datagram.begin(); iter != _waiting_arp_ip_Datagram.end();
         /* nop */) {
        if (iter->first.ipv4_numeric() == src_ip_addr) {
            send_datagram(iter->second, iter->first);
            iter = _waiting_arp_ip_Datagram.erase(iter);
        } else
            ++iter;
    }
    _waiting_arp_response.erase(src_ip_addr);
}