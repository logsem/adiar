#ifndef ADIAR_HOMOMORPHISM_CPP
#define ADIAR_HOMOMORPHISM_CPP

#include "homomorphism.h"

#include <adiar/file_stream.h>
#include <adiar/priority_queue.h>
#include <adiar/tuple.h>

namespace adiar
{
  //////////////////////////////////////////////////////////////////////////////
  // Data structures
  struct homomorphism_tuple_data : tuple_data
  {
    bool from_1;
  };

  typedef node_priority_queue<tuple, tuple_queue_label, tuple_queue_1_lt, std::less<>, 2> homomorphism_priority_queue_t;
  typedef tpie::priority_queue<homomorphism_tuple_data, tuple_queue_2_lt> homomorphism_data_priority_queue_t;

  //////////////////////////////////////////////////////////////////////////////
  // Helper functions

  // Return whether one can do an early exit
  inline bool homomorphism_resolve_request(homomorphism_priority_queue_t &pq,
                                           ptr_t r1, ptr_t r2)
  {
    // Are they both a sink (and the same sink)?
    if (is_sink_ptr(r1) || is_sink_ptr(r2)) {
      return is_sink_ptr(r1) && is_sink_ptr(r2)
        ? value_of(r1) != value_of(r2)
        : true;
    }

    // Do they NOT point to a node with the same label?
    if (label_of(r1) != label_of(r2)) {
      return true;
    }

    // No violation, so recurse
    pq.push({ r1, r2 });
    return false;
  }

  //////////////////////////////////////////////////////////////////////////////
  bool is_homomorphic(const node_file &f1, const node_file &f2,
                      bool negate1, bool negate2)
  {
    // Are they literally referring to the same underlying file?
    if (f1._file_ptr == f2._file_ptr) {
      return negate1 == negate2;
    }

    // Are they trivially not the same thing, since they have different number
    // of layers (in the _meta_file) or have different number of nodes (in
    // _files[0])
    if (f1._file_ptr -> _meta_file.size() != f2._file_ptr -> _meta_file.size()
        || f1._file_ptr -> _files[0].size() != f2._file_ptr -> _files[0].size()) {
      return false;
    }

    node_stream<> in_nodes_1(f1, negate1);
    node_stream<> in_nodes_2(f2, negate2);

    node_t v1 = in_nodes_1.pull();
    node_t v2 = in_nodes_2.pull();

    if (is_sink(v1) && is_sink(v2)) {
      return value_of(v1) == value_of(v2);
    }

    // We already have from the meta file, if only one of them is a non-sink
    adiar_debug(!is_sink(v1) && !is_sink(v2), "sink cases handled");

    // Does the root label differ?
    if (label_of(v1) != label_of(v2)) {
      return false;
    }

    // Set up priority queue for recursion
    tpie::memory_size_type available_memory = tpie::get_memory_manager().available();

    homomorphism_priority_queue_t pq({f1,f2},(available_memory * 3) / 4);

    // Check for violation on root children, or 'recurse' otherwise
    if (homomorphism_resolve_request(pq, v1.low, v2.low)) {
      return false;
    }

    if (homomorphism_resolve_request(pq, v1.high, v2.high)) {
      return false;
    }

    if (in_nodes_1.can_pull()) {
      adiar_debug(in_nodes_2.can_pull(), "input are the same size");
      v1 = in_nodes_1.pull();
      v2 = in_nodes_2.pull();
    }

    homomorphism_data_priority_queue_t pq_data;

    while (pq.can_pull() || pq.has_next_layer() || !pq_data.empty()) {
      if (!pq.can_pull() && pq_data.empty()) {
        pq.setup_next_layer();
      }

      ptr_t t1, t2;
      bool with_data, from_1 = false;
      ptr_t data_low = NIL, data_high = NIL;

      // Merge requests from pq or pq_data
      if (pq.can_pull() && (pq_data.empty() ||
                              std::min(pq.top().t1, pq.top().t2) <
                              std::max(pq_data.top().t1, pq_data.top().t2))) {
        with_data = false;
        t1 = pq.top().t1;
        t2 = pq.top().t2;

        pq.pop();
      } else {
        with_data = true;
        t1 = pq_data.top().t1;
        t2 = pq_data.top().t2;

        from_1 = pq_data.top().from_1;
        data_low = pq_data.top().data_low;
        data_high = pq_data.top().data_high;

        pq_data.pop();
      }

      // Seek request partially in stream
      if (with_data) {
        if (from_1) {
          while (v2.uid < t2) {
            v2 = in_nodes_2.pull();
          }
        } else {
          while (v1.uid < t1) {
            v1 = in_nodes_1.pull();
          }
        }
      } else {
        if (t1 == t2) {
          while (v1.uid < t1) {
            v1 = in_nodes_1.pull();
          }
          while (v2.uid < t2) {
            v2 = in_nodes_2.pull();
          }
        } else if (t1 < t2) {
          while (v1.uid < t1) {
            v1 = in_nodes_1.pull();
          }
        } else {
          while (v2.uid < t2) {
            v2 = in_nodes_2.pull();
          }
        }
      }

      // Forward information across the layer
      if (!with_data
          && !is_sink_ptr(t1) && !is_sink_ptr(t2) && label_of(t1) == label_of(t2)
          && (v1.uid != t1 || v2.uid != t2)) {
        bool from_1 = v1.uid == t1;
        node_t v0 = from_1 ? v1 : v2;

        pq_data.push({ t1, t2, v0.low, v0.high, from_1 });

        // Skip all requests to the same node
        while (pq.can_pull() && (pq.top().t1 == t1 && pq.top().t2 == t2)) {
          pq.pull();
        }
        continue;
      }

      // Check for violation in request, or 'recurse' otherwise
      if (homomorphism_resolve_request(pq,
                                       with_data && from_1 ? data_low : v1.low,
                                       with_data && !from_1 ? data_low : v2.low)) {
        return false;
      }

      if (homomorphism_resolve_request(pq,
                                       with_data && from_1 ? data_high : v1.high,
                                       with_data && !from_1 ? data_high : v2.high)) {
        return false;
      }
    }

    return true;
  }
}

#endif // ADIAR_HOMOMORPHISM_CPP