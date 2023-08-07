#ifndef STAN_MATH_FWD_FUNCTOR_FVAR_FINITE_DIFF_HPP
#define STAN_MATH_FWD_FUNCTOR_FVAR_FINITE_DIFF_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/functor/apply.hpp>
#include <stan/math/prim/functor/finite_diff_gradient_auto.hpp>
#include <stan/math/prim/fun/value_of.hpp>
#include <stan/math/prim/fun/sum.hpp>
#include <stan/math/prim/fun/elt_multiply.hpp>
#include <test/unit/math/serializer.hpp>

namespace stan {
namespace math {
namespace internal {
/**
 * Helper function for aggregating tangents if the respective input argument
 * was an fvar<T> type.
 *
 * Overload for when the input is not an fvar<T> and no tangents are needed.
 *
 * @tparam FuncTangentT Type of tangent calculated by finite-differences
 * @tparam InputArgT Type of the function input argument
 * @param tangent Calculated tangent
 * @param arg Input argument
 */
template <typename FuncTangentT, typename InputArgT,
          require_not_st_fvar<InputArgT>* = nullptr>
inline constexpr double aggregate_tangent(const FuncTangentT& tangent,
                                          const InputArgT& arg) {
  return 0;
}

/**
 * Helper function for aggregating tangents if the respective input argument
 * was an fvar<T> type.
 *
 * Overload for when the input is an fvar<T> and its tangent needs to be
 * aggregated.
 *
 * @tparam FuncTangentT Type of tangent calculated by finite-differences
 * @tparam InputArgT Type of the function input argument
 * @param tangent Calculated tangent
 * @param arg Input argument
 */
template <typename FuncTangentT, typename InputArgT,
          require_st_fvar<InputArgT>* = nullptr>
auto aggregate_tangent(const FuncTangentT& tangent, const InputArgT& arg) {
  return sum(elt_multiply(tangent, arg.d()));
}
}  // namespace internal

/**
 * This frameworks adds fvar<T> support for arbitrary functions through
 * finite-differencing. Higher-order inputs (i.e., fvar<var> & fvar<fvar<T>>)
 * are also implicitly supported.
 *
 * @tparam F Type of functor for which fvar<T> support is needed
 * @tparam TArgs... Types of arguments (containing at least one fvar<T> type)
 *                     to be passed to function
 * @param func Functor for which fvar<T> support is needed
 * @param args... Parameter pack of arguments to be passed to functor.
 */
template <typename F, typename... TArgs,
          require_any_st_fvar<TArgs...>* = nullptr>
auto fvar_finite_diff(const F& func, const TArgs&... args) {
  using FvarT = return_type_t<TArgs...>;
  using FvarInnerT = typename FvarT::Scalar;

  auto val_args = std::make_tuple(stan::math::value_of(args)...);

  auto serialised_args = stan::math::apply(
      [&](auto&&... tuple_args) {
        return math::to_vector(
            stan::test::serialize<FvarInnerT>(tuple_args...));
      },
      val_args);

  // Create a 'wrapper' functor which will take the flattened column-vector
  // and transform it to individual arguments which are passed to the
  // user-provided functor
  auto serial_functor = [&](const auto& v) {
    auto ds = stan::test::to_deserializer(v);
    return stan::math::apply(
        [&](auto&&... tuple_args) { return func(ds.read(tuple_args)...); },
        val_args);
  };

  FvarInnerT rtn_value;
  Eigen::Matrix<FvarInnerT, -1, 1> grad;
  finite_diff_gradient_auto(serial_functor, serialised_args, rtn_value, grad);

  auto ds_grad = stan::test::to_deserializer(grad);
  FvarInnerT rtn_grad = 0;
  // Use a fold-expression to aggregate tangents for input arguments
  (void)std::initializer_list<int>{(
      rtn_grad += internal::aggregate_tangent(ds_grad.read(args), args), 0)...};

  return fvar<FvarInnerT>(rtn_value, rtn_grad);
}

/**
 * This frameworks adds fvar<T> support for arbitrary functions through
 * finite-differencing. Higher-order inputs (i.e., fvar<var> & fvar<fvar<T>>)
 * are also implicitly supported.
 *
 * Overload for use when no fvar<T> arguments are passed, and finite-differences
 * are not needed.
 *
 * @tparam F Type of functor
 * @tparam TArgs... Types of arguments (containing no fvar<T> types)
 * @param func Functor
 * @param args... Parameter pack of arguments to be passed to functor.
 */
template <typename F, typename... TArgs,
          require_all_not_st_fvar<TArgs...>* = nullptr>
auto fvar_finite_diff(const F& func, const TArgs&... args) {
  return func(args...);
}

}  // namespace math
}  // namespace stan

#endif
