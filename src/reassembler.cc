#include "reassembler.hh"

using namespace std;

void Reassembler::insert(uint64_t first_index, string data, bool is_last_substring, Writer &output) {
    // Your code here.

    // over the left `cap`,do nothing and throw data aside
    if (first_index >= cap_ || data.size() > cap_) {
        return;
    }

    // maybe the storage is not enough, send and free them
    if (data.size() + bytes_pending() > cap_) {
        for (uint64_t i = 0; i < inter_storage_.size(); ++i) {
            // 出现失序的数据了，不能push到bytestream里
            if (inter_storage_.at(i).empty()) { break; }
            // 没有乱序时，继续push
            output.push(inter_storage_[i]);
            inter_storage_[i].clear();
            available_cap_--;
        }
    }

    // overlapping
    if (!inter_storage_[first_index].empty()) {
        return;
    }

    inter_storage_[first_index] = data;
    available_cap_ += data.length();

    if (is_last_substring) {
        output.close();
    }
}

uint64_t Reassembler::bytes_pending() const {
    // Your code here.
    return available_cap_;
}

Reassembler::Reassembler(uint64_t cap) : cap_(cap), inter_storage_(cap) {}