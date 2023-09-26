#include "zdd.h"

#include <adiar/zdd.h>

#include <memory>

#include <adiar/domain.h>
#include <adiar/internal/io/file_stream.h>
#include <adiar/internal/dot.h>
#include <adiar/internal/algorithms/convert.h>
#include <adiar/internal/algorithms/intercut.h>
#include <adiar/internal/algorithms/reduce.h>
#include <adiar/internal/data_types/arc.h>
#include <adiar/internal/data_types/node.h>
#include <adiar/bdd/bdd_policy.h>
#include <adiar/zdd/zdd_policy.h>

namespace adiar
{
  //////////////////////////////////////////////////////////////////////////////
  // '__zdd' Constructors
  __zdd::__zdd()
    : internal::__dd()
  { }

  __zdd::__zdd(const internal::__dd::shared_nodes_t &f)
    : internal::__dd(f)
  { }

  __zdd::__zdd(const internal::__dd::shared_arcs_t &f)
    : internal::__dd(f)
  { }

  __zdd::__zdd(const zdd &dd)
    : internal::__dd(dd)
  { }

  //////////////////////////////////////////////////////////////////////////////
  // 'zdd' Constructors
  zdd::zdd(const internal::dd::shared_nodes_t &f, bool negate)
    : internal::dd(f, negate)
  { }

  zdd::zdd()
    : zdd(zdd_empty())
  { }

  zdd::zdd(bool v)
    : zdd(zdd_terminal(v))
  { }

  zdd::zdd(const zdd &o)
    : internal::dd(o)
  { }

  zdd::zdd(zdd &&o)
    : internal::dd(o)
  { }

  zdd::zdd(__zdd &&o)
    : internal::dd(internal::reduce<zdd_policy>(std::forward<__zdd>(o)))
  { }

  //////////////////////////////////////////////////////////////////////////////
  // Operators
#define __zdd_oper(out_t, op)                                              \
  out_t operator op (__zdd &&lhs, __zdd &&rhs) {                           \
    return zdd(std::forward<__zdd>(lhs)) op zdd(std::forward<__zdd>(rhs)); \
  }                                                                        \
                                                                           \
  out_t operator op (const zdd &lhs, __zdd &&rhs) {                        \
    return lhs op zdd(std::forward<__zdd>(rhs));                           \
  }                                                                        \
                                                                           \
  out_t operator op (__zdd &&lhs, const zdd &rhs) {                        \
    return zdd(std::forward<__zdd>(lhs)) op rhs;                           \
  }

  __zdd_oper(__zdd, &)
  __zdd_oper(__zdd, |)
  __zdd_oper(__zdd, -)

  __zdd_oper(bool, ==)
  __zdd_oper(bool, !=)
  __zdd_oper(bool, <=)
  __zdd_oper(bool, >=)
  __zdd_oper(bool, <)
  __zdd_oper(bool, >)

  zdd& zdd::operator= (const zdd &other)
  {
    this -> negate = other.negate;
    this -> file = other.file;
    return *this;
  }

  zdd& zdd::operator= (__zdd &&other)
  {
    deref();
    return (*this = internal::reduce<zdd_policy>(std::forward<__zdd>(other)));
  }

  __zdd operator~ (const zdd &A)
  {
    return zdd_complement(A);
  }

  __zdd operator~ (__zdd &&A)
  {
    return ~zdd(std::forward<__zdd>(A));
  }

  __zdd operator& (const zdd& lhs, const zdd& rhs)
  {
    return zdd_intsec(lhs, rhs);
  }

  zdd& zdd::operator&= (const zdd &other)
  {
    return (*this = zdd_intsec(*this, other));
  }

  zdd& zdd::operator&= (zdd &&other)
  {
    __zdd&& temp = zdd_intsec(*this, other);
    other.deref();
    return (*this = std::move(temp));
  }

  __zdd operator| (const zdd& lhs, const zdd& rhs)
  {
    return zdd_union(lhs, rhs);
  }

  zdd& zdd::operator|= (const zdd &other)
  {
    return (*this = zdd_union(*this, other));
  }

  zdd& zdd::operator|= (zdd &&other)
  {
    __zdd&& temp = zdd_union(*this, other);
    other.deref();
    return (*this = std::move(temp));
  }

  __zdd operator- (const zdd& lhs, const zdd& rhs)
  {
    return zdd_diff(lhs, rhs);
  }

  zdd& zdd::operator-= (const zdd &other)
  {
    return (*this = zdd_diff(*this, other));
  }

  zdd& zdd::operator-= (zdd &&other)
  {
    __zdd&& temp = zdd_diff(*this, other);
    other.deref();
    return (*this = std::move(temp));
  }

  bool operator== (const zdd& lhs, const zdd& rhs)
  {
    return zdd_equal(lhs, rhs);
  }

  bool operator!= (const zdd& lhs, const zdd& rhs) {
    return zdd_unequal(lhs, rhs);
  }

  bool operator<= (const zdd& lhs, const zdd& rhs)
  {
    return zdd_subseteq(lhs, rhs);
  }

  bool operator>= (const zdd& lhs, const zdd& rhs) {
    return zdd_subseteq(rhs, lhs);
  }

  bool operator< (const zdd& lhs, const zdd& rhs)
  {
    return zdd_subset(lhs, rhs);
  }

  bool operator> (const zdd& lhs, const zdd& rhs) {
    return zdd_subset(rhs, lhs);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Input variables
  zdd::label_t zdd_minvar(const zdd &A)
  {
    return internal::dd_minvar(A);
  }

  zdd::label_t zdd_maxvar(const zdd &A)
  {
    return internal::dd_maxvar(A);
  }

  void zdd_varprofile(const zdd &A, const consumer<zdd::label_t> &cb)
  {
    return internal::dd_varprofile(A, cb);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Conversion
  __zdd zdd_from(const bdd &f, const generator<zdd::label_t> &dom)
  {
    return internal::intercut<internal::convert_dd_policy<zdd_policy, bdd_policy>>
      (f, dom);
  }

  __zdd zdd_from(const bdd &f)
  {
    const shared_file<zdd::label_t> dom = adiar_get_domain();
    internal::file_stream<domain_var_t> ds(dom);

    return internal::intercut<internal::convert_dd_policy<zdd_policy, bdd_policy>>
      (f, make_generator(ds));
  }

  //////////////////////////////////////////////////////////////////////////////
  // Debug
  void zdd_printdot(const zdd &A, std::ostream &out)
  {
    internal::print_dot<zdd>(A, out);
  }

  void zdd_printdot(const zdd &A, const std::string &file_name)
  {
    internal::print_dot<zdd>(A, file_name);
  }
}
