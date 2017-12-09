#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
////////////////////////// TypeList的一个minimal实现 /////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace meta {

template <typename T, T ...s>
struct Sequence {
  using value_type = T;

  template <template <typename ...> class Host>
  using bind_to = Host<std::integral_constant<T, s>...>;

  template <typename CollectionT>
  inline static CollectionT Instantiate() {
    return { s... };
  }
};

template <typename ...Ts>
struct TypeList;

namespace internal {

using EmptyList = TypeList<>;

// ---------------------------------------------------------------------------
// TypeList meta functions.

// TypeList::at
template <typename TL, typename PosTL, typename Enable = void>
struct ElementAtImpl;

template <typename TL, typename PosTL>
struct ElementAtImpl<TL, PosTL, std::enable_if_t<PosTL::length == 0>> {
  using type = TL;
};

// TypeList::append
template <typename TL, typename UL>
struct AppendImpl;

template <typename ...Ts, typename ...Us>
struct AppendImpl<TypeList<Ts...>, TypeList<Us...>> {
  using type = TypeList<Ts..., Us...>;
};

template <typename TL, typename PosTL>
struct ElementAtImpl<TL, PosTL, std::enable_if_t<PosTL::length != 0>>
    : ElementAtImpl<typename std::tuple_element<
                        PosTL::head::value,
                        typename TL::template bind_to<std::tuple>>::type,
                    typename PosTL::tail> {};

// TypeList::filter
template <typename Out, typename Rest, template <typename ...> class Op,
          typename Enable = void>
struct FilterImpl;

template <typename Out, typename Rest, template <typename ...> class Op>
struct FilterImpl<Out, Rest, Op, std::enable_if_t<Rest::length == 0>> {
  using type = Out;
};

template <typename Out, typename Rest, template <typename ...> class Op>
struct FilterImpl<Out, Rest, Op, std::enable_if_t<Op<typename Rest::head>::value>>
    : FilterImpl<typename Out::template push_back<typename Rest::head>,
                 typename Rest::tail, Op> {};

template <typename Out, typename Rest, template <typename ...> class Op>
struct FilterImpl<Out, Rest, Op, std::enable_if_t<!Op<typename Rest::head>::value>>
    : FilterImpl<Out, typename Rest::tail, Op> {};

// TypeList::foldl
template <typename Out, typename Rest, template <typename ...> class Op,
          typename Enable = void>
struct FoldlImpl;

template <typename Out, typename Rest, template <typename ...> class Op>
struct FoldlImpl<Out, Rest, Op, std::enable_if_t<Rest::length == 0>> {
  using type = Out;
};

template <typename Out, typename Rest, template <typename ...> class Op>
struct FoldlImpl<Out, Rest, Op, std::enable_if_t<Rest::length != 0>>
    : FoldlImpl<typename Op<Out, typename Rest::head>::type,
                typename Rest::tail, Op> {};

// TypeList::get
template <typename TL, typename T, typename Enable = void>
struct GetValueForKeyImpl {
  static_assert(TL::length != 0, "TypeObject does not contain the required key.");
};

template <typename TL, typename T>
struct GetValueForKeyImpl<
    TL, T, std::enable_if_t<TL::length != 0 &&
                            std::is_same<typename TL::head::head, T>::value>> {
  using type = typename TL::head::tail::head;
};

template <typename TL, typename T>
struct GetValueForKeyImpl<
    TL, T, std::enable_if_t<TL::length != 0 &&
                            !std::is_same<typename TL::head::head, T>::value>>
    : GetValueForKeyImpl<typename TL::tail, T> {};

// TypeList::reverse
template <typename Out, typename Rest, typename Enable = void>
struct ReverseImpl;

template <typename Out, typename Rest>
struct ReverseImpl<Out, Rest, std::enable_if_t<Rest::length == 0>> {
  using type = Out;
};

template <typename Out, typename Rest>
struct ReverseImpl<Out, Rest, std::enable_if_t<Rest::length != 0>>
    : ReverseImpl<typename Out::template push_front<typename Rest::head>,
                  typename Rest::tail> {};

// TypeList::zip
template <typename Out, typename RestL, typename RestR, typename Enable = void>
struct ZipImpl;

template <typename Out, typename RestL, typename RestR>
struct ZipImpl<Out, RestL, RestR,
               std::enable_if_t<RestL::length == 0 || RestR::length == 0>> {
  static_assert(RestL::length == 0 && RestR::length == 0,
                "Zip failed: TypeLists have unequal lengths");
  using type = Out;
};

template <typename Out, typename RestL, typename RestR>
struct ZipImpl<Out, RestL, RestR,
               std::enable_if_t<RestL::length != 0 && RestR::length != 0>>
    : ZipImpl<typename Out::template push_back<
                  TypeList<typename RestL::head, typename RestR::head>>,
              typename RestL::tail, typename RestR::tail> {};

// ---------------------------------------------------------------------------
// End of meta functions.

template <typename ...Ts>
struct TypeListBase {
  static constexpr std::size_t length = sizeof...(Ts);

  using type = TypeList<Ts...>;
  using self = type;

  // ---------------------------------------------------------------------------
  // Meta-methods.

  template <template <typename ...> class Host>
  using bind_to = Host<Ts...>;

  template <typename TL>
  using append = typename AppendImpl<self, TL>::type;

  template <std::size_t ...pos>
  using at = typename ElementAtImpl<
      self, TypeList<std::integral_constant<std::size_t, pos>...>>::type;

  template <typename T>
  using push_front = TypeList<T, Ts...>;

  template <typename T>
  using push_back = TypeList<Ts..., T>;

  template <typename Key>
  using get = typename GetValueForKeyImpl<self, Key>::type;

  template <template <typename ...> class Op>
  using map = TypeList<typename Op<Ts>::type...>;

  template <template <typename ...> class Op>
  using filter = typename FilterImpl<EmptyList, self, Op>::type;

  template <template <typename ...> class Op, typename InitT>
  using foldl = typename FoldlImpl<InitT, self, Op>::type;

  template <typename ...Dumb>
  using reverse = typename ReverseImpl<EmptyList, self, Dumb...>::type;

  template <typename TL>
  using zip = typename ZipImpl<EmptyList, self, TL>::type;
};

}  // namespace internal

template <typename T, typename ...Ts>
struct TypeList<T, Ts...> : public internal::TypeListBase<T, Ts...> {
 public:
  using head = T;
  using tail = TypeList<Ts...>;
};

template <>
struct TypeList<> : public internal::TypeListBase<> {};

}  // namespace meta


////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Roman to Integer ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

using RomanCharacters =
    meta::Sequence<char, 'I', 'V', 'X', 'L', 'C', 'D', 'M'>;
using RomanValues =
    meta::Sequence<int, 1, 5, 10, 50, 100, 500, 1000>;

using RomanLookupTable =
    RomanCharacters::bind_to<meta::TypeList>::zip<
        RomanValues::bind_to<meta::TypeList>>;

template <int first, int second>
using Pair = meta::TypeList<
    std::integral_constant<int, first>, std::integral_constant<int, second>>;

template <typename SU, typename C>
struct Combine {
  static constexpr int accu = SU::template at<0>::value;
  static constexpr int prev = SU::template at<1>::value;
  static constexpr int curr = RomanLookupTable::get<C>::value;
  static constexpr int next = curr < prev ? accu - curr : accu + curr;
  using type = Pair<next, curr>;
};

template <char ...characters>
struct RomanToInt {
  static int Get() {
    return meta::Sequence<char, characters...>
               ::template bind_to<meta::TypeList>
               ::template reverse<>
               ::template foldl<Combine, Pair<0, 0>>
               ::template at<0>
               ::value;
  }
};


int main(int argc, char *argv[]) {
  using R = RomanToInt<'D', 'C', 'X', 'X', 'I'>;
  std::cout << R::Get() << "\n";

  return 0;
}
