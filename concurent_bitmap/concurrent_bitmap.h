#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <boost/log/trivial.hpp>

/**
 * We need this function, because std::pow is not constexpr.
 */
constexpr std::size_t const_pow(std::size_t base, std::size_t x) noexcept
{
    std::size_t res = base;
    while (x > 1) {
        res *= base;
        x--;
    }
    return res;
}

class concurrent_bitmap {
public:
    using key_type = uint32_t;
    using value_type = bool;

    concurrent_bitmap()
    {
        m_l0_array.fill(nullptr);
    }

    ~concurrent_bitmap()
    {
        for (l1_array_t *l1_array : m_l0_array) {
            if (l1_array) {
                for (l2_array_t *l2_array: *l1_array) {
                    if (l2_array) {
                        for (leaf_block_t *leaf_block: *l2_array) {
                            if (leaf_block) {
                                delete leaf_block;
                            }
                        }
                        delete l2_array;
                    }
                }
                delete l1_array;
            }
        }
    }

    value_type get(key_type key) const
    {
        std::size_t l0_idx = get_index_to_l0(key);
        if (m_l0_array[l0_idx] == nullptr) {
            return false;
        }

        l1_array_t &l1_array = *m_l0_array[l0_idx];
        std::size_t l1_idx = get_index_to_l1(key);
        if (l1_array[l1_idx] == nullptr) {
            return false;
        }

        l2_array_t &l2_array = *l1_array[l1_idx];
        std::size_t l2_idx = get_index_to_l2(key);
        if (l2_array[l2_idx] == nullptr) {
            return false;
        }

        leaf_block_t &leaf_block = *l2_array[l2_idx];
        std::size_t leaf_block_idx = get_index_to_leaf_block(key);
        std::size_t byte_idx = get_index_to_byte(key);
        return get_bit(leaf_block[leaf_block_idx], byte_idx);
    }

    void set(key_type key, value_type value)
    {
        set_into_l0(key, value);
    }

private:
    static constexpr std::size_t l0_bits = 6;
    static constexpr std::size_t l1_bits = 6;
    static constexpr std::size_t l2_bits = 6;
    static constexpr std::size_t leaf_block_bits = 11;
    static_assert(l0_bits + l1_bits + l2_bits + leaf_block_bits == sizeof(key_type) * 8 - 3);
    static constexpr std::size_t leaf_block_array_size = const_pow(2, leaf_block_bits + 3);
    static constexpr std::size_t l0_array_size = const_pow(2, l0_bits);
    static constexpr std::size_t l1_array_size = const_pow(2, l1_bits);
    static constexpr std::size_t l2_array_size = const_pow(2, l2_bits);
    static constexpr key_type l_mask = 0x0000003F;
    static constexpr key_type leaf_block_mask = 0x000007FF;
    static constexpr key_type byte_idx_mask = 0x000000007;

    using leaf_block_t = std::array<uint8_t, leaf_block_array_size>;
    using l2_array_t = std::array<leaf_block_t *, l2_array_size>;
    using l1_array_t = std::array<l2_array_t *, l1_array_size>;
    using l0_array_t = std::array<l1_array_t *, l0_array_size>;

    l0_array_t m_l0_array;


    void set_into_l0(key_type key, value_type value)
    {
        std::size_t l0_idx = get_index_to_l0(key);
        if (m_l0_array[l0_idx] == nullptr) {
            BOOST_LOG_TRIVIAL(debug) << "Creating new L1 array at l0_idx=" << l0_idx;
            m_l0_array[l0_idx] = new l1_array_t{};
            m_l0_array[l0_idx]->fill(nullptr); // TODO: Is this necessary?
        }
        set_into_l1(*m_l0_array[l0_idx], key, value);
    }

    void set_into_l1(l1_array_t &l1_array, key_type key, value_type value)
    {
        std::size_t l1_idx = get_index_to_l1(key);
        if (l1_array[l1_idx] == nullptr) {
            BOOST_LOG_TRIVIAL(debug) << "Creating new L2 array at l1_idx=" << l1_idx;
            l1_array[l1_idx] = new l2_array_t{};
            l1_array[l1_idx]->fill(nullptr); // TODO: Is this necessary?
        }
        set_into_l2(*l1_array[l1_idx], key, value);
    }

    void set_into_l2(l2_array_t &l2_array, key_type key, value_type value)
    {
        std::size_t l2_idx = get_index_to_l2(key);
        if (l2_array[l2_idx] == nullptr) {
            BOOST_LOG_TRIVIAL(debug) << "Creating new leaf block at l2_idx=" << l2_idx;
            l2_array[l2_idx] = new leaf_block_t{};
            l2_array[l2_idx]->fill(0); // TODO: Is this necessary?
        }
        set_into_leaf(*l2_array[l2_idx], key, value);
    }

    void set_into_leaf(leaf_block_t &leaf_block, key_type key, value_type value)
    {
        std::size_t leaf_block_idx = get_index_to_leaf_block(key);
        std::size_t byte_idx = get_index_to_byte(key);
        BOOST_LOG_TRIVIAL(debug) << "Setting bit idx="<< byte_idx << " at leaft block idx=" << leaf_block_idx;
        uint8_t byte = leaf_block[leaf_block_idx];
        if (value) {
            byte = set_bit(byte, byte_idx);
        }
        else {
            byte = reset_bit(byte, byte_idx);
        }
        leaf_block[leaf_block_idx] = byte;
    }

    std::size_t get_index_to_l0(key_type key) const
    {
        return l_mask & key;
    }

    std::size_t get_index_to_l1(key_type key) const
    {
        std::size_t idx = key >> l0_bits;
        idx &= l_mask;
        return idx;
    }

    std::size_t get_index_to_l2(key_type key) const
    {
        std::size_t idx = key >> (l0_bits + l1_bits);
        idx &= l_mask;
        return idx;
    }

    std::size_t get_index_to_leaf_block(key_type key) const
    {
        std::size_t idx = key >> (l0_bits + l1_bits + l2_bits);
        idx &= leaf_block_mask;
        return idx;
    }

    std::size_t get_index_to_byte(key_type key) const
    {
        std::size_t idx = key >> (l0_bits + l1_bits + l2_bits + leaf_block_bits);
        idx &= byte_idx_mask;
        return idx;
    }

    uint8_t set_bit(uint8_t byte, std::size_t idx) const
    {
        byte |= (1 << idx);
        return byte;
    }

    uint8_t reset_bit(uint8_t byte, std::size_t idx) const
    {
        byte &= (1 << idx);
        return byte;
    }

    bool get_bit(uint8_t byte, std::size_t idx) const
    {
        byte >>= idx;
        byte &= 0x01;
        return byte;
    }
};
