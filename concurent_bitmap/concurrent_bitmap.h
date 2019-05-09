// concurrent_bitmap.h
// Pavel Marek NPRG051 2018/2019

// Note that the implementation is only in header, because there are some template inner classes.

#include <cstdint>
#include <cassert>
#include <array>
#include <atomic>
#include <mutex>

/**
 * We need this function, because std::pow is not constexpr.
 */
static constexpr std::size_t const_pow(std::size_t base, std::size_t x) noexcept
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
    using value_type = bool;
    using key_type = uint32_t;

    concurrent_bitmap()
        : m_root{*this, l0_bit_range.first, l0_bit_range.second}
    {}

    value_type get(key_type key) const
    {
        return m_root.get(key);
    }

    void set(key_type key, value_type value)
    {
        m_root.set(key, value);
    }

private:
    // 32-bit key is divided into l0, l1, l2, and leaf blocks.
    // Every block represents index into certain node.
    // The indexing is done from most-significant bit (from left).
    static constexpr std::size_t l0_block_bits = 6;
    static constexpr std::size_t l1_block_bits = 6;
    static constexpr std::size_t l2_block_bits = 6;
    static constexpr std::size_t leaf_block_bits = 11;
    // We need 3 bits to index certain bit in a byte.
    static_assert(l0_block_bits + l1_block_bits + l2_block_bits + leaf_block_bits == sizeof(key_type) * 8 - 3);

    // Definitions of various bit ranges and node sizes.
    using bit_range_t = std::pair<std::size_t, std::size_t>;
    static constexpr bit_range_t leaf_bit_range = std::make_pair(3, 3 + leaf_block_bits);
    static constexpr bit_range_t l2_bit_range = std::make_pair(leaf_bit_range.second, leaf_bit_range.second + l2_block_bits);
    static constexpr bit_range_t l1_bit_range = std::make_pair(l2_bit_range.second, l2_bit_range.second + l1_block_bits);
    static constexpr bit_range_t l0_bit_range = std::make_pair(l1_bit_range.second, l1_bit_range.second + l0_block_bits);
    static constexpr std::size_t l0_node_size = const_pow(2, l0_block_bits);
    static constexpr std::size_t l1_node_size = const_pow(2, l1_block_bits);
    static constexpr std::size_t l2_node_size = const_pow(2, l2_block_bits);
    static constexpr std::size_t leaf_node_size = const_pow(2, leaf_block_bits);

    class i_node {
    public:
        i_node(std::size_t bit_idx_from, std::size_t bit_idx_to)
            : m_bit_idx_from{static_cast<uint8_t>(bit_idx_from)},
            m_bit_idx_to{static_cast<uint8_t>(bit_idx_to)}
        {}
        virtual ~i_node() = default;
        virtual void set(key_type key, value_type value) = 0;
        virtual value_type get(key_type key) const = 0;

    protected:
        const uint8_t m_bit_idx_from;
        const uint8_t m_bit_idx_to;

        /// Deduces array index from key.
        std::size_t get_index_from_key(key_type key) const
        {
            key_type mask = static_cast<key_type>(1 << (m_bit_idx_to - m_bit_idx_from));
            mask--;
            key >>= m_bit_idx_from;
            key &= mask;
            return static_cast<std::size_t>(key);
        }
    };

    template <std::size_t array_size>
    class inner_node : public i_node {
    public:
        inner_node(const concurrent_bitmap &bitmap, std::size_t bit_idx_from, std::size_t bit_idx_to)
            : i_node{bit_idx_from, bit_idx_to},
            m_bitmap{bitmap}
        {
            for (auto &&child : m_children) {
                child = nullptr;
            }
        }

        ~inner_node() override
        {
            for (auto &&child : m_children) {
                if (child != nullptr) {
                    delete child;
                }
            }
        }

        void set(key_type key, value_type value) override
        {
            std::size_t idx = get_index_from_key(key);
            if (m_children[idx] == nullptr) {
                std::lock_guard<std::mutex> lock{m_mtx};
                if (m_children[idx] == nullptr) {
                    // Create new child node.
                    auto [next_from_idx, next_to_idx] = m_bitmap.get_next_bit_indexes(m_bit_idx_from, m_bit_idx_to);
                    m_children[idx] = m_bitmap.create_bitmap_node(next_from_idx, next_to_idx);
                }
            }
            m_children[idx]->set(key, value);
        }

        value_type get(key_type key) const override
        {
            std::size_t idx = get_index_from_key(key);
            if (m_children[idx] == nullptr) {
                return false;
            }
            else {
                return m_children[idx]->get(key);
            }
        }

    private:
        const concurrent_bitmap &m_bitmap;
        std::mutex m_mtx;
        std::array<i_node *, array_size> m_children;
    };

    template <std::size_t array_size>
    class leaf_node : public i_node {
    public:
        leaf_node(std::size_t bit_idx_from, std::size_t bit_idx_to)
            : i_node{bit_idx_from, bit_idx_to},
            m_data{}
        {}

        void set(key_type key, value_type value) override
        {
            std::size_t byte_idx = get_index_from_key(key);
            std::size_t bit_idx = get_bit_index(key);
            if (value) {
                m_data[byte_idx] |= (1 << bit_idx);
            }
            else {
                m_data[byte_idx] &= ~(1 << bit_idx);
            }
        }

        value_type get(key_type key) const override
        {
            std::size_t byte_idx = get_index_from_key(key);
            std::size_t bit_idx = get_bit_index(key);
            return (m_data[byte_idx] & (1 << bit_idx)) != 0;
        }

    private:
        static constexpr key_type bit_idx_mask = 0x000000007;
        std::array<std::atomic<uint8_t>, array_size> m_data;

        /// Deduces index of bit from last 3 bits in key.
        std::size_t get_bit_index(key_type key) const
        {
            key &= bit_idx_mask;
            return static_cast<std::size_t>(key);
        }
    };

    inner_node<l0_node_size> m_root;

    /**
     * Returns pair of bit indexes that follows given pair of indexes, eg. l1 follows l0, l2 follows l1, etc.
     * @param bit_idx_from Current min bit index.
     * @param bit_idx_to Current max bit index.
     */
    std::pair<std::size_t, std::size_t> get_next_bit_indexes(std::size_t bit_idx_from, std::size_t bit_idx_to) const
    {
        assert(leaf_bit_range.first <= bit_idx_from && bit_idx_to <= l0_bit_range.second);

        if (are_indexes_in_bitrange(l0_bit_range, bit_idx_from, bit_idx_to)) {
            return l1_bit_range;
        }
        else if (are_indexes_in_bitrange(l1_bit_range, bit_idx_from, bit_idx_to)) {
            return l2_bit_range;
        }
        else if (are_indexes_in_bitrange(l2_bit_range, bit_idx_from, bit_idx_to)) {
            return leaf_bit_range;
        }
        else {
            return std::make_pair(0, 0);
        }
    }

    /**
     * Creates either inner node or leaf node depending on parameters.
     * @param bit_idx_from Min bit index for new node.
     * @param bit_idx_to Max bit index for new node.
     */
    i_node * create_bitmap_node(std::size_t bit_idx_from, std::size_t bit_idx_to) const
    {
        assert(leaf_bit_range.first <= bit_idx_from && bit_idx_to <= l1_bit_range.second);

        if (are_indexes_in_bitrange(l1_bit_range, bit_idx_from, bit_idx_to)) {
            return new inner_node<l1_node_size>{*this, bit_idx_from, bit_idx_to};
        }
        else if (are_indexes_in_bitrange(l2_bit_range, bit_idx_from, bit_idx_to)) {
            return new inner_node<l2_node_size>{*this, bit_idx_from, bit_idx_to};
        }
        else if (are_indexes_in_bitrange(leaf_bit_range, bit_idx_from, bit_idx_to)) {
            return new leaf_node<leaf_node_size>{bit_idx_from, bit_idx_to};
        }
        else {
            return nullptr;
        }
    }

    bool are_indexes_in_bitrange(const bit_range_t &bitrange, std::size_t idx_from, std::size_t idx_to) const
    {
        return bitrange.first <= idx_from && idx_to <= bitrange.second;
    }
};
