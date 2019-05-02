#ifndef MY_ARRAY_HPP
#define MY_ARRAY_HPP

#include <vector>

template <typename T>
class my_array {
    class my_array_iterator {
    public:
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        my_array_iterator(std::vector<T> &row)
                : m_row(row),
                  m_current_elem{nullptr}
        {}

        bool operator==(const my_array_iterator &other_iterator)
        {
            return m_current_elem == other_iterator.m_current_elem;
        }

        bool operator!=(const my_array_iterator &other_iterator)
        {
            return !(*this == other_iterator);
        }

        reference operator*()
        {
            return *m_current_elem;
        }

        my_array_iterator & operator++()
        {
            m_current_elem++;
            return *this;
        }

    private:
        friend my_array;
        std::vector<T> &m_row;
        T *m_current_elem;
    };

public:
    using iterator = my_array_iterator;

    my_array(std::vector<T> &vector)
        : m_content(vector)
    {}

    my_array_iterator begin()
    {
        my_array_iterator begin_iterator{m_content};
        begin_iterator.m_current_elem = &m_content[0];
        return begin_iterator;
    }

    my_array_iterator end()
    {
        my_array_iterator end_iterator{m_content};
        end_iterator.m_current_elem = &m_content[m_content.size() - 1];
        end_iterator.m_current_elem++;
        return end_iterator;
    }

private:

    std::vector<T> &m_content;
};

#endif //MY_ARRAY_HPP
