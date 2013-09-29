#include <iostream>

#include "units.hpp"

using namespace units;
using namespace si;

int main() {
	static_assert(is_equal<mega<volt>, divide<watt, micro<ampere>>>::value, "Error!");
	// int x = kilo<volt>(); // Uncomment go get a fun error describing the reduced type of kilovolt.
	// (It is scale<multiply<kilogram, power<metre, 2>, power<second, -3>, power<ampere, -1>>, 1000, 1>.)
}

