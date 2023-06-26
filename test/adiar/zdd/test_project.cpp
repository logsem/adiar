#include "../../test.h"

go_bandit([]() {
  describe("adiar/zdd/project.cpp", []() {
    //////////////////////
    // Sink cases

    // Ø
    shared_levelized_file<zdd::node_t> zdd_empty;
    {
      node_writer nw(zdd_empty);
      nw << node(false);
    }

    // { Ø }
    shared_levelized_file<zdd::node_t> zdd_null;
    {
      node_writer nw(zdd_null);
      nw << node(true);
    }

    //////////////////////
    // Non-terminal edge cases
    ptr_uint64 terminal_F = ptr_uint64(false);
    ptr_uint64 terminal_T = ptr_uint64(true);

    // { Ø, {0}, {1}, {1,2}, {1,3}, {1,3,4} }
    /*
    //                     1         ---- x0
    //                    / \
    //                    2 T        ---- x1
    //                   ||
    //                    3          ---- x2
    //                   / \
    //                   4 T         ---- x3
    //                  / \
    //                  F 5          ---- x4
    //                   / \
    //                   T T
    */
    shared_levelized_file<zdd::node_t> zdd_1;
    {
      node_writer nw(zdd_1);
      nw << node(4, node::MAX_ID, terminal_T, terminal_T)
         << node(3, node::MAX_ID, terminal_F, ptr_uint64(4, ptr_uint64::MAX_ID))
         << node(2, node::MAX_ID, ptr_uint64(3, ptr_uint64::MAX_ID), terminal_T)
         << node(1, node::MAX_ID, ptr_uint64(2, ptr_uint64::MAX_ID), ptr_uint64(2, ptr_uint64::MAX_ID))
         << node(0, node::MAX_ID, ptr_uint64(1, ptr_uint64::MAX_ID), terminal_T)
        ;
    }

    // { {0}, {2}, {0,3}, {2,4} }
    /*
    //                    1         ---- x0
    //                   / \
    //                   |  \       ---- x1
    //                   |   \
    //                   2   |      ---- x2
    //                  / \  |
    //                  F |  3      ---- x3
    //                    | / \
    //                    4 T T     ---- x4
    //                   / \
    //                   T T
    */
    shared_levelized_file<zdd::node_t> zdd_2;
    {
      node_writer nw(zdd_2);
      nw << node(4, node::MAX_ID, terminal_T, terminal_T)
         << node(3, node::MAX_ID, terminal_T, terminal_T)
         << node(2, node::MAX_ID, terminal_F, ptr_uint64(4, ptr_uint64::MAX_ID))
         << node(0, node::MAX_ID, ptr_uint64(2, ptr_uint64::MAX_ID), ptr_uint64(3, ptr_uint64::MAX_ID))
        ;
    }

    // { {0}, {2}, {1,2}, {0,2} }
    /*
    //                    1      ---- x0
    //                   / \
    //                   2 |     ---- x1
    //                  / \|
    //                  3  4     ---- x2
    //                 / \/ \
    //                 F T  T
    */
    shared_levelized_file<zdd::node_t> zdd_3;
    {
      node_writer nw(zdd_3);
      nw << node(2, node::MAX_ID,   terminal_T, terminal_T)
         << node(2, node::MAX_ID-1, terminal_F, terminal_T)
         << node(1, node::MAX_ID,   ptr_uint64(2, ptr_uint64::MAX_ID-1), ptr_uint64(2, ptr_uint64::MAX_ID))
         << node(0, node::MAX_ID,   ptr_uint64(1, ptr_uint64::MAX_ID), ptr_uint64(2, ptr_uint64::MAX_ID))
        ;
    }

    // { {4}, {0,2}, {0,4}, {2,4}, {0,2,4} }
    /*
    //                    _1_      ---- x0
    //                   /   \
    //                   2   3     ---- x2
    //                   \\ / \
    //                     4  5    ---- x4
    //                    / \/ \
    //                    F  T T
    */
    shared_levelized_file<zdd::node_t> zdd_4;
    {
      node_writer nw(zdd_4);
      nw << node(4, node::MAX_ID,   terminal_T, terminal_T)
         << node(4, node::MAX_ID-1, terminal_F, terminal_T)
         << node(2, node::MAX_ID,   ptr_uint64(4, ptr_uint64::MAX_ID-1), ptr_uint64(4, ptr_uint64::MAX_ID))
         << node(2, node::MAX_ID-1, ptr_uint64(4, ptr_uint64::MAX_ID-1), ptr_uint64(4, ptr_uint64::MAX_ID-1))
         << node(0, node::MAX_ID,   ptr_uint64(2, ptr_uint64::MAX_ID-1), ptr_uint64(2, ptr_uint64::MAX_ID))
        ;
    }

    // TODO: Turn 'GreaterThanOrEqualTo' in max 1-level cuts below into an
    // 'EqualTo'.

    describe("zdd_project(const zdd&, const std::function<bool(zdd::label_t)>)", [&]() {
      it("returns same file for Ø with dom = {1,3,5,...} [const &]", [&](){
        zdd out = zdd_project(zdd_empty, [](zdd::label_t x) { return x % 2; });
        AssertThat(out.file_ptr(), Is().EqualTo(zdd_empty));
      });

      it("returns same file for { Ø } with dom = {0,2,4,...} [&&]", [&](){
        zdd out = zdd_project(zdd(zdd_null), [](zdd::label_t x) { return !(x % 2); });
        AssertThat(out.file_ptr(), Is().EqualTo(zdd_null));
      });

      describe("quantify_mode == SINGLETON", [&]() {
        quantify_mode = quantify_mode_t::SINGLETON;

        it("computes with dom = Ø to be { Ø } for non-empty input [zdd_1] [const &]", [&](){
          adiar::shared_file<zdd::label_t> dom;

          const zdd in = zdd_1;
          zdd out = zdd_project(in, [](zdd::label_t) { return false; });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("computes with dom = Ø to be { Ø } for non-empty input [zdd_2] [&&]", [&](){
          adiar::shared_file<zdd::label_t> dom;

          zdd out = zdd_project(zdd(zdd_2), [](zdd::label_t) { return false; });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("computes with disjoint dom = { x | x > 2 } to be { Ø } [zdd_3] [&&]", [&](){
          zdd out = zdd_project(zdd(zdd_3), [](zdd::label_t x) { return x > 2; });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("computes zdd_3 with dom = { x | x % 2 == 0 }", [&](){
          /* Expected: { {0}, {2}, {0,2} }
          //
          //                           1    ---- x0
          //                          / \
          //                          | |   ---- x1
          //                          \ /
          //                           2    ---- x2
          //                          / \
          //                          T T
          */

          zdd out = zdd_project(zdd_3, [](zdd::label_t x) { return !(x % 2); });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(2, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(0, node::MAX_ID,
                                                         ptr_uint64(2, ptr_uint64::MAX_ID),
                                                         ptr_uint64(2, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(2,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(0,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(2u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(2u));
        });

        quantify_mode = quantify_mode_t::AUTO;
      });

      describe("quantify_mode == PARTIAL", [&]() {
        quantify_mode = quantify_mode_t::PARTIAL;

        // TODO

        quantify_mode = quantify_mode_t::AUTO;
      });

      describe("quantify_mode == NESTED", [&]() {
        quantify_mode = quantify_mode_t::NESTED;

        it("computes with dom = Ø to be { Ø } for non-empty input [zdd_1]", [&](){
          adiar::shared_file<zdd::label_t> dom;

          const zdd in = zdd_1;
          zdd out = zdd_project(in, [](zdd::label_t) { return false; });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("computes with disjoint dom = { x | x > 2 } to be { Ø } [zdd_3]", [&](){
          zdd out = zdd_project(zdd(zdd_3), [](zdd::label_t x) { return x > 2; });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("computes zdd_3 with dom = { x | x % 2 == 0 }", [&](){
          /* Expected: { {0}, {2}, {0,2} }
          //
          //                           1    ---- x0
          //                          / \
          //                          | |   ---- x1
          //                          \ /
          //                           2    ---- x2
          //                          / \
          //                          T T
          */

          zdd out = zdd_project(zdd_3, [](zdd::label_t x) { return !(x % 2); });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(2, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(0, node::MAX_ID,
                                                         ptr_uint64(2, ptr_uint64::MAX_ID),
                                                         ptr_uint64(2, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(2,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(0,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(2u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(2u));
        });

        quantify_mode = quantify_mode_t::AUTO;
      });

      describe("quantify_mode == AUTO", [&]() {
        quantify_mode = quantify_mode_t::AUTO;

        it("computes with dom = Ø to be { Ø } for non-empty input [zdd_2]", [&](){
          adiar::shared_file<zdd::label_t> dom;

          zdd out = zdd_project(zdd(zdd_2), [](zdd::label_t) { return false; });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("computes with disjoint dom = { x | x > 2 } to be { Ø } [zdd_3]", [&](){
          zdd out = zdd_project(zdd(zdd_3), [](zdd::label_t x) { return x > 2; });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("computes zdd_1 with dom = { x | x % 2 == 1 }", [&](){
          /* Expected: { Ø, {1}, {1,3} }
          //
          //                    2         ---- x1
          //                   / \
          //                   | |
          //                   \ /
          //                    4         ---- x3
          //                   / \
          //                   T T
          */

          zdd out = zdd_project(zdd_1, [](zdd::label_t x) { return x % 2 == 1; });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(3, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(1, node::MAX_ID,
                                                         ptr_uint64(3, ptr_uint64::MAX_ID),
                                                         ptr_uint64(3, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(3,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(1,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(2u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(2u));
        });

        it("computes zdd_1 with dom = { x | x != 0,2 }", [&](){
          /* Expected: { Ø, {1}, {1,3}, {1,3,4} }
          //
          //                    2         ---- x1
          //                   / \
          //                   | |
          //                   \ /
          //                    4         ---- x3
          //                   / \
          //                   T 5        ---- x4
          //                    / \
          //                    T T
          */

          zdd out = zdd_project(zdd_1, [](zdd::label_t x) { return x != 0 && x != 2; });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(3, node::MAX_ID,
                                                         terminal_T,
                                                         ptr_uint64(4, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(1, node::MAX_ID,
                                                         ptr_uint64(3, ptr_uint64::MAX_ID),
                                                         ptr_uint64(3, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(4,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(3,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(1,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(3u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(3u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(3u));
        });

        it("computes zdd_3 with dom = { x | x % 2 == 0 }", [&](){
          /* Expected: { {0}, {2}, {0,2} }
          //
          //                           1    ---- x0
          //                          / \
          //                          | |   ---- x1
          //                          \ /
          //                           2    ---- x2
          //                          / \
          //                          T T
          */

          zdd out = zdd_project(zdd_3, [](zdd::label_t x) { return x % 2 == 0; });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(2, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(0, node::MAX_ID,
                                                         ptr_uint64(2, ptr_uint64::MAX_ID),
                                                         ptr_uint64(2, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(2,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(0,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(2u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(2u));
        });

        quantify_mode = quantify_mode_t::AUTO;
      });
    });

    describe("zdd_project(const zdd&, const std::function<zdd::label_t()>&)", [&]() {
      it("returns same file for Ø with dom = {6,4,2,0} [const &]", [&](){
        zdd::label_t var = 6;

        const zdd in = zdd_empty;
        zdd out = zdd_project(in, [&var]() {
          const zdd::label_t ret = var;
          var -= 2;
          return ret;
        });

        AssertThat(out.file_ptr(), Is().EqualTo(in.file_ptr()));
      });

      it("returns same file for { Ø } with dom = {1,3,5} [&&]", [&](){
        zdd::label_t var = 6;

        zdd out = zdd_project(zdd(zdd_null), [&var]() {
          const zdd::label_t ret = var;
          var -= 2;
          return ret;
        });

        AssertThat(out.file_ptr(), Is().EqualTo(zdd_null));
      });

      it("returns same file for Ø with dom = Ø [&&]", [&](){
        zdd out = zdd_project(zdd(zdd_empty), []() { return -1; });
        AssertThat(out.file_ptr(), Is().EqualTo(zdd_empty));
      });

      it("returns same file for { Ø } with dom = Ø [const &]", [&](){
        const zdd in = zdd_null;
        zdd out = zdd_project(in, []() { return -1; });
        AssertThat(out.file_ptr(), Is().EqualTo(in.file_ptr()));
      });

      describe("quantify_mode == SINGLETON / PARTIAL", [&]() {
        quantify_mode = quantify_mode_t::SINGLETON;

        it("collapses zdd_1 with dom = Ø into { Ø } [const &]", [&](){
          const zdd in = zdd_1;
          zdd out = zdd_project(in, []() { return -1; });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("collapses zdd_2 with dom = Ø into { Ø } [&&]", [&](){
          zdd out = zdd_project(zdd(zdd_2), []() { return -1; });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("collapses zdd_3 with dom = Ø into { Ø } [const &]", [&](){
          const zdd in = zdd_3;
          zdd out = zdd_project(in, []() { return -1; });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("collapses zdd_2 with disjoint domain into { Ø } [const &]", [&](){
          zdd::label_t var = 1;

          const zdd in = zdd_2;
          zdd out = zdd_project(in, [&var]() {
            const zdd::label_t ret = var;
            var -= 2;
            return ret;
          });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("computes with disjoint dom to be { Ø } [zdd_3] [&&]", [&](){
          zdd::label_t var = 5;
          zdd out = zdd_project(zdd(zdd_3), [&var]() {
            return 3 <= var && var <= 5 ? var-- : -1;
          });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        //////////////////////
        // Single-node case
        shared_levelized_file<zdd::node_t> zdd_x1;
        {
          node_writer nw(zdd_x1);
          nw << node(1u, node::MAX_ID, ptr_uint64(false), ptr_uint64(true));
        }

        it("returns { Ø } for {1} with dom = {0} [const &]", [&](){
          adiar::shared_file<zdd::label_t> dom;
          { label_writer lw(dom);
            lw << 0;
          }

          const zdd in = zdd_x1;

          /* Expected: { Ø }
           *
           *        T
           */

          zdd::label_t var = 0;
          zdd out = zdd_project(in, [&var]() {
            return var == 0 ? var--: var;
          });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("returns { 1 } for {1} with dom = {1,0} [const &]", [&](){
          adiar::shared_file<zdd::label_t> dom;
          { label_writer lw(dom);
            lw << 0 << 1;
          }

          const zdd in = zdd_x1;

          /* Expected: { 1 }
           *
           *        1       ---- x1
           *       / \
           *       F T
           */
          zdd::label_t var = 1;
          zdd out = zdd_project(in, [&var]() {
            return var <= 1 ? var--: var;
          });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(1, node::MAX_ID, zdd::ptr_t(false), zdd::ptr_t(true))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(1,1u)));
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(1u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        //////////////////////
        // Non-terminal general case
        it("computes zdd_1 with dom = {4,3,2} [const &]", [&](){
          const zdd in = zdd_1;

          /* Expected: { Ø, {2}, {3}, {3,4} }
          //
          //         1    ---- x2
          //        / \
          //        2 T   ---- x3
          //       / \
          //       T 3    ---- x4
          //        / \
          //        T T
          */
          zdd::label_t var = 4;
          zdd out = zdd_project(in, [&var]() {
            return 2 <= var && var <= 4 ? var-- : -1;
          });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(3, node::MAX_ID,
                                                         terminal_T,
                                                         ptr_uint64(4, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(2, node::MAX_ID,
                                                         ptr_uint64(3, ptr_uint64::MAX_ID),
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(4,1u)));


          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(3,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(2,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(4u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(4u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(4u));
        });

        it("computes zdd_2 with dom = {4,3,2} [&&]", [&](){
          /* Expected: { Ø, {2}, {3}, {2,4} }
          //
          //      1      ---- x2
          //     / \
          //     2  \    ---- x3
          //    / \ |
          //    T T 3    ---- x4
          //       / \
          //       T T
          */
          zdd::label_t var = 4;
          zdd out = zdd_project(zdd_2, [&var]() {
            return 2 <= var && var <= 4 ? var-- : -1;
          });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(3, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(2, node::MAX_ID,
                                                         ptr_uint64(3, ptr_uint64::MAX_ID),
                                                         ptr_uint64(4, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(4,1u)));


          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(3,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(2,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(4u));
        });

        it("computes zdd_3 with dom = {4,2,0} [&&]", [&](){
          /* Expected: { {0}, {2}, {0,2} }
          //
          //         1    ---- x0
          //        / \
          //        | |   ---- x1
          //        \ /
          //         2    ---- x2
          //        / \
          //        T T
          */
          zdd::label_t var = 4;
          zdd out = zdd_project(zdd_3, [&var]() {
            const zdd::label_t ret = var;
            var -= 2;
            return ret;
          });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(2, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(0, node::MAX_ID,
                                                         ptr_uint64(2, ptr_uint64::MAX_ID),
                                                         ptr_uint64(2, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(2,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(0,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(2u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(2u));
        });

        it("computes zdd_4 with dom = {4,0} [&&]", [&](){
          /* Expected: { {0}, {4}, {0,4} }
          //
          //       1     ---- x0
          //      / \
          //     /   \   ---- x2
          //     |   |
          //     2   3   ---- x4
          //    / \ / \
          //    F T T T
          */
          zdd::label_t var = 4;
          zdd out = zdd_project(zdd_4, [&var]() {
            const zdd::label_t ret = var;
            var -= 4;
            return ret;
          });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID-1,
                                                         terminal_F,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(0, node::MAX_ID,
                                                         ptr_uint64(4, ptr_uint64::MAX_ID-1),
                                                         ptr_uint64(4, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(4,2u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(0,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(3u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(3u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(1u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(3u));
        });

        it("computes zdd_4 with dom = {4,2} [&&]", [&](){
          /* Expected: { {2}, {4}, {2,4} }
          //
          //       1     ---- x2
          //      / \
          //      2 3    ---- x4
          //     / \||
          //     F  T
          */
          zdd::label_t var = 4;
          zdd out = zdd_project(zdd_4, [&var]() {
            const zdd::label_t res = var;
            if (var == 4) { var -= 2; }
            else          { var = -1; }
            return res;
          });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID-1,
                                                         terminal_F,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(2, node::MAX_ID,
                                                         ptr_uint64(4, ptr_uint64::MAX_ID-1),
                                                         ptr_uint64(4, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(4,2u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(2,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(3u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(1u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(3u));
        });

        quantify_mode = quantify_mode_t::AUTO;
      });

      describe("quantify_mode == NESTED / AUTO", [&]() {
        quantify_mode = quantify_mode_t::NESTED;

        it("collapses zdd_1 with dom = Ø into { Ø } [const &]", [&](){
          const zdd in = zdd_1;
          zdd out = zdd_project(in, []() { return -1; });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("collapses zdd_2 with dom = Ø into { Ø } [&&]", [&](){
          zdd out = zdd_project(zdd(zdd_2), []() { return -1; });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("collapses zdd_3 with dom = Ø into { Ø } [const &]", [&](){
          const zdd in = zdd_3;
          zdd out = zdd_project(in, []() { return -1; });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("collapses zdd_2 with disjoint domain into { Ø } [const &]", [&](){
          zdd::label_t var = 1;

          const zdd in = zdd_2;
          zdd out = zdd_project(in, [&var]() {
            const zdd::label_t ret = var;
            var -= 2;
            return ret;
          });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("computes with disjoint dom to be { Ø } [zdd_3] [&&]", [&](){
          zdd::label_t var = 5;
          zdd out = zdd_project(zdd(zdd_3), [&var]() {
            return 3 <= var && var <= 5 ? var-- : -1;
          });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        // TODO: Shortcut on nothing to do

        //////////////////////
        // Single-node case
        shared_levelized_file<zdd::node_t> zdd_x1;
        {
          node_writer nw(zdd_x1);
          nw << node(1u, node::MAX_ID, ptr_uint64(false), ptr_uint64(true));
        }

        it("returns { Ø } for {1} with dom = {0} [const &]", [&](){
          adiar::shared_file<zdd::label_t> dom;
          { label_writer lw(dom);
            lw << 0;
          }

          const zdd in = zdd_x1;

          /* Expected: { Ø }
           *
           *        T
           */

          zdd::label_t var = 0;
          zdd out = zdd_project(in, [&var]() {
            return var == 0 ? var--: var;
          });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(true)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        it("returns { 1 } for {1} with dom = {1,0} [const &]", [&](){
          adiar::shared_file<zdd::label_t> dom;
          { label_writer lw(dom);
            lw << 0 << 1;
          }

          const zdd in = zdd_x1;

          /* Expected: { 1 }
           *
           *        1       ---- x1
           *       / \
           *       F T
           */
          zdd::label_t var = 1;
          zdd out = zdd_project(in, [&var]() {
            return var <= 1 ? var--: var;
          });

          node_test_stream out_nodes(out);
          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(1, node::MAX_ID, zdd::ptr_t(false), zdd::ptr_t(true))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);
          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(1,1u)));
          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(1u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(1u));
        });

        //////////////////////
        // Non-terminal general case
        it("computes zdd_1 with dom = {4,3,2} [const &]", [&](){
          const zdd in = zdd_1;

          /* Expected: { Ø, {2}, {3}, {3,4} }
          //
          //         1    ---- x2
          //        / \
          //        2 T   ---- x3
          //       / \
          //       T 3    ---- x4
          //        / \
          //        T T
          */
          zdd::label_t var = 4;
          zdd out = zdd_project(in, [&var]() {
            return 2 <= var && var <= 4 ? var-- : -1;
          });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(3, node::MAX_ID,
                                                         terminal_T,
                                                         ptr_uint64(4, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(2, node::MAX_ID,
                                                         ptr_uint64(3, ptr_uint64::MAX_ID),
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(4,1u)));


          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(3,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(2,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(4u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(4u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(4u));
        });

        it("computes zdd_2 with dom = {4,3,2} [&&]", [&](){
          /* Expected: { Ø, {2}, {3}, {2,4} }
          //
          //      1      ---- x2
          //     / \
          //     2  \    ---- x3
          //    / \ |
          //    T T 3    ---- x4
          //       / \
          //       T T
          */
          zdd::label_t var = 4;
          zdd out = zdd_project(zdd_2, [&var]() {
            return 2 <= var && var <= 4 ? var-- : -1;
          });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(3, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(2, node::MAX_ID,
                                                         ptr_uint64(3, ptr_uint64::MAX_ID),
                                                         ptr_uint64(4, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(4,1u)));


          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(3,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(2,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(4u));
        });

        it("computes zdd_3 with dom = {4,2,0} [&&]", [&](){
          /* Expected: { {0}, {2}, {0,2} }
          //
          //         1    ---- x0
          //        / \
          //        | |   ---- x1
          //        \ /
          //         2    ---- x2
          //        / \
          //        T T
          */
          zdd::label_t var = 4;
          zdd out = zdd_project(zdd_3, [&var]() {
            const zdd::label_t ret = var;
            var -= 2;
            return ret;
          });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(2, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(0, node::MAX_ID,
                                                         ptr_uint64(2, ptr_uint64::MAX_ID),
                                                         ptr_uint64(2, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(2,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(0,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(2u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(2u));
        });

        it("computes zdd_4 with dom = {4,0} [&&]", [&](){
          /* Expected: { {0}, {4}, {0,4} }
          //
          //       1     ---- x0
          //      / \
          //     /   \   ---- x2
          //     |   |
          //     2   3   ---- x4
          //    / \ / \
          //    F T T T
          */
          zdd::label_t var = 4;
          zdd out = zdd_project(zdd_4, [&var]() {
            const zdd::label_t ret = var;
            var -= 4;
            return ret;
          });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID-1,
                                                         terminal_F,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(0, node::MAX_ID,
                                                         ptr_uint64(4, ptr_uint64::MAX_ID-1),
                                                         ptr_uint64(4, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(4,2u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(0,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(3u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(3u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(1u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(3u));
        });

        it("computes zdd_4 with dom = {4,2} [&&]", [&](){
          /* Expected: { {2}, {4}, {2,4} }
          //
          //       1     ---- x2
          //      / \
          //      2 3    ---- x4
          //     / \||
          //     F  T
          */
          zdd::label_t var = 4;
          zdd out = zdd_project(zdd_4, [&var]() {
            const zdd::label_t res = var;
            if (var == 4) { var -= 2; }
            else          { var = -1; }
            return res;
          });

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID-1,
                                                         terminal_F,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(2, node::MAX_ID,
                                                         ptr_uint64(4, ptr_uint64::MAX_ID-1),
                                                         ptr_uint64(4, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(4,2u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(2,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(3u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(1u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(3u));
        });

        quantify_mode = quantify_mode_t::AUTO;
      });
    });

    describe("zdd_project(const zdd&, IT, IT)", [&]() {
      // Since this is merely a wrapper on the generator function, we will just
      // double-check with a few tests.

      describe("quantify_mode == SINGLETON / PARTIAL", [&]() {
        quantify_mode = quantify_mode_t::NESTED;

        it("computes zdd_2 with dom = {4,3,2} [&&]", [&](){
          const std::vector<int> dom = {4,3,2};

          /* Expected: { Ø, {2}, {3}, {2,4} }
          //
          //      1      ---- x2
          //     / \
          //     2  \    ---- x3
          //    / \ |
          //    T T 3    ---- x4
          //       / \
          //       T T
          */
          zdd out = zdd_project(zdd_2, dom.cbegin(), dom.cend());

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(3, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(2, node::MAX_ID,
                                                         ptr_uint64(3, ptr_uint64::MAX_ID),
                                                         ptr_uint64(4, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(4,1u)));


          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(3,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(2,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(1u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(0u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(1u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(4u));
        });

        quantify_mode = quantify_mode_t::AUTO;
      });

      describe("quantify_mode == NESTED / AUTO", [&]() {
        quantify_mode = quantify_mode_t::NESTED;

        it("computes zdd_3 with dom = {4,2,0} [&&]", [&](){
          const std::vector<int> dom = {4,2,0};

          /* Expected: { {0}, {2}, {0,2} }
          //
          //         1    ---- x0
          //        / \
          //        | |   ---- x1
          //        \ /
          //         2    ---- x2
          //        / \
          //        T T
          */
          zdd out = zdd_project(zdd_3, dom.cbegin(), dom.cend());

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(2, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(0, node::MAX_ID,
                                                         ptr_uint64(2, ptr_uint64::MAX_ID),
                                                         ptr_uint64(2, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(2,1u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(0,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(2u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(0u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(2u));
        });

        it("computes zdd_4 with dom = {4,0} [const &]", [&](){
          std::vector<int> dom = {4,0};
          zdd in = zdd_4;

          /* Expected: { {0}, {4}, {0,4} }
          //
          //       1     ---- x0
          //      / \
          //     /   \   ---- x2
          //     |   |
          //     2   3   ---- x4
          //    / \ / \
          //    F T T T
          */
          zdd out = zdd_project(in, dom.cbegin(), dom.cend());

          node_test_stream out_nodes(out);

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID,
                                                         terminal_T,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(4, node::MAX_ID-1,
                                                         terminal_F,
                                                         terminal_T)));

          AssertThat(out_nodes.can_pull(), Is().True());
          AssertThat(out_nodes.pull(), Is().EqualTo(node(0, node::MAX_ID,
                                                         ptr_uint64(4, ptr_uint64::MAX_ID-1),
                                                         ptr_uint64(4, ptr_uint64::MAX_ID))));

          AssertThat(out_nodes.can_pull(), Is().False());

          level_info_test_stream ms(out);

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(4,2u)));

          AssertThat(ms.can_pull(), Is().True());
          AssertThat(ms.pull(), Is().EqualTo(level_info(0,1u)));

          AssertThat(ms.can_pull(), Is().False());

          AssertThat(out->max_1level_cut[cut_type::INTERNAL], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_FALSE], Is().GreaterThanOrEqualTo(2u));
          AssertThat(out->max_1level_cut[cut_type::INTERNAL_TRUE], Is().GreaterThanOrEqualTo(3u));
          AssertThat(out->max_1level_cut[cut_type::ALL], Is().GreaterThanOrEqualTo(3u));

          AssertThat(out->number_of_terminals[false], Is().EqualTo(1u));
          AssertThat(out->number_of_terminals[true],  Is().EqualTo(3u));
        });

        quantify_mode = quantify_mode_t::AUTO;
      });
    });
  });
 });
