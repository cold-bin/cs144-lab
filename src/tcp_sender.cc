#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender(uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn)
        : isn_(fixed_isn.value_or(Wrap32{random_device()()})), initial_RTO_ms_(initial_RTO_ms) {}

uint64_t TCPSender::sequence_numbers_in_flight() const {
    // Your code here.
    return outstanding_seqno_;
}

uint64_t TCPSender::consecutive_retransmissions() const {
    // Your code here.
    return number_of_retransmissions_;
}

optional<TCPSenderMessage> TCPSender::maybe_send() {
    // Your code here.
    if (!segments_out_.empty() && is_setup_syn_) {
        auto e = std::move(segments_out_.front());
        segments_out_.pop();
        return e;
    }
    return nullopt;
}

void TCPSender::push(Reader &outbound_stream) {
    // Your code here.

    // if window size is zero, you can assume the window size is one(just one byte),
    // in order to wait for the receiver
    if (window_size_ == 0) {
        window_size_ = 1;
    }

    // best effort to fit more TCPSenderMessage
    while (window_size_ > outstanding_seqno_) {
        TCPSenderMessage msg;

        // set seqno
        msg.seqno = isn_ + next_absolute_seqno_;

        // place syn if needed
        if (!is_setup_syn_) {
            is_setup_syn_ = msg.SYN = true;
        }

        // place payload
        const uint64_t payload_size
                = min(TCPConfig::MAX_PAYLOAD_SIZE, window_size_ - outstanding_seqno_ - msg.SYN);
        std::string payload = std::string(outbound_stream.peek()).substr(0, payload_size);
        outbound_stream.pop(payload_size);

        msg.payload = Buffer(std::move(payload));

        // place fin if these conditions satisfied:
        // 1. not set fin before;
        // 2. Reader has no data;
        // 3. after place syn and payload, the window can yet contain fin.
        if (!is_setup_fin_ && outbound_stream.is_finished() &&
            msg.payload.size() + outstanding_seqno_ + msg.SYN < window_size_) {
            is_setup_fin_ = msg.FIN = true;
        }

        if (outstanding_segments_.empty()) {
            rto_ = initial_RTO_ms_;
            timer_ = 0;
        }

        segments_out_.push(msg);

        outstanding_seqno_ += msg.sequence_length();
        outstanding_segments_.insert(std::make_pair(next_absolute_seqno_, msg));
        next_absolute_seqno_ += msg.sequence_length();

        if (msg.FIN) {
            break;
        }
    }
}

TCPSenderMessage TCPSender::send_empty_message() const {
    // Your code here.
    TCPSenderMessage msg;
    msg.seqno = isn_ + next_absolute_seqno_;
    return msg;
}

void TCPSender::receive(const TCPReceiverMessage &msg) {
    // Your code here.
    if (!msg.ackno.has_value()) { // Don't return directly
    } else {
        const uint64_t recv_abs_seqno = msg.ackno.value().unwrap(isn_, next_absolute_seqno_);
        if (recv_abs_seqno > next_absolute_seqno_) {
            // Impossible, we couldn't transmit future data
            return;
        }

        for (auto iter = outstanding_segments_.begin(); iter != outstanding_segments_.end();) {
            const auto &[abs_seqno, segment] = *iter;
            if (abs_seqno + segment.sequence_length() <= recv_abs_seqno) {
                outstanding_seqno_ -= segment.sequence_length();
                iter = outstanding_segments_.erase(iter);
                // reset RTO and if outstanding data is not empty, start timer
                rto_ = initial_RTO_ms_;
                if (!outstanding_segments_.empty()) {
                    timer_ = 0;
                }
            } else {
                break;
            }
        }
        number_of_retransmissions_ = 0;
    }
    window_size_ = msg.window_size;
}

void TCPSender::tick(const size_t ms_since_last_tick) {
    // Your code here.
    timer_ += ms_since_last_tick;
    auto iter = outstanding_segments_.begin();
    if (timer_ >= rto_ && iter != outstanding_segments_.end()) {
        const auto &[abs_seqno, segment] = *iter;
        if (window_size_ > 0) {
            rto_ *= 2;
        }
        number_of_retransmissions_++;
        timer_ = 0;
        segments_out_.push(segment);
    }
}
