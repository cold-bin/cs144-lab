#include "wrapping_integers.hh"

using namespace std;

// absolute seqno -> seqno
Wrap32 Wrap32::wrap(uint64_t n, Wrap32 zero_point) {
    // Your code here.
    // 其实可以强制转换，这样只是截断低32位，和下面差不多
    // true: static_cast<uint32_t>(0xfff000ff000 % (1L<<32)) == static_cast<uint32_t>(0xfff000ff000)
    return Wrap32{(zero_point + static_cast<uint32_t>(n % Wrap32::MOD_LEN))};
}

// seqno -> absolute seqno
uint64_t Wrap32::unwrap(Wrap32 zero_point, uint64_t checkpoint) const {
    // Your code here.

    const uint32_t seqno_offset = (this->raw_value_ - zero_point.raw_value_ + Wrap32::MOD_LEN) % Wrap32::MOD_LEN;
    /* checkpoint > offset we just need to find the nearest one */
    if (checkpoint > seqno_offset) {
        // 加上半个 Wrap32::MOD_LEN 是为了四舍五入，这样可以找到nearest
        const uint64_t real_checkpoint = checkpoint - seqno_offset + (Wrap32::MOD_LEN >> 1);
        // 找到nearest
        const uint64_t wrap_num = real_checkpoint / Wrap32::MOD_LEN;
        return wrap_num * Wrap32::MOD_LEN + seqno_offset;
    }
    // 如果 checkpoint <= seqno_offset，说明seqno就是最近的了，否则再加 Wrap32::MOD_LEN 就不是nearest了
    return seqno_offset;
}