#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>
#include <iostream>


using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender(uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn)
        : isn_(fixed_isn.value_or(Wrap32{random_device()()})), initial_RTO_ms_(initial_RTO_ms) {}

uint64_t TCPSender::sequence_numbers_in_flight() const { return outstanding_seqno_; }

uint64_t TCPSender::consecutive_retransmissions() const { return consecutive_retransmission_times_; }

optional<TCPSenderMessage> TCPSender::maybe_send() {
    if (!segments_out_.empty() && set_syn_) {
        TCPSenderMessage segment = std::move(segments_out_.front());
        segments_out_.pop();
        return segment;
    }

    return nullopt;
}

void TCPSender::push(Reader &outbound_stream) {
    const uint64_t curr_window_size = window_size_ ? window_size_ : 1;
    while (curr_window_size > outstanding_seqno_) {
        TCPSenderMessage msg;

        if (!set_syn_) {
            msg.SYN = true;
            set_syn_ = true;
        }

        msg.seqno = get_next_seqno();
        const uint64_t payload_size
                = min(TCPConfig::MAX_PAYLOAD_SIZE, curr_window_size - outstanding_seqno_ - msg.SYN);
        std::string payload = std::string(outbound_stream.peek()).substr(0, payload_size);
        outbound_stream.pop(payload_size);

        if (!set_fin_ && outbound_stream.is_finished()
            && payload.size() + outstanding_seqno_ + msg.SYN < curr_window_size) {
            msg.FIN = true;
            set_fin_ = true;
        }

        msg.payload = Buffer(std::move(payload));

        // no data, stop sending
        if (msg.sequence_length() == 0) {
            break;
        }

        // no outstanding segments, restart timer
        if (outstanding_seg_.empty()) {
            RTO_timeout_ = initial_RTO_ms_;
            timer_ = 0;
        }

        segments_out_.push(msg);

        outstanding_seqno_ += msg.sequence_length();
        outstanding_seg_.insert(std::make_pair(next_abs_seqno_, msg));
        next_abs_seqno_ += msg.sequence_length();

        if (msg.FIN) {
            break;
        }
    }
}

TCPSenderMessage TCPSender::send_empty_message() const {
    TCPSenderMessage segment;
    segment.seqno = get_next_seqno();

    return segment;
}

void TCPSender::receive(const TCPReceiverMessage &msg) {
    if (!msg.ackno.has_value()) { ; // Don't return directly
    } else {
        const uint64_t recv_abs_seqno = msg.ackno.value().unwrap(isn_, next_abs_seqno_);
        if (recv_abs_seqno > next_abs_seqno_) {
            // Impossible, we couldn't transmit future data
            return;
        }

        for (auto iter = outstanding_seg_.begin(); iter != outstanding_seg_.end();) {
            const auto &[abs_seqno, segment] = *iter;
            if (abs_seqno + segment.sequence_length() <= recv_abs_seqno) {
                outstanding_seqno_ -= segment.sequence_length();
                iter = outstanding_seg_.erase(iter);
                // reset RTO and if outstanding data is not empty, start timer
                RTO_timeout_ = initial_RTO_ms_;
                if (!outstanding_seg_.empty()) {
                    timer_ = 0;
                }
            } else {
                break;
            }
        }
        consecutive_retransmission_times_ = 0;
    }
    window_size_ = msg.window_size;
}

void TCPSender::tick(const size_t ms_since_last_tick) {
    timer_ += ms_since_last_tick;
    auto iter = outstanding_seg_.begin();
    if (timer_ >= RTO_timeout_ && iter != outstanding_seg_.end()) {
        const auto &[abs_seqno, segment] = *iter;
        if (window_size_ > 0) {
            RTO_timeout_ *= 2;
        }
        timer_ = 0;
        consecutive_retransmission_times_++;
        segments_out_.push(segment);
    }
}