#ifndef ADIAR_TYPES_H
#define ADIAR_TYPES_H

#include <optional>
#include <utility>

namespace adiar
{
  //////////////////////////////////////////////////////////////////////////////
  /// \brief Possible values to assign a variable.
  //////////////////////////////////////////////////////////////////////////////
  enum class assignment : char
  {
    False = 0, // false
    True  = 1, // true
    None  = -1 // TODO: change into '2'?
  };

  //////////////////////////////////////////////////////////////////////////////
  /// \brief A pair of values.
  //////////////////////////////////////////////////////////////////////////////
  template<typename T1, typename T2>
  using pair = std::pair<T1, T2>;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Create an `adiar::pair`, deducing the target type based on the
  ///        types of the arguments.
  //////////////////////////////////////////////////////////////////////////////
  template<typename T1, typename T2>
  constexpr pair<T1, T2>
  make_pair(const T1 &t1, const T2 &t2)
  { return std::make_pair(t1, t2); }

  /*
  //////////////////////////////////////////////////////////////////////////////
  /// \brief Create an `adiar::pair`, deducing the target type based on the
  ///        types of the arguments.
  //////////////////////////////////////////////////////////////////////////////
  template<typename T1, typename T2>
  constexpr pair<T1, T2>
  make_pair(T1 &&t1, T2 &&t2)
  { return std::make_pair(std::move(t1), std::move(t2)); }
  */

  //////////////////////////////////////////////////////////////////////////////
  /// \brief An optional value, i.e. a possibly existent value.
  ///
  /// \details Not having a value is for example used to indicate the end of
  ///          streams and generators.
  //////////////////////////////////////////////////////////////////////////////
  template<typename T>
  using optional = std::optional<T>;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Create an empty `adiar::optional`, i.e. *None*.
  //////////////////////////////////////////////////////////////////////////////
  template<typename T>
  constexpr optional<T>
  make_optional()
  { return optional<T>(); }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Create an `adiar::optional` with *Some* value.
  //////////////////////////////////////////////////////////////////////////////
  template<typename T>
  constexpr optional<T>
  make_optional(const T &t)
  { return std::make_optional(t); }

  /*
  //////////////////////////////////////////////////////////////////////////////
  /// \brief Create an `adiar::optional` with *Some* value.
  //////////////////////////////////////////////////////////////////////////////
  template<typename T>
  constexpr optional<T>
  make_optional(T &&t)
  { return std::make_optional(std::move(t)); }
  */
}

#endif // ADIAR_TYPES_H
