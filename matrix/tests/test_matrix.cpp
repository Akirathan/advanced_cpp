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

    template <typename T>
    static T get_matrix_element(const matrix<T> &matrix, size_t i, size_t j)
    {
        return matrix.m_content[i][j];
    }
};

struct complex {
    int re = 0;
    int im = 0;

    bool operator==(const complex &other)
    {
        return re == other.re && im == other.im;
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

// ================================================================================== //
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

BOOST_AUTO_TEST_CASE(range_based_for_loop)
{
    matrix<int> m{3, 5};

    for (auto &&row : m.rows()) {
        for (auto &&row_element : row) {
            row_element = 42;
        }
    }

    for (auto &&row : m.rows()) {
        for (auto &&row_element : row) {
            BOOST_TEST(row_element == 42);
        }
    }
}

BOOST_AUTO_TEST_CASE(std_for_each)
{
    using matrix_t = matrix<int>;
    matrix_t m{4, 6};

    int value = 1;
    std::for_each(m.rows().begin(), m.rows().end(),
              [&](matrix_t::rows_t::reference row)
              {
                  std::for_each(row.begin(), row.end(),
                                [&](matrix_t::rows_t::value_type::reference el) {
                                    el = value;
                                    value++;
                                });
              });

    value = 1;
    std::for_each(m.rows().begin(), m.rows().end(),
                  [&](matrix_t::rows_t::reference row)
                  {
                      std::for_each(row.begin(), row.end(),
                                    [&](matrix_t::rows_t::value_type::reference el) {
                                        BOOST_TEST(el == value);
                                        value++;
                                    });
                  });
}

BOOST_AUTO_TEST_CASE(arrow_operator)
{
    matrix<complex> m{3, 6};
    auto rows_it = m.rows().begin();
    auto rows_elem_it = rows_it->begin();
    (++rows_elem_it)->re = 2;
    rows_elem_it->im = 3;

    complex should_match{2, 3};
    auto set_element = matrix_tester::get_matrix_element(m, 0, 1);
    bool elem_eq = set_element == should_match;
    BOOST_TEST(elem_eq);

    (++rows_it)->begin()->re = 5;
    set_element = matrix_tester::get_matrix_element(m, 1, 0);
    BOOST_TEST(set_element.re == 5);
}

BOOST_AUTO_TEST_CASE(postfix_plus)
{
    matrix<int> m{4, 7};
    auto it1 = m.cols()[0].begin();
    // it2 still points to [0, 0]
    auto it2 = it1++;
    *it2 = 5;

    matrix_tester::check_matrix_element(m, 0, 0, 5);
}

BOOST_AUTO_TEST_SUITE_END() // rows_iterator

// ================================================================================== //

BOOST_AUTO_TEST_SUITE(cols_iterator)

BOOST_AUTO_TEST_CASE(cols_iterator_init)
{
    matrix<int> matrix{3, 3};
    auto cols_iterator = matrix.cols();
    check_iterators_neq(cols_iterator.begin(), cols_iterator.end());
}

BOOST_AUTO_TEST_CASE(cols_iterator_simple_increment)
{
    matrix<int> matrix{3, 3};
    auto cols_iterator = matrix.cols().begin();
    ++cols_iterator;
    check_iterators_neq(matrix.cols().begin(), cols_iterator);
}

BOOST_AUTO_TEST_CASE(cols_iterator_at_end)
{
    matrix<int> matrix{4, 1};
    auto cols_iterator = matrix.cols().begin();
    ++cols_iterator;
    check_iterators_eq(cols_iterator, matrix.cols().end());
}

BOOST_AUTO_TEST_CASE(cols_iterator_dereference)
{
    matrix<int> matrix{3, 3};
    auto first_col_iterator = *matrix.cols().begin();
    *first_col_iterator = 42;
    matrix_tester::check_matrix_element(matrix, 0, 0, 42);
}

BOOST_AUTO_TEST_CASE(cols_iterator_dereference_after_increment)
{
    matrix<int> matrix{3, 3};
    auto cols_iterator = matrix.cols().begin();
    ++cols_iterator;
    auto col_element_iterator = *cols_iterator;
    ++col_element_iterator;
    *col_element_iterator = 42;
    matrix_tester::check_matrix_element(matrix, 1, 1, 42);
}

BOOST_AUTO_TEST_CASE(count_iterations_over_cols)
{
    matrix<int> m{2, 4};
    size_t iterations_over_cols = 0;

    for (auto cols_iterator = m.cols().begin(); cols_iterator != m.cols().end(); ++cols_iterator) {
        iterations_over_cols++;
    }

    BOOST_TEST(iterations_over_cols == m.get_col_size());
}

BOOST_AUTO_TEST_CASE(count_iterations_over_col_element)
{
    matrix<int> m{4, 5};
    size_t iterations_over_col_elements = 0;

    auto col_element_it = *m.cols().begin();
    for (auto elem_it = col_element_it.begin(); elem_it != col_element_it.end(); ++elem_it) {
        iterations_over_col_elements++;
    }

    BOOST_TEST(iterations_over_col_elements == m.get_row_size());
}

BOOST_AUTO_TEST_CASE(assign_value_to_entire_column)
{
    matrix<int> m{5, 2};

    auto col_element_it = *m.cols().begin();
    for (auto elem_it = col_element_it.begin(); elem_it != col_element_it.end(); ++elem_it) {
        *elem_it = 42;
    }

    for (auto elem_it = col_element_it.begin(); elem_it != col_element_it.end(); ++elem_it) {
        BOOST_TEST(*elem_it == 42);
    }
}

BOOST_AUTO_TEST_CASE(assign_value_to_entire_matrix)
{
    using matrix_t = matrix<int>;

    matrix_t m{4, 6};
    int value = 1;

    for (auto cols_iterator = m.cols().begin(); cols_iterator != m.cols().end(); ++cols_iterator) {
        matrix_t::cols_t::reference col_element_iterator = *cols_iterator;
        for (auto element_ptr = col_element_iterator.begin(); element_ptr != col_element_iterator.end(); ++element_ptr) {
            matrix_t::cols_t::value_type::reference elem_ref = *element_ptr;
            elem_ref = value;
            value++;
        }
    }

    value = 1;
    for (auto cols_iterator = m.cols().begin(); cols_iterator != m.cols().end(); ++cols_iterator) {
        matrix_t::cols_t::reference col_element_iterator = *cols_iterator;
        for (auto element_ptr = col_element_iterator.begin(); element_ptr != col_element_iterator.end(); ++element_ptr) {
            matrix_t::cols_t::value_type::reference elem_ref = *element_ptr;
            BOOST_TEST(elem_ref == value);
            value++;
        }
    }
}

BOOST_AUTO_TEST_CASE(range_based_for_loop)
{
    matrix<int> m{3, 5};

    for (auto &&col : m.cols()) {
        for (auto &&col_element : col) {
            col_element = 42;
        }
    }

    for (auto &&col : m.cols()) {
        for (auto &&col_element : col) {
            BOOST_TEST(col_element == 42);
        }
    }
}

BOOST_AUTO_TEST_CASE(std_for_each)
{
    using matrix_t = matrix<int>;
    matrix_t m{4, 6};

    int value = 1;
    std::for_each(m.cols().begin(), m.cols().end(),
                  [&](matrix_t::cols_t::reference col)
                  {
                      std::for_each(col.begin(), col.end(),
                                    [&](matrix_t::cols_t::value_type::reference el) {
                                        el = value;
                                        value++;
                                    });
                  });

    value = 1;
    std::for_each(m.cols().begin(), m.cols().end(),
                  [&](matrix_t::cols_t::reference col)
                  {
                      std::for_each(col.begin(), col.end(),
                                    [&](matrix_t::cols_t::value_type::reference el) {
                                        BOOST_TEST(el == value);
                                        value++;
                                    });
                  });
}

BOOST_AUTO_TEST_CASE(arrow_operator)
{
    matrix<complex> m{3, 6};
    auto cols_it = m.cols().begin();
    auto cols_elem_it = cols_it->begin();
    (++cols_elem_it)->re = 2;
    cols_elem_it->im = 3;

    complex should_match{2, 3};
    auto set_element = matrix_tester::get_matrix_element(m, 1, 0);
    bool elem_eq = set_element == should_match;
    BOOST_TEST(elem_eq);

    (++cols_it)->begin()->re = 5;
    set_element = matrix_tester::get_matrix_element(m, 0, 1);
    BOOST_TEST(set_element.re == 5);
}

BOOST_AUTO_TEST_SUITE_END() // cols_iterator

// ================================================================================== //

BOOST_AUTO_TEST_SUITE(both_iterators)

BOOST_AUTO_TEST_CASE(postfix_increment_operator)
{
    matrix<int> m{4, 6};

    auto cols_iter = m.cols().begin();
    cols_iter++;

    auto other_cols_iter = m.cols().begin();
    ++other_cols_iter;

    check_iterators_eq(cols_iter, other_cols_iter);


    auto rows_iter = m.rows().begin();
    rows_iter++;

    auto other_rows_iter = m.rows().begin();
    ++other_rows_iter;

    check_iterators_eq(rows_iter, other_rows_iter);
}

BOOST_AUTO_TEST_SUITE_END() // both_iterators

// ================================================================================== //
BOOST_AUTO_TEST_SUITE(matrix_general)

BOOST_AUTO_TEST_CASE(square_bracket_operator_one_element)
{
    matrix<int> m{4, 6};
    m[1][1] = 42;
    matrix_tester::check_matrix_element(m, 1, 1, 42);
}

BOOST_AUTO_TEST_CASE(square_bracket_operator_entire_matrix)
{
    const size_t rows = 5;
    const size_t cols = 6;
    matrix<int> m{rows, cols};

    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            m[i][j] = 42;
        }
    }

    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            matrix_tester::check_matrix_element(m, i, j, 42);
        }
    }
}

BOOST_AUTO_TEST_CASE(square_bracket_operator_on_rows)
{
    matrix<int> m{4, 6};
    m[3][2] = 42;

    BOOST_TEST(m[3][2] == m.rows()[3][2]);
    BOOST_TEST(m[3][2] == 42);
}

BOOST_AUTO_TEST_CASE(square_bracket_operator_on_cols)
{
    matrix<int> m{4, 6};
    m.cols()[3][2] = 42;

    BOOST_TEST(m[2][3] == m.cols()[3][2]);
    BOOST_TEST(m[2][3] == 42);
}

BOOST_AUTO_TEST_CASE(use_iterators_on_copied_matrix)
{
    using matrix_t = matrix<int>;
    matrix_t m1{3, 5};

    // Assign 42 to first row.
    auto first_row_begin_it = m1.rows()[0].begin();
    auto first_row_end_it = m1.rows()[0].end();
    std::for_each(first_row_begin_it, first_row_end_it, [](int &elem){
        elem = 42;
    });


    // Assign 42 to first column through m2 iteratos.
    matrix_t m2 = m1; // copy-initialization --> copy constructor is called
    auto first_col_begin_it = m2.cols()[0].begin();
    auto first_col_end_it = m2.cols()[0].end();
    std::for_each(first_col_begin_it, first_col_end_it, [](int &elem) {
        elem = 42;
    });


    // Check first row of m2.
    for (size_t j = 0; j < m2.get_col_size(); ++j) {
        matrix_tester::check_matrix_element(m2, 0, j, 42);
    }
    // Check first column of m2.
    for (size_t i = 0; i < m2.get_row_size(); ++i) {
        matrix_tester::check_matrix_element(m2, i, 0, 42);
    }
}

BOOST_AUTO_TEST_CASE(iterators_of_copied_matrix_are_not_equal_to_original_iteratos)
{
    matrix<int> m1{4, 6};
    matrix<int> m2 = m1;
    
    check_iterators_neq(m1.rows().begin(), m2.rows().begin());
    check_iterators_neq(m1.cols().begin(), m2.cols().begin());
}

BOOST_AUTO_TEST_SUITE_END() // matrix_general

