#ifndef ADIAR_INTERNAL_CUT_H
#define ADIAR_INTERNAL_CUT_H

namespace adiar
{
  ////////////////////////////////////////////////////////////////////////////
  /// \brief Type for a cut size
  ////////////////////////////////////////////////////////////////////////////
  typedef size_t cut_t;

  ////////////////////////////////////////////////////////////////////////////
  /// \brief Maximum value for a cut.
  ////////////////////////////////////////////////////////////////////////////
  constexpr cut_t MAX_CUT = std::numeric_limits<size_t>::max();

  ////////////////////////////////////////////////////////////////////////////
  /// \brief Available types of cuts. This should be used with the
  ///        <tt>max_ilevel_cut</tt> variables in <tt>node_file</tt> and
  ///        <tt>arc_file</tt>.
  ////////////////////////////////////////////////////////////////////////////
  enum cut_type {
    /** Internal arcs only */
    INTERNAL       = 0, // 0x00
    /** Internal arcs and false arcs only */
    INTERNAL_FALSE = 1, // 0x01
    /** Internal arcs and true arcs only */
    INTERNAL_TRUE  = 2, // 0x10
    /** All types of arcs: internal, false, and true */
    ALL            = 3  // 0x11
  };

  ////////////////////////////////////////////////////////////////////////////
  /// \brief Number of different types of cuts.
  ////////////////////////////////////////////////////////////////////////////
  constexpr size_t CUT_TYPES = 4u;

  ////////////////////////////////////////////////////////////////////////////
  /// \brief Type for list of (all) possible cuts.
  ////////////////////////////////////////////////////////////////////////////
  typedef cut_t cuts_t[CUT_TYPES];

  ////////////////////////////////////////////////////////////////////////////
  /// \brief Get the desired <tt>cut_type</tt> based on whether to
  ///        respectively include the false and true sinks.
  ///
  /// \param incl_false Whether to include arcs to the false sink.
  /// \param incl_true  Whether to include arcs to the true sink.
  ////////////////////////////////////////////////////////////////////////////
  inline cut_type cut_type_with(const bool incl_false, const bool incl_true)
  {
    return static_cast<cut_type>((incl_true << 1) + incl_false);
  }

  ////////////////////////////////////////////////////////////////////////////
  /// \brief Whether a type of cut includes arcs to the desired sink.
  ////////////////////////////////////////////////////////////////////////////
  inline bool includes_sink(const cut_type cut, const bool sink_val)
  {
    return sink_val
      ? cut >= cut_type::INTERNAL_TRUE
      : cut <= cut_type::INTERNAL_FALSE;
  }

  inline bool includes_sink(const size_t cut, const bool sink_val)
  {
    return includes_sink(static_cast<cut_type>(cut), sink_val);
  }

  // TODO: Exact top-down sweep computation of 1-level and 2-level cuts.
}

#endif // ADIAR_INTERNAL_CUT_H