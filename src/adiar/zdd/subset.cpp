#include <adiar/zdd.h>
#include <adiar/zdd/zdd_policy.h>

#include <adiar/internal/assert.h>
#include <adiar/internal/unreachable.h>
#include <adiar/internal/util.h>
#include <adiar/internal/algorithms/substitution.h>
#include <adiar/internal/io/file_stream.h>

namespace adiar
{
  template<assignment FIX_VALUE>
  class zdd_subset_labels
  {
    const std::function<zdd::label_t()> &gen;

    /// \brief The current level (including the current algorithm level)
    zdd::label_t l_incl = zdd::MAX_LABEL+1;

    /// \brief The next level (definitely excluding the current level)
    zdd::label_t l_excl = zdd::MAX_LABEL+1;

    /// We will rememeber how far the algorithm in substitution.h has got
    zdd::label_t alg_level = 0;

  public:
    /// We will remember whether any level of the input actually matched.
    bool l_match = false;

  public:
    zdd_subset_labels(const std::function<zdd::label_t()> &g)
      : gen(g)
    {
      l_incl = gen();
      if (l_incl <= zdd::MAX_LABEL) { l_excl = gen(); }
    }

  private:
    /// \brief Forwards through the input to the given level
    inline void forward_to_level(const zdd::label_t new_level)
    {
      adiar_assert(alg_level <= new_level,
                   "The algorithm should ask for the levels in increasing order.");

      alg_level = new_level;

      while (l_incl <= zdd::MAX_LABEL && l_incl < new_level) {
        l_incl = std::move(l_excl);
        if (l_incl <= zdd::MAX_LABEL) { l_excl = gen(); };
      }
    }

  public:
    /// \brief Obtain the assignment for the current level
    assignment assignment_for_level(const zdd::label_t new_level)
    {
      forward_to_level(new_level);

      const bool level_matches = l_incl == new_level;
      l_match |= level_matches;

      return level_matches ? FIX_VALUE : assignment::None;
    }

  public:
    /// \brief Whether the manager has a next level (including the current)
    bool has_level_incl()
    {
      return l_incl <= zdd::MAX_LABEL && alg_level <= l_incl;
    }

    /// \brief Get the current level (including the current algorithm level)
    zdd::label_t level_incl()
    {
      return l_incl;
    }

    /// \brief Whether the manager has a level ahead of the current
    bool has_level_excl()
    {
      return (l_incl <= zdd::MAX_LABEL && alg_level < l_incl) || l_excl <= zdd::MAX_LABEL;
    }

    /// \brief Get the next level (excluding the current one)
    zdd::label_t level_excl()
    {
      if (alg_level < l_incl) { return l_incl; }
      return l_excl;
    }
  };

  //////////////////////////////////////////////////////////////////////////////
  template<typename assignment_mgr>
  class zdd_offset_policy : public zdd_policy
  {
  public:
    static internal::substitute_rec keep_node(const zdd::node_t &n, assignment_mgr &/*amgr*/)
    { return internal::substitute_rec_output { n }; }

    static internal::substitute_rec fix_false(const zdd::node_t &n, assignment_mgr &/*amgr*/)
    { return internal::substitute_rec_skipto { n.low() }; }

    // LCOV_EXCL_START
    static internal::substitute_rec fix_true(const zdd::node_t &/*n*/, assignment_mgr &/*amgr*/)
    { adiar_unreachable(); }
    // LCOV_EXCL_STOP

  public:
    static inline zdd terminal(bool terminal_val, assignment_mgr& /*amgr*/)
    { return zdd_terminal(terminal_val); }
  };

  __zdd zdd_offset(const zdd &A, const std::function<zdd::label_t()> &xs)
  {
    // Both { Ø }, and Ø cannot have more variables removed
    if (is_terminal(A)) { return A; }

    zdd_subset_labels<assignment::False> amgr(xs);

    // Empty set of variables in `xs`?
    if (!amgr.has_level_incl()) { return A; }

    // Run Substitute sweep
    __zdd res = internal::substitute<zdd_offset_policy<zdd_subset_labels<assignment::False>>>(A, amgr);

    // Skip Reduce if no level of `xs` matched with any in `A`.
    if (!amgr.l_match) {
      return A;
    }
    return res;
  }

  //////////////////////////////////////////////////////////////////////////////
  template<typename assignment_mgr>
  class zdd_onset_policy : public zdd_policy
  {
  public:
    static internal::substitute_rec keep_node(const zdd::node_t &n, assignment_mgr &amgr)
    {
      if (amgr.has_level_incl()) {
        // If recursion goes past the intended level, then it is replaced with
        // the false terminal.
        const zdd::ptr_t low  = n.low().is_terminal() || n.low().label() > amgr.level_incl()
          ? zdd::ptr_t(false)
          : n.low();

        // If this applies to high, then the node should be skipped entirely.
        if (n.high().is_terminal() || n.high().label() > amgr.level_incl()) {
          return internal::substitute_rec_skipto { low };
        }
        return internal::substitute_rec_output { zdd::node_t(n.uid(), low, n.high()) };
      }
      return internal::substitute_rec_output { n };
    }

    // LCOV_EXCL_START
    static internal::substitute_rec fix_false(const zdd::node_t &/*n*/, assignment_mgr &/*amgr*/)
    { adiar_unreachable(); }
    // LCOV_EXCL_STOP

    static internal::substitute_rec fix_true(const zdd::node_t &n, assignment_mgr &amgr)
    {
      if (amgr.has_level_excl()) {
        if (n.high().is_terminal() || n.high().label() > amgr.level_excl()) {
          return internal::substitute_rec_skipto { zdd::ptr_t(false) };
        }
      }

      return internal::substitute_rec_output { zdd::node_t(n.uid(), zdd::ptr_t(false), n.high()) };
    }

  public:
    static inline zdd terminal(bool terminal_val, assignment_mgr &amgr)
    {
      return zdd_terminal(!amgr.has_level_excl() && terminal_val);
    }
  };

  __zdd zdd_onset(const zdd &A, const std::function<zdd::label_t()> &xs)
  {
    if (is_false(A)) { return A; }

    zdd_subset_labels<assignment::True> amgr(xs);

    // Empty set of variables in `xs`?
    if (!amgr.has_level_incl()) {
      return A;
    }

    // If `A` is { Ø } and `xs` is non-empty, then it trivially collapses to Ø.
    if (is_true(A)) {
      return zdd_empty();
    }

    // Run Substitute sweep
    __zdd res = internal::substitute<zdd_onset_policy<zdd_subset_labels<assignment::True>>>(A, amgr);

    // Skip Reduce no levels of `xs` matched with one from `A`.
    if (!amgr.l_match) {
      return zdd_empty();
    }
    return res;
  }
}
