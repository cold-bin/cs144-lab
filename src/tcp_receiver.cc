#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive(TCPSenderMessage message, Reassembler &reassembler, Writer &inbound_stream) {
    // Your code here.
    cur_message = message;
    // 如果没有设置isn，就设置isn
    if (!is_isn_set) {
        isn = message.SYN ? message.seqno : message.seqno - 1;
        is_isn_set = true;
    }

    // seqno 转化为 absolute seqno
    uint64_t const absolute_seqno = message.seqno.unwrap(isn, reassembler.getUnassembledIdx());

    // fin时，需要重置isn
    if (message.FIN) {
        isn = Wrap32{0};
        is_isn_set = false;
    }

    // 开始写入reassembler
    reassembler.insert(absolute_seqno, message.payload.operator std::string &(), message.FIN, inbound_stream);
}

TCPReceiverMessage TCPReceiver::send(const Writer &inbound_stream) const {
    // Your code here.
    struct TCPReceiverMessage ans;
    // 尚未收到初始序列号，ackno为空
    if (is_isn_set) {
        ans.ackno = cur_message.seqno + cur_message.sequence_length();
    }
    if (inbound_stream.is_closed()) {/*额外的fin*/
        ans.window_size++;
    }
    ans.window_size = inbound_stream.available_capacity();
    return ans;
}
