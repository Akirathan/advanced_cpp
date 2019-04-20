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

    class cols_element_iterator;
    class cols_t;
    class row_element_iterator;
    class rows_t;


    class cols_element_iterator {
    public:
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        explicit cols_element_iterator(content_type &content, size_t col_idx)
            : m_content(content),
            m_col_idx{col_idx},
            m_row_idx{0}
        {}

        void set_column(size_t col_idx)
        {
            m_col_idx = col_idx;
            m_row_idx = 0;
        }

        cols_element_iterator begin()
        {
            cols_element_iterator begin_iterator{m_content, m_col_idx};
            begin_iterator.m_row_idx = 0;
            return begin_iterator;
        }

        cols_element_iterator end()
        {
            cols_element_iterator end_iterator{m_content, m_col_idx};
            end_iterator.m_row_idx = m_content.size();
            return end_iterator;
        }

        bool operator==(const cols_element_iterator &other_iterator)
        {
            return m_col_idx == other_iterator.m_col_idx && m_row_idx == other_iterator.m_row_idx;
        }

        bool operator!=(const cols_element_iterator &other_iterator)
        {
            return !(*this == other_iterator);
        }

        reference operator[](size_t row_idx)
        {
            if (!is_index_in_bounds(m_content, row_idx, m_col_idx)) {
                // TODO: out_of_bounds exception
                throw std::exception{};
            }

            return m_content[row_idx][m_col_idx];
        }

        reference operator*()
        {
            assert(is_index_in_bounds(m_content, m_row_idx, m_col_idx));
            return m_content[m_row_idx][m_col_idx];
        }

        cols_element_iterator & operator++()
        {
            m_row_idx++;
            return *this;
        }

    private:
        content_type &m_content;
        size_t m_col_idx;
        size_t m_row_idx;
    };

    class cols_t {
    public:
        using value_type = cols_element_iterator;
        using reference = cols_element_iterator&;
        using pointer = cols_element_iterator*;
        using difference_type = std::ptrdiff_t; // TODO: Neco jinyho?
        using iterator_category = std::forward_iterator_tag;

        explicit cols_t(content_type &content)
            : m_content(content),
            m_element_iterator{content, 0},
            m_col_idx{0}
        {}

        cols_t begin()
        {
            cols_t begin_iterator{m_content};
            begin_iterator.m_col_idx = 0;
            return begin_iterator;
        }

        cols_t end()
        {
            cols_t end_iterator{m_content};
            end_iterator.m_col_idx = m_content[0].size();
            return end_iterator;
        }

        bool operator==(const cols_t &other_iterator)
        {
            return m_col_idx == other_iterator.m_col_idx;
        }

        bool operator!=(const cols_t &other_iterator)
        {
            return !(*this == other_iterator);
        }

        reference operator[](size_t col_idx)
        {
            m_element_iterator.set_column(col_idx);
            return m_element_iterator;
        }

        reference operator*()
        {
            m_element_iterator.set_column(m_col_idx);
            return m_element_iterator;
        }

        cols_t & operator++()
        {
            m_col_idx++;
            return *this;
        }

    private:
        content_type &m_content;
        cols_element_iterator m_element_iterator;
        size_t m_col_idx;
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

        reference operator[](size_t idx)
        {
            assert(m_row);
            return *(m_row->data() + idx);
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
        using pointer = std::vector<T> *; // TODO: Fix
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        explicit rows_t(content_type &content)
                : m_content{content},
                  m_current_row{nullptr}
        { }

        rows_t begin()
        {
            rows_t begin_iterator{m_content};
            begin_iterator.m_current_row = get_first_row();
            return begin_iterator;
        }

        rows_t end()
        {
            rows_t end_iterator{m_content};
            end_iterator.m_current_row = get_first_row() + m_content.size();
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

        reference operator[](size_t row_idx)
        {
            pointer row = get_first_row() + row_idx;
            m_row_element_iterator.set_row(row);
            return m_row_element_iterator;
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
        std::vector<T> *m_current_row;

        pointer get_first_row()
        {
            return m_content.data();
        }
    };


    matrix(size_t row_size, size_t cols_size, T initial_value = T{})
        : m_rows_iterator{m_content},
        m_cols_iterator{m_content},
        m_row_size{row_size},
        m_col_size{cols_size}
    {
        resize_content(row_size, cols_size, initial_value);
    }

    matrix<T> & operator=(const matrix<T> &other_matrix)
    {
        const size_t new_row_size = other_matrix.m_content.size();
        const size_t new_cols_size = other_matrix.m_content[0].size();

        m_row_size = new_row_size;
        m_col_size = new_cols_size;
        resize_content(new_row_size, new_cols_size);

        for (size_t i = 0; i < new_row_size; ++i) {
            for (size_t j = 0; j < new_cols_size; ++j) {
                m_content[i][j] = other_matrix.m_content[i][j];
            }
        }
    }

    row_element_iterator operator[](size_t row_idx)
    {
        row_element_iterator element_iterator;
        std::vector<T> *row = &m_content[row_idx];
        element_iterator.set_row(row);
        return element_iterator;
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
        return m_cols_iterator;
    }

    const cols_t & cols() const
    {

    }

private:
    friend class matrix_tester;

    content_type m_content;
    rows_t m_rows_iterator;
    cols_t m_cols_iterator;
    size_t m_row_size;
    size_t m_col_size;

    void resize_content(size_t row_size, size_t col_size, T initial_value = T{})
    {
        m_content.resize(row_size);
        for (auto &&row : m_content) {
            row.resize(col_size, initial_value);
        }
    }

    static bool is_index_in_bounds(const content_type &content, size_t i, size_t j)
    {
        size_t max_row_idx = content.size() - 1;
        size_t max_col_idx = content[0].size() - 1;
        return i >= 0 && i <= max_row_idx && j >= 0 && j <= max_col_idx;
    }
};

#endif //MATRIX_MATRIX_HPP
