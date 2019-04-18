#include "du2matrix.hpp"

#include <iostream>
#include <algorithm>

using my_matrix = matrix< int>;

int cnt = 100;

void f1( my_matrix::cols_t::value_type::reference x)
{
    x = ++cnt;
}

void f2( my_matrix::cols_t::reference r)
{
    std::for_each( r.begin(), r.end(), f1);
}

void f3( my_matrix::rows_t::value_type::reference x)
{
    std::cout << x << " ";
}

void f4( my_matrix::rows_t::reference r)
{
    std::for_each( r.begin(), r.end(), f3);
    std::cout << std::endl;
}


struct Complex
{
    double re = 0;
    double im = 0;
};

int main( int, char * *)
{
    try {
		my_matrix a( 3, 4, 0);  // matice 3 radky * 4 sloupce inicializovana nulami
		matrix b{ a };
		matrix c( 10, 20, 1);
		c = a;

		std::for_each( b.cols().begin(), b.cols().end(), f2);

		c = b;
		c[0][2] = b[1][1];

		// std::for_each( c.rows().begin(), c.rows().end(), f4);
		for (auto&& r : c.rows())
			f4( r);

		// postfix ++
		auto it1 = c.cols()[0].begin();
		auto it2 = it1++;
		*it2 = -99;

		// iterators
		std::for_each(c.rows().begin(), c.rows().end(),
			  [](my_matrix::rows_t::reference row)
			  {
				  std::for_each(row.begin(), row.end(),
					  [](my_matrix::rows_t::value_type::reference el) {
						  std::cout << el << ' ';
					  });
				  std::cout << '\n';
			  });

		// prefix ++, ->
		matrix cc(6, 5, Complex{});
		auto it3 = cc.rows().begin();
		auto it4 = it3->begin();
		(++it4)->re = 4;
		(++it3)->begin()->im = 7;

		// range-base for
		for (auto&& col : cc.cols())
		{
			  for (auto&& el : col)
				  std::cout << el.re << "+" << el.im << "i ";
			  std::cout << '\n';
		}
    }
    catch ( const std::exception & e) {
		std::cout << e.what() << std::endl;
    }

    return 0;
}
