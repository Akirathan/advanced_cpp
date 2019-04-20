#ifndef MATRIX_MATRIX_HPP
#define MATRIX_MATRIX_HPP

/**
 * Various links:
 * - Assignment details: http://www.ksi.mff.cuni.cz/lectures/NPRG051/du/du-1819-2.pdf
 * - iterator trais: https://en.cppreference.com/w/cpp/iterator/iterator_traits
 * - legacy forward iterator: https://en.cppreference.com/w/cpp/named_req/ForwardIterator
 */

#include <cstddef>
#include <cassert>
#include <vector>
#include <iterator>
#include <boost/log/trivial.hpp>

template <typename T>
class matrix {
    using content_type = std::vector<std::vector<T>>;

public:
    using value_type = T;
    using reference = T&;
    using pointer = T*;

    class cols_t;
    class cols_iterator;
    class row_element_iterator;
    class rows_t;

    class cols_t {

    };

    class cols_iterator {

    };


    class row_element_iterator {
    public:
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        row_element_iterator()
            : m_row{nullptr},
            m_current_elem{nullptr}
        {}

        void set_row(std::vector<T> *row)
        {
            m_row = row;
            m_current_elem = m_row->data();
        }

        pointer begin()
        {
            assert(m_row);
            return m_row->data();
        }

        pointer end()
        {
            assert(m_row);
            return m_row->data() + m_row->size();
        }

        bool operator==(const row_element_iterator &other_iterator)
        {
            return m_current_elem == other_iterator.m_current_elem;
        }

        bool operator!=(const row_element_iterator &other_iterator)
        {
            return !(*this == other_iterator);
        }

        reference operator*()
        {
            return *m_current_elem;
        }

        row_element_iterator & operator++()
        {
            m_current_elem++;
            return *this;
        }

    private:
        std::vector<T> *m_row;
        T *m_current_elem;
    };

    /**
     * Entire row iterator.
     */
    class rows_t {
    public:
        using value_type = row_element_iterator;
        using reference = row_element_iterator&;
        using pointer = std::vector<T> *;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        explicit rows_t(content_type &content)
                : m_content{content},
                  m_current_row{nullptr}
        { }

        rows_t begin()
        {
            rows_t begin_iterator{m_content};
            begin_iterator.m_current_row = m_content.data();
            return begin_iterator;
        }

        rows_t end()
        {
            rows_t end_iterator{m_content};
            end_iterator.m_current_row = m_content.data() + m_content.size();
            return end_iterator;
        }

        bool operator==(const rows_t &other_iterator)
        {
            return m_current_row == other_iterator.m_current_row;
        }

        bool operator!=(const rows_t &other_iterator)
        {
            return !(*this == other_iterator);
        }

        reference operator*()
        {
            m_row_element_iterator.set_row(m_current_row);
            BOOST_LOG_TRIVIAL(debug) << "rows_t operator *, m_current_row = " << m_current_row;
            return m_row_element_iterator;
        }

        rows_t & operator++()
        {
            m_current_row++;
            return *this;
        }

    private:
        content_type &m_content;
        row_element_iterator m_row_element_iterator;
        // TODO: Implement with pointer to std::vector<T>.
        std::vector<T> *m_current_row;
    };


    matrix(size_t row_size, size_t cols_size, T initial_value = T{})
        : m_rows_iterator{m_content},
        m_row_size{row_size},
        m_col_size{cols_size}
    {
        m_content.resize(row_size);
        for (auto &&row : m_content) {
            row.resize(cols_size, initial_value);
        }
    }

    size_t get_row_size() const
    {
        return m_row_size;
    }

    size_t get_col_size() const
    {
        return m_col_size;
    }

    rows_t & rows()
    {
        return m_rows_iterator;
    }

    const rows_t & rows() const
    {

    }

    cols_t & cols()
    {

    }

    const cols_t & cols() const
    {

    }

private:
    friend class matrix_tester;

    content_type m_content;
    rows_t m_rows_iterator;
    size_t m_row_size;
    size_t m_col_size;
};

#endif //MATRIX_MATRIX_HPP
