#ifndef COOM_COUNT_CPP
#define COOM_COUNT_CPP

#include <tpie/file_stream.h>
#include <tpie/priority_queue.h>

#include "data.h"

#include "debug.h"
#include "debug_data.h"

#include "assert.h"

#include "count.h"

namespace coom
{
  namespace debug
  {
    inline void println_count_low_sum([[maybe_unused]] const uint64_t node_ptr,
                                      [[maybe_unused]] const uint64_t count)
    {
#if COOM_DEBUG
      debug::print_node_ptr(node_ptr);
      tpie::log_info() << " | low : " << count << std::endl;
#endif
    }

    inline void println_count_high_sum([[maybe_unused]] const uint64_t node_ptr,
                                       [[maybe_unused]] const uint64_t count)
    {
#if COOM_DEBUG
      debug::print_node_ptr(node_ptr);
      tpie::log_info() << " | high : " << count << std::endl;
#endif
    }

    inline void println_count_result([[maybe_unused]] const uint64_t count)
    {
#if COOM_DEBUG
      tpie::log_info() << std::endl << "total : " << count << std::endl;
#endif
    }

  }

  struct partial_sum
  {
    uint64_t node_ptr;
    uint64_t sum;
  };

  struct count_queue_lt
  {
    bool operator()(const partial_sum &a, const partial_sum &b)
    {
      return a.node_ptr < b.node_ptr;
    }
  };

  inline uint64_t count(tpie::file_stream<node> &nodes,
                        const sink_pred &sink_pred,
                        const bool count_skipped_layers)
  {
    debug::println_algorithm_start("COUNT");

    assert::is_valid_input_stream(nodes);
    debug::println_file_stream(nodes, "nodes");

    nodes.seek(0);
    uint64_t biggest_label = label_of(nodes.read());

    nodes.seek(0, tpie::file_stream_base::end);
    tpie::priority_queue<partial_sum, count_queue_lt> partial_sums;

    //Take root out and put its children into the priority queue or count them immediately if they are sinks
    node root = nodes.read_back();
    uint64_t result = 0;
    if (is_sink(root.low))
    {
      if (sink_pred(root.low))
      {
        uint64_t new_sum_low = count_skipped_layers ? 1 << (biggest_label - label_of(root)) : 1;
        result = result + new_sum_low;
        debug::println_count_low_sum(root.node_ptr, new_sum_low);
      } else {
        debug::println_count_low_sum(root.node_ptr, 0);
      }
    }
    else
    {
      uint64_t new_sum_low = count_skipped_layers ?  1 << (label_of(root.low) - label_of(root) - 1) : 1;
      partial_sums.push({root.low, new_sum_low});
      debug::println_count_low_sum(root.node_ptr, new_sum_low);
    }
    if (is_sink(root.high))
    {
      if (sink_pred(root.high))
      {
        uint64_t new_sum_high = count_skipped_layers ? 1 << (biggest_label - label_of(root)) : 1;
        result = result + new_sum_high;

        debug::println_count_high_sum(root.node_ptr, new_sum_high);
      } else {
        debug::println_count_high_sum(root.node_ptr, 0);
      }
    }
    else
    {
      uint64_t new_sum_high = count_skipped_layers ? 1 << (label_of(root.high) - label_of(root) - 1) : 1;
      partial_sums.push({root.high, new_sum_high});
      debug::println_count_high_sum(root.node_ptr, new_sum_high);
    }

    //Take out the rest of the nodes and process them one by one
    node current_node;
    partial_sum current_sum;
    while (nodes.can_read_back())
    {
      current_node = nodes.read_back();
      uint64_t next_sum = 0;

      //Pull out all "edges" that point to the current node and add their paths
      while (!partial_sums.empty() && partial_sums.top().node_ptr == current_node.node_ptr)
      {
        current_sum = partial_sums.top();
        partial_sums.pop();
        next_sum = next_sum + current_sum.sum;
      }

      //Put children of the current node into the priority queue or count them if they are sinks
      if (is_sink(current_node.low))
      {
        if (sink_pred(current_node.low))
        {
          uint64_t new_sum_low = count_skipped_layers ? next_sum * (1 << (biggest_label - label_of(current_node))) : next_sum;
          result = result + new_sum_low;
          debug::println_count_low_sum(current_node.node_ptr, new_sum_low);
        } else {
          debug::println_count_low_sum(current_node.node_ptr, 0);
        }
      }
      else
      {
        uint64_t new_sum_low = count_skipped_layers ? next_sum * (1 << (label_of(current_node.low) - label_of(current_node) - 1)) : next_sum;
        partial_sums.push({current_node.low, new_sum_low});
        debug::println_count_low_sum(current_node.node_ptr, new_sum_low);
      }

      if (is_sink(current_node.high))
      {
        if (sink_pred(current_node.high))
        {
          uint64_t new_sum_high = count_skipped_layers ? next_sum * (1 << (biggest_label - label_of(current_node))) : next_sum;
          result = result + new_sum_high;
          debug::println_count_high_sum(current_node.node_ptr, new_sum_high);
        } else {
          debug::println_count_high_sum(current_node.node_ptr, 0);
        }
      }
      else
      {
        uint64_t new_sum_high = count_skipped_layers ? next_sum * (1 << (label_of(current_node.high) - label_of(current_node) - 1)) : next_sum;
        partial_sums.push({current_node.high, new_sum_high});
        debug::println_count_high_sum(current_node.node_ptr, new_sum_high);
      }
    }

    debug::println_count_result(result);
    debug::println_algorithm_end("COUNT");
    return result;
  }

  uint64_t count_paths(tpie::file_stream<node> &nodes,
                       const sink_pred &sink_pred)
  {
    return count(nodes, sink_pred, false);
  }

  uint64_t count_paths(tpie::file_stream<node> &nodes)
  {
    return count_paths(nodes, coom::is_any);
  }

  uint64_t count_assignments(tpie::file_stream<node> &nodes,
                             const sink_pred &sink_pred)
  {
    return count(nodes, sink_pred, true);
  }
} // namespace coom

#endif // COOM_COUNT_CPP