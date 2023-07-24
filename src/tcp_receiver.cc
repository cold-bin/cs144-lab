#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive(TCPSenderMessage message, Reassembler &reassembler, Writer &inbound_stream) {
    // Your code here.
    // 如果没有设置isn，就设置isn
    if (!is_isn_set) {
        if (!message.SYN) {
            return;
        }
        isn = message.seqno;
        is_isn_set = true;
    }

    // seqno 转化为 absolute seqno
    uint64_t const absolute_seqno = message.seqno.unwrap(isn, inbound_stream.bytes_pushed() + 1);
    // 开始写入reassembler
//    uint64_t stream_idx = 0;
//    if (message.SYN) {/* 需要减去syn报文暂用的一个序列号 */
//        stream_idx = absolute_seqno - 1;
//    } else {
//        stream_idx = absolute_seqno;
//    }
    uint64_t const stream_idx = absolute_seqno - 1 + message.SYN;
    reassembler.insert(stream_idx, message.payload.release(), message.FIN, inbound_stream);
}

TCPReceiverMessage TCPReceiver::send(const Writer &inbound_stream) const {
    // Your code here.
    struct TCPReceiverMessage ans;
    ans.window_size =
            inbound_stream.available_capacity() > UINT16_MAX ? UINT16_MAX : inbound_stream.available_capacity();

    // 尚未收到初始序列号，ackno为空
    if (is_isn_set) {
//        uint64_t ackno_offset = 0;
//        ackno_offset = inbound_stream.bytes_pushed() + 1;
//        if (inbound_stream.is_closed()) {/*额外的fin*/
//            ackno_offset++;
//        }
//        ans.ackno = isn + ackno_offset;
        ans.ackno = isn + inbound_stream.bytes_pushed() + 1 + inbound_stream.is_closed();
    }

    return ans;
}
