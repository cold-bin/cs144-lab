#include "reassembler.hh"

using namespace std;

void Reassembler::insert(uint64_t first_index, string data, bool is_last_substring, Writer &output) {
    // Your code here.

    /* check
     * 1、is out of bound ?
     * 2、is all overlapped ?
     * 3、is data empty?
     * 4、is no available space?
     */
    if (first_index >= unassembled_idx_ + output.available_capacity() || /* out of bound*/
        first_index + data.length() - 1 < unassembled_idx_ || // all overlapped
        data.empty() || /* data empty */
        output.available_capacity() == 0 /* no available space */
            ) {
        // if need be closed
        if (is_last_substring && bytes_pending() == 0) {
            output.close();
        }
        // do nothing
        return;
    }

    // sub data to fit the cap
    uint64_t new_idx = first_index; // type the right start index, after sub data
    if (first_index <= unassembled_idx_) {
        /* overlapped */
        new_idx = unassembled_idx_; // ignore the overlapped bytes of `data`
        const uint64_t overlapped_len = unassembled_idx_ - first_index;
        data = data.substr(overlapped_len, min(data.size() - overlapped_len, output.available_capacity()));
    } else {
        /* not overlapped */
        data = data.substr(0, min(data.size(), output.available_capacity()));// data self size
        if (first_index + data.size() - 1 > unassembled_idx_ + output.available_capacity() - 1) {
            /* need to judge the end index whether that is out of range */
            data = data.substr(0, output.available_capacity() + unassembled_idx_ - first_index);
        }
    }

    // solved overlapped part
    auto rear_iter = inter_storage_.lower_bound(new_idx); // 注意此处，rear_index>=new_idx
    while (rear_iter != inter_storage_.end()) {
        auto &[rear_index, rear_data] = *rear_iter;
        if (new_idx + data.size() - 1 < rear_index) {
            break;
        } // No overlap conflict

        uint64_t rear_overlapped_length = 0;

        // Prepare for next rear early, because the data may be erased afterward.
        const uint64_t next_rear = rear_index + rear_data.size() - 1;

        if (new_idx + data.size() - 1 < rear_index + rear_data.size() - 1) {
            // overlapped case: new_idx rear_index new_end rear_end
            rear_overlapped_length = new_idx + data.size() - rear_index; // new_end - rear_index
            // 在data里删除一部分重叠的，相比从map里取出来再剪掉再放进去更好
            data.erase(data.end() - static_cast<int64_t>(rear_overlapped_length), data.end());
        } else {
            // overlapped case: new_idx rear_index rear_end new_end
            rear_overlapped_length = rear_data.size();
            // 前面已经存储的字节流里现在却是当前字节流的一部分重复，所以，需要删掉以前的那个字节流
            unassembled_bytes_ -= rear_data.size();
            inter_storage_.erase(rear_index);
        }

        // 取出下一个稍大的index，就是不断从后面往前面取比较流
        rear_iter = inter_storage_.lower_bound(next_rear);
    }

    // no overlap
    if (first_index > unassembled_idx_) {
        auto front_iter = inter_storage_.upper_bound(new_idx);
        if (front_iter != inter_storage_.begin()) {
            front_iter--;
            const auto &[front_index, front_data] = *front_iter;

            if (front_index + front_data.size() - 1 >= first_index) {
                uint64_t overlapped_length = 0;
                if (front_index + front_data.size() <= first_index + data.size()) {
                    overlapped_length = front_index + front_data.size() - first_index;
                } else {
                    overlapped_length = data.size();
                }
                if (overlapped_length == front_data.size()) {
                    unassembled_bytes_ -= front_data.size();
                    inter_storage_.erase(front_index);
                } else {
                    data.erase(data.begin(), data.begin() + static_cast<int64_t>(overlapped_length));
                    // Don't forget to update the inserted location
                    new_idx = first_index + overlapped_length;
                }
            }
        }
    }

    // If the processed data is empty, no need to insert it.
    if (!data.empty()) {
        unassembled_bytes_ += data.size();
        inter_storage_.insert(make_pair(new_idx, std::move(data)));
    }

    for (auto iter = inter_storage_.begin(); iter != inter_storage_.end(); /* nop */) {
        auto &[sub_index, sub_data] = *iter;
        // 序号对上了
        if (sub_index == unassembled_idx_) {
            const uint64_t prev_bytes_pushed = output.bytes_pushed();
            output.push(sub_data);// 注意，此处push的实现中，如果超过cap时，会发生数据截断，因此需要在后面再检查是否需要重发被被截断的数据
            const uint64_t bytes_pushed = output.bytes_pushed();
            if (bytes_pushed != prev_bytes_pushed + sub_data.size()) {
                // Cannot push all data, we need to reserve the un-pushed part.
                const uint64_t pushed_length = bytes_pushed - prev_bytes_pushed;
                unassembled_idx_ += pushed_length;
                unassembled_bytes_ -= pushed_length;
                inter_storage_.insert(make_pair(unassembled_idx_, sub_data.substr(pushed_length)));
                // Don't forget to remove the previous incompletely transferred data
                inter_storage_.erase(sub_index);
                break;
            }
            unassembled_idx_ += sub_data.size();
            unassembled_bytes_ -= sub_data.size();
            inter_storage_.erase(sub_index);
            iter = inter_storage_.find(unassembled_idx_);
        } else {
            break; // No need to do more. Data has been discontinuous.
        }
    }

    if (is_last_substring && bytes_pending() == 0) {
        output.close();
    }
}

uint64_t Reassembler::bytes_pending() const {
    // Your code here.
    return unassembled_bytes_;
}