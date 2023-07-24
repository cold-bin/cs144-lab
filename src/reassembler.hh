#pragma once

#include "byte_stream.hh"
#include <map>
class Reassembler {

private:
    std::map<uint64_t ,std::string> inter_storage_{};
    uint64_t unassembled_bytes_{0}; // 未组装好的字节数目，也就是留在内部存储中
    uint64_t unassembled_idx_{0};
public:
    uint64_t getUnassembledIdx() const;

private:
    // 当前处理到的idx
    bool closed_{false};
public:
    Reassembler() = default;

    /*
     * Insert a new substring to be reassembled into a ByteStream.
     *   `first_index`: the index of the first byte of the substring
     *   `data`: the substring itself
     *   `is_last_substring`: this substring represents the end of the stream
     *   `output`: a mutable reference to the Writer
     *
     * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
     * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
     * learns the next byte in the stream, it should write it to the output.
     *
     * If the Reassembler learns about bytes that fit within the stream's available capacity
     * but can't yet be written (because earlier bytes remain unknown), it should store them
     * internally until the gaps are filled in.
     *
     * The Reassembler should discard any bytes that lie beyond the stream's available capacity
     * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
     *
     * The Reassembler should close the stream after writing the last byte.
     */
    void insert(uint64_t first_index, std::string data, bool is_last_substring, Writer &output);

    // How many bytes are stored in the Reassembler itself?
    uint64_t bytes_pending() const;
};
