#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <queue>
#include <map>

class TCPSender {
    Wrap32 isn_;
    uint64_t initial_RTO_ms_; // 初始给的RTO
    uint64_t rto_{}; // 重传超时时间
    size_t timer_{}; // 存储时间

    bool is_setup_syn_{false};
    bool is_setup_fin_{false};

    uint64_t outstanding_seqno_{0};
    std::map<uint64_t/* absolute seqno */, TCPSenderMessage> outstanding_segments_{};// 已发送但未收到ack的数据段payload，空payload不保存。
    std::queue<TCPSenderMessage> segments_out_{}; // fifo队列

    uint64_t next_absolute_seqno_{0}; // 下一个绝对序列号
    uint64_t number_of_retransmissions_{0};// 重传次数
    uint16_t window_size_{1};

public:
    /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
    TCPSender(uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn);

    /* Push bytes from the outbound stream */
    void push(Reader &outbound_stream);

    /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
    std::optional<TCPSenderMessage> maybe_send();

    /* Generate an empty TCPSenderMessage */
    TCPSenderMessage send_empty_message() const;

    /* Receive an act on a TCPReceiverMessage from the peer's receiver */
    void receive(const TCPReceiverMessage &msg);

    /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
    void tick(uint64_t ms_since_last_tick);

    /* Accessors for use in testing */
    uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
    uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
