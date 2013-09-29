#include <ratio>
#include <type_traits>

namespace units {

namespace reduced {
	template<typename, int> struct power {};
	template<typename...> struct multiply {};
	template<typename, intmax_t n = 1, intmax_t d = 1> struct scale {};
};

namespace internal {
	template<typename... As>
	struct concat2_ {
		using type = reduced::multiply<As...>;
	};
	template<typename A>
	struct concat2_<A> {
		using type = A;
	};
	template<typename A, typename B>
	struct concat_ {
		using type = reduced::multiply<A, B>;
	};
	template<typename... As, typename... Bs>
	struct concat_<reduced::multiply<As...>, reduced::multiply<Bs...>> {
		using type = typename concat2_<As..., Bs...>::type;
	};
	template<typename... As, typename B>
	struct concat_<reduced::multiply<As...>, B> {
		using type = typename concat_<reduced::multiply<As...>, reduced::multiply<B>>::type;
	};
	template<typename A, typename... Bs>
	struct concat_<A, reduced::multiply<Bs...>> {
		using type = typename concat_<reduced::multiply<A>, reduced::multiply<Bs...>>::type;
	};
	template<typename A, typename B> using concat = typename concat_<A, B>::type;
};

// ratio_power, because std:: doesn't provide that.

template<typename R, int p>
struct ratio_power {
private:
	using S = typename std::ratio_multiply<R, R>::type;
public:
	using type = typename std::ratio_multiply<
		typename ratio_power<R, p%2>::type,
		typename ratio_power<S, p/2>::type
	>::type;
};
template<typename R>
struct ratio_power<R, 0> {
	using type = std::ratio<1>;
};
template<typename R>
struct ratio_power<R, 1> {
	using type = R;
};
template<typename R>
struct ratio_power<R, -1> {
	using type = typename std::ratio_divide<std::ratio<1>, R>::type;
};

// scale

template<typename T, intmax_t n, intmax_t d>
struct scale_ {
private:
	using s = std::ratio<n, d>;
public:
	using type = typename std::conditional<
		std::ratio_equal<s, std::ratio<1>>::value,
		T,
		reduced::scale<T, s::num, s::den>
	>::type;
};

template<typename T, intmax_t n1, intmax_t d1, intmax_t n2, intmax_t d2>
struct scale_<reduced::scale<T, n2, d2>, n1, d1> {
private:
	using s = std::ratio_multiply<std::ratio<n1, d1>, std::ratio<n2, d2>>;
public:
	using type = typename scale_<T, s::num, s::den>::type;
};

template<typename T, intmax_t n = 1, intmax_t d = 1>
using scale = typename scale_<T, n, d>::type;

// get_scale

template<typename T>
struct get_scale {
	using base = T;
	using scale = std::ratio<1>;
};

template<typename T, intmax_t n, intmax_t d>
struct get_scale<reduced::scale<T, n, d>> {
	using base = T;
	using scale = std::ratio<n, d>;
};

// power

template<typename T, int o>
struct power_ {
	using type = typename std::conditional<
		o == 0,
		reduced::multiply<>,
		typename std::conditional<
			o == 1,
			T,
			reduced::power<T, o>
		>::type
	>::type;
};

template<typename T, int o1, int o2>
struct power_<reduced::power<T, o1>, o2> {
	using type = typename power_<T, o1*o2>::type;
};

template<intmax_t n, intmax_t d, typename T, int o>
struct power_<reduced::scale<T, n, d>, o> {
private:
	using s = typename ratio_power<std::ratio<n, d>, o>::type;
public:
	using type = scale<typename power_<T, o>::type, s::num, s::den>;
};

template<typename T, int o> using power = typename power_<T, o>::type;

template<typename T> using inverse = power<T, -1>;

// get_power

template<typename T>
struct get_power {
	using base = T;
	static constexpr int order = 1;
};

template<typename T, int o>
struct get_power<reduced::power<T, o>> {
	using base = T;
	static constexpr int order = o;
};

// count_power

template<typename, typename> struct count_power;

template<typename T>
struct count_power<T, reduced::multiply<>> {
	static constexpr int value = 0;
	using rest = reduced::multiply<>;
};

template<typename T, typename... As, typename... Bs>
struct count_power<T, reduced::multiply<reduced::multiply<As...>, Bs...>> {
private:
	using X = count_power<T, reduced::multiply<As..., Bs...>>;
public:
	static constexpr int value = X::value;
	using rest = typename X::rest;
};

template<typename T, typename A, typename... As>
struct count_power<T, reduced::multiply<A, As...>> {
private:
	using count_powerrest = count_power<T, reduced::multiply<As...>>;
public:
	static constexpr int value = count_powerrest::value;
	using rest = internal::concat<A, typename count_powerrest::rest>;
};

template<typename T, typename... As>
struct count_power<T, reduced::multiply<T, As...>> {
private:
	using count_powerrest = count_power<T, reduced::multiply<As...>>;
public:
	static constexpr int value = 1 + count_powerrest::value;
	using rest = typename count_powerrest::rest;
};

template<typename T, int o, typename... As>
struct count_power<T, reduced::multiply<reduced::power<T, o>, As...>> {
private:
	using count_powerrest = count_power<T, reduced::multiply<As...>>;
public:
	static constexpr int value = o + count_powerrest::value;
	using rest = typename count_powerrest::rest;
};

// multiply

template<typename... T>
struct multiply2_ {
	using type = reduced::multiply<T...>;
};

template<typename... As, typename... Bs>
struct multiply2_<reduced::multiply<As...>, Bs...> {
	using type = typename multiply2_<As..., Bs...>::type;
};

template<typename... As>
struct multiply2_<reduced::multiply<As...>> {
	using type = typename multiply2_<As...>::type;
};

template<typename A>
struct multiply2_<A> {
	using type = A;
};

template<typename A, typename... As>
struct multiply2_<A, As...> {
private:
	using P = typename get_power<A>::base;
	using X = count_power<P, reduced::multiply<A, As...>>;
public:
	using type = internal::concat<power<P, X::value>, typename multiply2_<typename X::rest>::type>;
};

template<typename... As>
struct multiply_ {
	using type = typename multiply2_<As...>::type;
};

template<typename T, intmax_t n, intmax_t d>
struct multiply_<reduced::scale<T, n, d>> {
	using type = scale<typename multiply2_<T>::type, n, d>;
};

template<
	typename T1, intmax_t n1, intmax_t d1,
	typename T2, intmax_t n2, intmax_t d2,
	typename... As
>
struct multiply_<reduced::scale<T1, n1, d1>, reduced::scale<T2, n2, d2>, As...> {
private:
	using s = std::ratio_multiply<std::ratio<n1, d1>, std::ratio<n2, d2>>;
public:
	using type = typename multiply_<reduced::scale<internal::concat<T1, T2>, s::num, s::den>, As...>::type;
};

template<
	typename T, intmax_t n, intmax_t d,
	typename A, typename... As
>
struct multiply_<reduced::scale<T, n, d>, A, As...> {
	using type = typename multiply_<reduced::scale<internal::concat<T, A>, n, d>, As...>::type;
};

template<typename A, typename... As>
struct multiply_<A, As...> {
	using type = typename multiply_<reduced::scale<A>, As...>::type;
};

template<typename... T> using multiply = typename multiply_<T...>::type;

// divide

template<typename A, typename B> using divide = multiply<A, inverse<B>>;

// power<multiply<..>, ..>

template<typename... As, int o>
struct power_<reduced::multiply<As...>, o> {
	using type = multiply<power<As, o>...>;
};

// is_equal

template<typename A, typename B> using is_equal = std::is_same<divide<A, B>, multiply<>>;

using unit = multiply<>;

namespace si {
	struct kilogram {};
	struct metre    {};
	struct second   {};
	struct ampere   {};
	struct mole     {};
	struct kelvin   {};
	struct candela  {};

	using square_metre            = multiply < metre            , metre                   >;
	using cubic_metre             = multiply < square_metre     , metre                   >;
	using metre_per_second        = divide   < metre            , second                  >;
	using metre_per_square_second = divide   < metre_per_second , second                  >;
	using newton                  = multiply < kilogram         , metre_per_square_second >;
	using joule                   = multiply < newton           , metre                   >;
	using pascal                  = divide   < newton           , square_metre            >;
	using gray                    = divide   < joule            , kilogram                >;
	using watt                    = divide   < joule            , second                  >;
	using hertz                   = divide   < unit             , second                  >;
	using katal                   = divide   < mole             , second                  >;
	using volt                    = divide   < watt             , ampere                  >;
	using weber                   = multiply < volt             , second                  >;
	using henry                   = divide   < weber            , ampere                  >;
	using tesla                   = divide   < weber            , square_metre            >;
	using coulomb                 = multiply < ampere           , second                  >;
	using farad                   = divide   < coulomb          , volt                    >;
	using ohm                     = divide   < volt             , ampere                  >;
	using siemens                 = divide   < unit             , ohm                     >;
	using radian                  = divide   < metre            , metre                   >;
	using steradian               = divide   < square_metre     , square_metre            >;
	using lumen                   = multiply < candela          , steradian               >;
	using lux                     = divide   < lumen            , square_metre            >;
}

template<typename T> using deca  = scale<T, 10l>;
template<typename T> using hecto = scale<T, 100l>;
template<typename T> using kilo  = scale<T, 1000l>;
template<typename T> using mega  = scale<T, 1000000l>;
template<typename T> using giga  = scale<T, 1000000000l>;
template<typename T> using tera  = scale<T, 1000000000000l>;
template<typename T> using peta  = scale<T, 1000000000000000l>;
template<typename T> using exa   = scale<T, 1000000000000000000l>;

template<typename T> using deci  = scale<T, 1l, 10l>;
template<typename T> using centi = scale<T, 1l, 100l>;
template<typename T> using milli = scale<T, 1l, 1000l>;
template<typename T> using micro = scale<T, 1l, 1000000l>;
template<typename T> using nano  = scale<T, 1l, 1000000000l>;
template<typename T> using pico  = scale<T, 1l, 1000000000000l>;
template<typename T> using femto = scale<T, 1l, 1000000000000000l>;
template<typename T> using atto  = scale<T, 1l, 1000000000000000000l>;

template<typename T> using kibi  = scale<T, 1024l>;
template<typename T> using mebi  = scale<T, 1024l*1024l>;
template<typename T> using gibi  = scale<T, 1024l*1024l*1024l>;
template<typename T> using tebi  = scale<T, 1024l*1024l*1024l*1024l>;
template<typename T> using pebi  = scale<T, 1024l*1024l*1024l*1024l*1024l>;
template<typename T> using exbi  = scale<T, 1024l*1024l*1024l*1024l*1024l*1024l>;

namespace imperial {
	using yard          = scale<si::metre, 9144, 10000>;
	using foot          = scale<yard, 1, 3>;
	using inch          = scale<foot, 1, 12>;
	using line          = scale<inch, 1, 12>;
	using pica          = scale<inch, 1, 6>;
	using point         = scale<pica, 1, 12>;
	using thou          = scale<inch, 1, 1000>;
	using chain         = scale<yard, 22>;
	using furlong       = scale<chain, 10>;
	using mile          = scale<furlong, 8>;
	using league        = scale<mile, 3>;
	using rod           = scale<chain, 1, 4>;
	using link          = scale<rod, 1, 25>;
	using perch         = power<rod, 2>;
	using rood          = multiply<furlong, rod>;
	using acre          = multiply<furlong, chain>;
	using pound         = scale<si::kilogram, 45359237, 100000000>;
	using ounce         = scale<pound, 1, 16>;
	using drachm        = scale<pound, 1, 256>;
	using grain         = scale<pound, 1, 7000>;
	using stone         = scale<pound, 14>;
	using quarter       = scale<pound, 28>;
	using hundredweight = scale<pound, 112>;
	using ton           = scale<pound, 2240>;
	// TODO: fluid ounce, gill, pint, quart, gallon
}

namespace us {
	using imperial::inch;
	using imperial::foot;
	using imperial::yard;
	using imperial::mile;
	using imperial::pica;
	using imperial::point;
	using imperial::link;
	using imperial::rod;
	using imperial::chain;
	using imperial::furlong;
	using imperial::league;
	using fathom = scale<yard, 2>;
	using cable = scale<fathom, 120>;
	using nautical_mile = scale<si::metre, 1852>;
}

/* TODO:
template<typename T, typename U>
struct quantity {
public:
	using value_type = T;
	using unit = U;
	value_type value;
	explicit quantity(value_type v) : value(v) {}

	// ...

};
*/

}
