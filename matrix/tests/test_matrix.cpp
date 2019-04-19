//
// Created by pal on 18.4.19.
//

#define BOOST_TEST_MODULE Matrix_Tests

#include <boost/test/included/unit_test.hpp>
#include "../matrix.hpp"

class matrix_tester {
public:
    template <typename T>
    static void check_matrix_empty(const matrix<T> &matrix)
    {
        for (size_t i = 0; i < matrix.get_row_size(); ++i) {
            for (size_t j = 0; j < matrix.get_col_size(); ++j) {
                BOOST_TEST(matrix.m_content[i][j] == T{});
            }
        }
    }

    template <typename T>
    static void check_matrix_element(const matrix<T> &matrix, size_t i, size_t j, T element)
    {
        BOOST_TEST(matrix.m_content[i][j] == element);
    }
};

template <typename IteratorType1, typename IteratorType2>
static void check_iterators_eq(IteratorType1 iterator_1, IteratorType2 iterator_2)
{
    bool eq = iterator_1 == iterator_2;
    BOOST_TEST(eq);
}

template <typename IteratorType1, typename IteratorType2>
static void check_iterators_neq(IteratorType1 iterator_1, IteratorType2 iterator_2)
{
    bool neq = iterator_1 != iterator_2;
    BOOST_TEST(neq);
}

BOOST_AUTO_TEST_SUITE(rows_iterator)

BOOST_AUTO_TEST_CASE(matrix_init)
{
    matrix<int> matrix{3, 3};
    matrix_tester::check_matrix_empty(matrix);
}

BOOST_AUTO_TEST_CASE(matrix_rows_iterator_init)
{
    matrix<int> matrix{3, 3};
    auto rows_iterator = matrix.rows();
    check_iterators_neq(rows_iterator.begin(), rows_iterator.end());
}

BOOST_AUTO_TEST_CASE(matrix_rows_iterator_simple_increment)
{
    matrix<int> matrix{3, 3};
    auto rows_iterator = matrix.rows().begin();
    ++rows_iterator;
    check_iterators_neq(matrix.rows().begin(), rows_iterator);
}

BOOST_AUTO_TEST_CASE(matrix_rows_iterator_at_end)
{
    matrix<int> matrix{1, 4};
    auto rows_iterator = matrix.rows().begin();
    ++rows_iterator;
    check_iterators_eq(rows_iterator, matrix.rows().end());
}

BOOST_AUTO_TEST_CASE(matrix_rows_iterator_dereference)
{
    matrix<int> matrix{3, 3};
    auto first_row_iterator = *matrix.rows().begin();
    *first_row_iterator = 42;
    matrix_tester::check_matrix_element(matrix, 0, 0, 42);
}

BOOST_AUTO_TEST_CASE(matrix_rows_iterator_dereference_after_increment)
{
    matrix<int> matrix{3, 3};
    auto row_iterator = matrix.rows().begin();
    ++row_iterator;
    auto row_element_iterator = *row_iterator;
    ++row_element_iterator;
    *row_element_iterator = 42;
    matrix_tester::check_matrix_element(matrix, 1, 1, 42);
}

BOOST_AUTO_TEST_CASE(count_iterations_over_rows)
{
    matrix<int> m{3, 3};
    size_t iterations_over_rows = 0;

    for (auto rows_iterator = m.rows().begin(); rows_iterator != m.rows().end(); ++rows_iterator) {
        iterations_over_rows++;
    }

    BOOST_TEST(iterations_over_rows == m.get_row_size());
}

BOOST_AUTO_TEST_CASE(count_iterations_over_row_element)
{
    matrix<int> m{3, 3};
    size_t iterations_over_row_elements = 0;

    auto row_element_it = *m.rows().begin();
    for (auto elem_it = row_element_it.begin(); elem_it != row_element_it.end(); ++elem_it) {
        iterations_over_row_elements++;
    }

    BOOST_TEST(iterations_over_row_elements == m.get_col_size());
}

BOOST_AUTO_TEST_CASE(assign_value_to_entire_row)
{
    matrix<int> m{3, 3};

    auto row_element_it = *m.rows().begin();
    for (auto elem_it = row_element_it.begin(); elem_it != row_element_it.end(); ++elem_it) {
        *elem_it = 42;
    }

    for (auto elem_it = row_element_it.begin(); elem_it != row_element_it.end(); ++elem_it) {
        BOOST_TEST(*elem_it == 42);
    }
}

BOOST_AUTO_TEST_CASE(assign_value_to_entire_matrix)
{
    using matrix_t = matrix<int>;

    matrix_t m{3, 3};
    int value = 1;
    for (auto rows_iterator = m.rows().begin(); rows_iterator != m.rows().end(); ++rows_iterator) {
        matrix_t::rows_t::reference row_element_iterator = *rows_iterator;
        for (auto element_ptr = row_element_iterator.begin(); element_ptr != row_element_iterator.end(); ++element_ptr) {
            matrix_t::rows_t::value_type::reference elem_ref = *element_ptr;
            elem_ref = value;
            value++;
        }
    }

    value = 1;
    for (auto rows_iterator = m.rows().begin(); rows_iterator != m.rows().end(); ++rows_iterator) {
        matrix_t::rows_t::reference row_element_iterator = *rows_iterator;
        for (auto elem_it = row_element_iterator.begin(); elem_it != row_element_iterator.end(); ++elem_it) {
            matrix_t::rows_t::value_type::reference elem_ref = *elem_it;
            BOOST_TEST(elem_ref == value);
            value++;
        }
    }
}

BOOST_AUTO_TEST_SUITE_END() // rows_iterator

