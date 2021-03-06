// Copyright (c) 2020 Matthew J. Smith and Overkit contributors
// License: MIT (http://opensource.org/licenses/MIT)

namespace ovk {

template <typename T, int N> constexpr OVK_FORCE_INLINE typename elem<T,N>::iterator begin(
  elem<T,N> &Elem) {
  return Elem.Begin();
}

template <typename T, int N> constexpr OVK_FORCE_INLINE typename elem<T,N>::const_iterator begin(
  const elem<T,N> &Elem) {
  return Elem.Begin();
}

template <typename T, int N> constexpr OVK_FORCE_INLINE typename elem<T,N>::iterator end(
  elem<T,N> &Elem) {
  return Elem.End();
}

template <typename T, int N> constexpr OVK_FORCE_INLINE typename elem<T,N>::const_iterator end(
  const elem<T,N> &Elem) {
  return Elem.End();
}

namespace elem_internal {
template <typename T, int N, std::size_t... Indices> constexpr OVK_FORCE_INLINE elem<T,N>
  MakeUniformElemHelper(core::index_sequence<Indices...>, T Value) {
  // Not sure if there is a nicer way to do this
  return {(&Value)[0*Indices]...};
}
}
template <typename T, int N> constexpr OVK_FORCE_INLINE elem<T,N> MakeUniformElem(T Value) {
  return elem_internal::MakeUniformElemHelper<T,N>(core::index_sequence_of_size<N>(), Value);
}

namespace elem_internal {
template <typename T, int N, std::size_t Index1, std::size_t Index2, std::size_t... RemainingIndices
  > constexpr OVK_FORCE_INLINE bool EqualityHelper(core::index_sequence<Index1, Index2,
  RemainingIndices...>, const elem<T,N> &Left, const elem<T,N> &Right) {
  return (Left(Index1) == Right(Index1)) && EqualityHelper(core::index_sequence<Index2,
    RemainingIndices...>(), Left, Right);
}
template <typename T, int N, std::size_t Index> constexpr OVK_FORCE_INLINE bool EqualityHelper(
  core::index_sequence<Index>, const elem<T,N> &Left, const elem<T,N> &Right) {
  return Left(Index) == Right(Index);
}
}
template <typename T, int N> constexpr OVK_FORCE_INLINE bool operator==(const elem<T,N> &Left,
  const elem<T,N> &Right) {
  return elem_internal::EqualityHelper(core::index_sequence_of_size<N>(), Left, Right);
}
template <typename T, int N> constexpr OVK_FORCE_INLINE bool operator!=(const elem<T,N> &Left,
  const elem<T,N> &Right) {
  return !(Left == Right);
}

namespace elem_internal {
template <typename T, int N, typename U1, typename U2, std::size_t... Indices> constexpr
  OVK_FORCE_INLINE elem<T,N> PlusHelper(core::index_sequence<Indices...>, const U1 &Left, const U2
  &Right) {
  return {T(Left[Indices])+T(Right[Indices])...};
}
}
template <typename T, int N, OVK_FUNCDEF_REQUIRES(core::IsScalar<T>())> constexpr OVK_FORCE_INLINE
  elem<T,N> operator+(const elem<T,N> &Left, const elem<T,N> &Right) {
  return elem_internal::PlusHelper<T, N>(core::index_sequence_of_size<N>(), Left, Right);
}
template <typename T, int N, typename U, OVK_FUNCDEF_REQUIRES(core::IsScalar<T>() &&
  !std::is_same<U, elem<T,N>>::value && elem_internal::ImplicitlyConvertibleToElem<U, T, N>())>
  constexpr OVK_FORCE_INLINE elem<T,N> operator+(const elem<T,N> &Left, const U &Right) {
  return elem_internal::PlusHelper<T, N>(core::index_sequence_of_size<N>(), Left, Right);
}
template <typename U, typename T, int N, OVK_FUNCDEF_REQUIRES(core::IsScalar<T>() &&
  !std::is_same<U, elem<T,N>>::value && elem_internal::ImplicitlyConvertibleToElem<U, T, N>())>
  constexpr OVK_FORCE_INLINE elem<T,N> operator+(const U &Left, const elem<T,N> &Right) {
  return elem_internal::PlusHelper<T, N>(core::index_sequence_of_size<N>(), Left, Right);
}

namespace elem_internal {
template <typename T, int N, typename U1, typename U2, std::size_t... Indices> constexpr
  OVK_FORCE_INLINE elem<T,N> MinusHelper(core::index_sequence<Indices...>, const U1 &Left, const U2
  &Right) {
  return {T(Left[Indices])-T(Right[Indices])...};
}
}
template <typename T, int N, OVK_FUNCDEF_REQUIRES(core::IsScalar<T>())> constexpr OVK_FORCE_INLINE
  elem<T,N> operator-(const elem<T,N> &Left, const elem<T,N> &Right) {
  return elem_internal::MinusHelper<T, N>(core::index_sequence_of_size<N>(), Left, Right);
}
template <typename T, int N, typename U, OVK_FUNCDEF_REQUIRES(core::IsScalar<T>() &&
  !std::is_same<U, elem<T,N>>::value && elem_internal::ImplicitlyConvertibleToElem<U, T, N>())>
  constexpr OVK_FORCE_INLINE elem<T,N> operator-(const elem<T,N> &Left, const U &Right) {
  return elem_internal::MinusHelper<T, N>(core::index_sequence_of_size<N>(), Left, Right);
}
template <typename U, typename T, int N, OVK_FUNCDEF_REQUIRES(core::IsScalar<T>() &&
  !std::is_same<U, elem<T,N>>::value && elem_internal::ImplicitlyConvertibleToElem<U, T, N>())>
  constexpr OVK_FORCE_INLINE elem<T,N> operator-(const U &Left, const elem<T,N> &Right) {
  return elem_internal::MinusHelper<T, N>(core::index_sequence_of_size<N>(), Left, Right);
}

namespace elem_internal {
template <typename T, int N, typename U1, typename U2, std::size_t... Indices> constexpr
  OVK_FORCE_INLINE elem<T,N> MinHelper(core::index_sequence<Indices...>, const U1 &Left, const U2
  &Right) {
  return {Min(T(Left[Indices]), T(Right[Indices]))...};
}
}
template <typename T, int N, OVK_FUNCDEF_REQUIRES(core::IsScalar<T>())> constexpr OVK_FORCE_INLINE
  elem<T,N> Min(const elem<T,N> &Left, const elem<T,N> &Right) {
  return elem_internal::MinHelper<T, N>(core::index_sequence_of_size<N>(), Left, Right);
}
template <typename T, int N, typename U, OVK_FUNCDEF_REQUIRES(core::IsScalar<T>() &&
  !std::is_same<U, elem<T,N>>::value && elem_internal::ImplicitlyConvertibleToElem<U, T, N>())>
  constexpr OVK_FORCE_INLINE elem<T,N> Min(const elem<T,N> &Left, const U &Right) {
  return elem_internal::MinHelper<T, N>(core::index_sequence_of_size<N>(), Left, Right);
}
template <typename U, typename T, int N, OVK_FUNCDEF_REQUIRES(core::IsScalar<T>() &&
  !std::is_same<U, elem<T,N>>::value && elem_internal::ImplicitlyConvertibleToElem<U, T, N>())>
  constexpr OVK_FORCE_INLINE elem<T,N> Min(const U &Left, const elem<T,N> &Right) {
  return elem_internal::MinHelper<T, N>(core::index_sequence_of_size<N>(), Left, Right);
}

namespace elem_internal {
template <typename T, int N, typename U1, typename U2, std::size_t... Indices> constexpr
  OVK_FORCE_INLINE elem<T,N> MaxHelper(core::index_sequence<Indices...>, const U1 &Left, const U2
  &Right) {
  return {Max(T(Left[Indices]), T(Right[Indices]))...};
}
}
template <typename T, int N, OVK_FUNCDEF_REQUIRES(core::IsScalar<T>())> constexpr OVK_FORCE_INLINE
  elem<T,N> Max(const elem<T,N> &Left, const elem<T,N> &Right) {
  return elem_internal::MaxHelper<T, N>(core::index_sequence_of_size<N>(), Left, Right);
}
template <typename T, int N, typename U, OVK_FUNCDEF_REQUIRES(core::IsScalar<T>() &&
  !std::is_same<U, elem<T,N>>::value && elem_internal::ImplicitlyConvertibleToElem<U, T, N>())>
  constexpr OVK_FORCE_INLINE elem<T,N> Max(const elem<T,N> &Left, const U &Right) {
  return elem_internal::MaxHelper<T, N>(core::index_sequence_of_size<N>(), Left, Right);
}
template <typename U, typename T, int N, OVK_FUNCDEF_REQUIRES(core::IsScalar<T>() &&
  !std::is_same<U, elem<T,N>>::value && elem_internal::ImplicitlyConvertibleToElem<U, T, N>())>
  constexpr OVK_FORCE_INLINE elem<T,N> Max(const U &Left, const elem<T,N> &Right) {
  return elem_internal::MaxHelper<T, N>(core::index_sequence_of_size<N>(), Left, Right);
}

namespace elem_internal {
template <typename T, int N1, int N2, std::size_t... Indices1, std::size_t... Indices2> constexpr
  OVK_FORCE_INLINE elem<T,N1+N2> ConcatHelper(core::index_sequence<Indices1...>,
  core::index_sequence<Indices2...>, const elem<T,N1> &Left, const elem<T,N2> &Right) {
  return {Left(Indices1)..., Right(Indices2)...};
}
}
template <typename T, int N1, int N2> constexpr OVK_FORCE_INLINE elem<T,N1+N2> ConcatElems(const
  elem<T,N1> &Left, const elem<T,N2> &Right) {
  return elem_internal::ConcatHelper(core::index_sequence_of_size<N1>(),
    core::index_sequence_of_size<N2>(), Left, Right);
}

}
