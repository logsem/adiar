#include <adiar/zdd.h>
#include <adiar/zdd/zdd_policy.h>

#include <adiar/internal/algorithms/intercut.h>
#include <adiar/internal/data_types/node.h>

namespace adiar
{
  class zdd_expand_policy : public zdd_policy
  {
  public:
    static constexpr bool may_skip = false;

    static constexpr bool cut_true_terminal = true;
    static constexpr bool cut_false_terminal = false;

    static constexpr size_t mult_factor = 2u;

  public:
    static zdd on_empty_labels(const zdd& dd)
    {
      return dd;
    }


    static zdd on_terminal_input(const bool terminal_value, const zdd& dd, const label_file &labels)
    {
      return terminal_value ? zdd_powerset(labels) : dd;
    }

    // LCOV_EXCL_START
    static zdd terminal(const bool /*terminal_value*/)
    {
      adiar_unreachable();
    }
    // LCOV_EXCL_END

    static inline intercut_rec hit_existing(const node &/*n*/)
    {
      // The user should NOT have supplied a label that hits any existing nodes.
      adiar_unreachable(); // LCOV_EXCL_LINE
    }

    static inline intercut_rec_output hit_cut(const ptr_uint64 target)
    {
      return intercut_rec_output { target, target };
    }

    static inline intercut_rec_output miss_existing(const node &n)
    {
      return intercut_rec_output { n.low(), n.high() };
    }
  };

  __zdd zdd_expand(const zdd &dd, const label_file &labels)
  {
    return intercut<zdd_expand_policy>(dd, labels);
  }
}
