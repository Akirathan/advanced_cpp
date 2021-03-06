// du2matrix.hpp
// Pavel Marek NPRG051 2018/2019

#ifndef MATRIX_HPP_
#define MATRIX_HPP_


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

template <typename T>
class matrix {
    using content_type = std::vector<std::vector<T>>;

public:
    class iterator_base {
    public:
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;
    };

    class cols_element_iterator : public iterator_base {
    public:
        using value_type = T;
        using reference = T&;
        using pointer = T*;

        cols_element_iterator(content_type &content, size_t col_idx)
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

        size_t size() const
        {
            return m_content[0].size();
        }

        bool operator==(const cols_element_iterator &other_iterator)
        {
            return m_col_idx == other_iterator.m_col_idx && m_row_idx == other_iterator.m_row_idx
                && &m_content == &other_iterator.m_content;
        }

        bool operator!=(const cols_element_iterator &other_iterator)
        {
            return !(*this == other_iterator);
        }

        reference operator[](size_t row_idx)
        {
            return m_content[row_idx][m_col_idx];
        }

        reference operator*()
        {
            return m_content[m_row_idx][m_col_idx];
        }

        pointer operator->()
        {
            return &m_content[m_row_idx][m_col_idx];
        }

        cols_element_iterator & operator++()
        {
            m_row_idx++;
            return *this;
        }

        cols_element_iterator operator++(int)
        {
            auto old_copy = *this;
            m_row_idx++;
            return old_copy;
        }

    private:
        content_type &m_content;
        size_t m_col_idx;
        size_t m_row_idx;
    };

    class cols_t : public iterator_base {
    public:
        using value_type = cols_element_iterator;
        using reference = cols_element_iterator&;
        using pointer = cols_element_iterator*;

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

        size_t size() const
        {
            return m_content[0].size();
        }

        bool operator==(const cols_t &other_iterator)
        {
            return m_col_idx == other_iterator.m_col_idx && &m_content == &other_iterator.m_content;
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

        pointer operator->()
        {
            m_element_iterator.set_column(m_col_idx);
            return &m_element_iterator;
        }

        cols_t & operator++()
        {
            m_col_idx++;
            return *this;
        }

        cols_t operator++(int)
        {
            cols_t old_copy = *this;
            m_col_idx++;
            return old_copy;
        }

    private:
        content_type &m_content;
        cols_element_iterator m_element_iterator;
        size_t m_col_idx;
    };


    class row_element_iterator : public iterator_base {
    public:
        using value_type = T;
        using reference = T&;
        using pointer = T*;

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

        size_t size() const
        {
            assert(m_row);
            return m_row->size();
        }

        bool operator==(const row_element_iterator &other_iterator)
        {
            return m_current_elem == other_iterator.m_current_elem && m_row == other_iterator.m_row;
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

        pointer operator->()
        {
            return m_current_elem;
        }

        row_element_iterator & operator++()
        {
            m_current_elem++;
            return *this;
        }

        row_element_iterator operator++(int)
        {
            auto old_copy = *this;
            m_current_elem++;
            return old_copy;
        }

    private:
        std::vector<T> *m_row;
        T *m_current_elem;
    };

    class rows_t : public iterator_base {
    public:
        using value_type = row_element_iterator;
        using reference = row_element_iterator&;
        using pointer = row_element_iterator *;

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

        size_t size() const
        {
            return m_content.size();
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
            std::vector<T> *row = get_first_row() + row_idx;
            m_row_element_iterator.set_row(row);
            return m_row_element_iterator;
        }

        reference operator*()
        {
            m_row_element_iterator.set_row(m_current_row);
            return m_row_element_iterator;
        }

        pointer operator->()
        {
            m_row_element_iterator.set_row(m_current_row);
            return &m_row_element_iterator;
        }

        rows_t & operator++()
        {
            m_current_row++;
            return *this;
        }

        rows_t operator++(int)
        {
            auto old_copy = *this;
            m_current_row++;
            return old_copy;
        }

    private:
        content_type &m_content;
        row_element_iterator m_row_element_iterator;
        std::vector<T> *m_current_row;

        std::vector<T> * get_first_row()
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

    matrix(const matrix<T> &other_matrix)
        : m_content(other_matrix.m_content),
        m_rows_iterator{m_content},
        m_cols_iterator{m_content},
        m_row_size{other_matrix.m_row_size},
        m_col_size{other_matrix.m_col_size}
    {}

    matrix(matrix<T> &&other_matrix)
        : m_content(std::move(other_matrix.m_content)),
        m_rows_iterator{m_content},
        m_cols_iterator{m_content},
        m_row_size{other_matrix.m_row_size},
        m_col_size{other_matrix.m_col_size}
    {}

    matrix<T> & operator=(const matrix<T> &other_matrix)
    {
        const size_t new_row_size = other_matrix.m_content.size();
        const size_t new_cols_size = other_matrix.m_content[0].size();

        m_row_size = new_row_size;
        m_col_size = new_cols_size;
        resize_content(new_row_size, new_cols_size);

        copy_content(other_matrix.m_content);

        return *this;
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

    cols_t & cols()
    {
        return m_cols_iterator;
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

    void copy_content(const content_type &content)
    {
        for (size_t i = 0; i < m_row_size; ++i) {
            for (size_t j = 0; j < m_col_size; ++j) {
                m_content[i][j] = content[i][j];
            }
        }
    }

};

#endif //MATRIX_HPP_
