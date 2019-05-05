// concurrent_bitmap.h
// Pavel Marek NPRG051 2018/2019

#define USE_BOOST

#include <cstdint>
#include <cassert>
#include <array>
#include <atomic>
#include <mutex>
#ifdef USE_BOOST
#include <boost/log/trivial.hpp>
#endif

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

// debug
static std::string indentation(std::size_t indent_level)
{
    std::ostringstream output;
    for (std::size_t i = 0; i < indent_level; ++i)
        output << "  ";
    return output.str();
}

// debug
struct nodes_count {
    std::size_t inner_nodes_count = 0;
    std::size_t leaves_count = 0;
};



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

    std::size_t get_bytes_count() const
    {
        return m_root.get_set_bytes();
    }

    nodes_count get_nodes_count() const
    {
        return m_root.get_nodes_count(nodes_count{});
    }

    void log() const
    {
        m_root.log(0);
    }

    void log_bytes_count() const
    {
#ifdef USE_BOOST
        BOOST_LOG_TRIVIAL(info) << "Total set bytes count = " << m_root.get_set_bytes();
#endif
    }

    void log_nodes_count() const
    {
        auto nodes_count_ = m_root.get_nodes_count(nodes_count{});
#ifdef USE_BOOST
        BOOST_LOG_TRIVIAL(info) << "Nodes count:" << "\n\t Leaves count = " << nodes_count_.leaves_count
                                                    << "\n\t Inner nodes count = " << nodes_count_.inner_nodes_count;
#endif
    }

private:
    static constexpr std::size_t l0_bits = 6;
    static constexpr std::size_t l1_bits = 6;
    static constexpr std::size_t l2_bits = 6;
    static constexpr std::size_t leaf_block_bits = 11;
    static_assert(l0_bits + l1_bits + l2_bits + leaf_block_bits == sizeof(key_type) * 8 - 3);
    using bit_range_t = std::pair<std::size_t, std::size_t>;
    static constexpr bit_range_t l0_bit_range = std::make_pair(0, l0_bits);
    static constexpr bit_range_t l1_bit_range = std::make_pair(l0_bit_range.second, l0_bit_range.second + l1_bits);
    static constexpr bit_range_t l2_bit_range = std::make_pair(l1_bit_range.second, l1_bit_range.second + l2_bits);
    static constexpr bit_range_t leaf_bit_range = std::make_pair(l2_bit_range.second, l2_bit_range.second + leaf_block_bits);
    static constexpr std::size_t l0_array_size = const_pow(2, l0_bits);
    static constexpr std::size_t l1_array_size = const_pow(2, l1_bits);
    static constexpr std::size_t l2_array_size = const_pow(2, l2_bits);
    static constexpr std::size_t leaf_block_array_size = const_pow(2, leaf_block_bits);

    class i_bitmap_node {
    public:
        i_bitmap_node(std::size_t bit_idx_from, std::size_t bit_idx_to)
            : m_bit_idx_from{static_cast<uint8_t>(bit_idx_from)},
            m_bit_idx_to{static_cast<uint8_t>(bit_idx_to)}
        {
            assert(bit_idx_from < bit_idx_to);
        }

        virtual ~i_bitmap_node() = default;
        virtual void set(key_type key, value_type value) = 0;
        virtual value_type get(key_type key) const = 0;
        virtual void log(std::size_t indent_level) const = 0;
        virtual std::size_t get_set_bytes() const = 0;
        virtual nodes_count get_nodes_count(nodes_count accumulator) const = 0;

    protected:
        const uint8_t m_bit_idx_from;
        const uint8_t m_bit_idx_to;

        std::size_t get_index_from_key(key_type key) const
        {
            key_type mask = static_cast<key_type>(1 << (m_bit_idx_to - m_bit_idx_from));
            mask--;

            key >>= m_bit_idx_from;
            key &= mask;
            return static_cast<std::size_t>(key);
        }

        int get_children_array_size() const
        {
            return 1 << (m_bit_idx_to - m_bit_idx_from);
        }
    };

    class bitmap_node : public i_bitmap_node {
    public:
        bitmap_node(const concurrent_bitmap &bitmap, std::size_t bit_idx_from, std::size_t bit_idx_to)
            : i_bitmap_node{bit_idx_from, bit_idx_to},
            m_bitmap{bitmap}
        {
            int array_size = get_children_array_size();
            m_children = new i_bitmap_node * [array_size];

            for (int i = 0; i < array_size; ++i) {
                m_children[i] = nullptr;
            }
        }

        ~bitmap_node() override
        {
            int array_size = get_children_array_size();
            for (int i = 0; i < array_size; ++i) {
                if (m_children[i] != nullptr) {
                    delete m_children[i];
                }
            }
            delete[] m_children;
        }

        void set(key_type key, value_type value) override
        {
            std::size_t idx = get_index_from_key(key);
            if (m_children[idx] == nullptr) {
                auto [next_from_idx, next_to_idx] = m_bitmap.get_next_bit_indexes(m_bit_idx_from, m_bit_idx_to);
                std::lock_guard<std::mutex> lock{m_mtx};
                if (m_children[idx] == nullptr) {
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

        void log(std::size_t indent_level) const override
        {
            std::size_t children_count = 0;
            int array_size = get_children_array_size();
            for (int i = 0; i < array_size; ++i) {
                if (m_children[i] != nullptr) {
                    children_count++;
                }
            }
#ifdef USE_BOOST
            BOOST_LOG_TRIVIAL(debug) << indentation(indent_level) << "Node: children count = " << children_count;
#endif

            indent_level++;
            for (int i = 0; i < array_size; ++i) {
                if (m_children[i] != nullptr) {
                    m_children[i]->log(indent_level);
                }
            }
        }

        std::size_t get_set_bytes() const override
        {
            int array_size = get_children_array_size();
            std::size_t set_bytes_count = 0;
            for (int i = 0; i < array_size; ++i) {
                if (m_children[i] != nullptr) {
                    set_bytes_count += m_children[i]->get_set_bytes();
                }
            }
            return set_bytes_count;
        }

        nodes_count get_nodes_count(nodes_count accumulator) const override
        {
            accumulator.inner_nodes_count++;
            int array_size = get_children_array_size();
            for (int i = 0; i < array_size; ++i) {
                if (m_children[i] != nullptr) {
                    accumulator = m_children[i]->get_nodes_count(accumulator);
                }
            }
            return accumulator;
        }

    private:
        const concurrent_bitmap &m_bitmap;
        std::mutex m_mtx;
        i_bitmap_node **m_children;
    };

    class bitmap_leaf_node : public i_bitmap_node {
    public:
        bitmap_leaf_node(std::size_t bit_idx_from, std::size_t bit_idx_to)
            : i_bitmap_node{bit_idx_from, bit_idx_to}
        {
            int array_size = get_children_array_size();

            m_data = new std::atomic<uint8_t>[array_size];
            for (int i = 0; i < array_size; ++i) {
                m_data[i] = 0;
            }
        }

        ~bitmap_leaf_node() override
        {
            delete[] m_data;
        }

        void set(key_type key, value_type value) override
        {
            std::size_t byte_idx = get_index_from_key(key);
            std::size_t bit_idx = get_bit_index(key);
            uint8_t byte = m_data[byte_idx];
            if (value) {
                byte = set_bit(byte, bit_idx);
            }
            else {
                byte = reset_bit(byte, bit_idx);
            }
            m_data[byte_idx] = byte;
        }

        value_type get(key_type key) const override
        {
            std::size_t byte_idx = get_index_from_key(key);
            std::size_t bit_idx = get_bit_index(key);
            uint8_t byte = m_data[byte_idx];
            return get_bit(byte, bit_idx);
        }

        void log(std::size_t indent_level) const override
        {
            int array_size = get_children_array_size();
            std::size_t set_bytes_count = 0;

            for (int i = 0; i < array_size; ++i) {
                if (m_data[i] != 0)
                    set_bytes_count++;
            }

#ifdef USE_BOOST
            BOOST_LOG_TRIVIAL(debug) << indentation(indent_level) << "Leaf: set bytes count = " << set_bytes_count;
#endif
        }

        std::size_t get_set_bytes() const override
        {
            int array_size = get_children_array_size();
            std::size_t set_bytes_count = 0;
            for (int i = 0; i < array_size; ++i) {
                if (m_data[i] != 0)
                    set_bytes_count++;
            }
            return set_bytes_count;
        }

        nodes_count get_nodes_count(nodes_count accumulator) const override
        {
            accumulator.leaves_count++;
            return accumulator;
        }

    private:
        static constexpr key_type bit_idx_mask = 0x000000007;
        std::atomic<uint8_t> *m_data;

        std::size_t get_bit_index(key_type key) const
        {
            std::size_t idx = key >> (sizeof(key_type)*8 - 3);
            idx &= bit_idx_mask;
            assert(idx >= 0 && idx <= 7);
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

    bitmap_node m_root;

    std::pair<std::size_t, std::size_t> get_next_bit_indexes(std::size_t bit_idx_from, std::size_t bit_idx_to) const
    {
        assert(l0_bit_range.first <= bit_idx_from && bit_idx_to <= l2_bit_range.second);

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

    i_bitmap_node * create_bitmap_node(std::size_t bit_idx_from, std::size_t bit_idx_to) const
    {
        assert(l1_bit_range.first <= bit_idx_from && bit_idx_to <= leaf_bit_range.second);

        if (are_indexes_in_bitrange(l1_bit_range, bit_idx_from, bit_idx_to)) {
#ifdef USE_BOOST
            BOOST_LOG_TRIVIAL(debug) << "Creating L1 bitmap node, bit_idx_from="<< bit_idx_from
                                     << ", bit_idx_to=" << bit_idx_to;
#endif
            return new bitmap_node{*this, bit_idx_from, bit_idx_to};
        }
        else if (are_indexes_in_bitrange(l2_bit_range, bit_idx_from, bit_idx_to)) {
#ifdef USE_BOOST
            BOOST_LOG_TRIVIAL(debug) << "Creating L2 bitmap node, bit_idx_from="<< bit_idx_from
                                     << ", bit_idx_to=" << bit_idx_to;
#endif
            return new bitmap_node{*this, bit_idx_from, bit_idx_to};
        }
        else if (are_indexes_in_bitrange(leaf_bit_range, bit_idx_from, bit_idx_to)) {
#ifdef USE_BOOST
            BOOST_LOG_TRIVIAL(debug) << "Creating leaf node, bit_idx_from="<< bit_idx_from
                                     << ", bit_idx_to=" << bit_idx_to;
#endif
            return new bitmap_leaf_node{bit_idx_from, bit_idx_to};
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
