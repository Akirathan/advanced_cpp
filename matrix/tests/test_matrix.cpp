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
    const auto rows_iterator_begin = matrix.rows().begin();
    auto rows_iterator = matrix.rows().begin();
    ++rows_iterator;
    check_iterators_neq(rows_iterator_begin, rows_iterator);
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

