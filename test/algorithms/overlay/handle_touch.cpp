// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2015 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_GEOMETRY_DEBUG_IDENTIFIER
#define BOOST_GEOMETRY_DEBUG_SEGMENT_IDENTIFIER

#define BOOST_GEOMETRY_DEBUG_HANDLE_TOUCH


#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>


#include <geometry_test_common.hpp>


#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/enrichment_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/traversal_info.hpp>

#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/detail/overlay/enrich_intersection_points.hpp>
#include <boost/geometry/algorithms/detail/overlay/handle_touch.hpp>

#include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>

#include <boost/geometry/policies/robustness/get_rescale_policy.hpp>

#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/correct.hpp>

#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>


#if defined(TEST_WITH_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#endif




namespace detail
{

template
<
        typename G1, typename G2,
        bg::detail::overlay::operation_type Direction,
        bool Reverse1, bool Reverse2
        >
struct test_handle_touch
{

    static void apply(std::string const& case_id,
                      std::size_t expected_traverse,
                      std::size_t expected_discarded,
                      G1 const& g1, G2 const& g2)
    {

        typedef typename bg::strategy::side::services::default_strategy
                <
                typename bg::cs_tag<G1>::type
                >::type side_strategy_type;

        typedef typename bg::point_type<G2>::type point_type;
        typedef typename bg::rescale_policy_type<point_type>::type
                rescale_policy_type;

        rescale_policy_type rescale_policy
                = bg::get_rescale_policy<rescale_policy_type>(g1, g2);

        typedef bg::detail::overlay::traversal_turn_info
                <
                point_type,
                typename bg::segment_ratio_type<point_type, rescale_policy_type>::type
                > turn_info;
        std::vector<turn_info> turns;

        bg::detail::get_turns::no_interrupt_policy policy;
        bg::get_turns<Reverse1, Reverse2, bg::detail::overlay::assign_null_policy>(g1, g2, rescale_policy, turns, policy);
        bg::enrich_intersection_points<Reverse1, Reverse2>(turns,
                                                           Direction == 1 ? bg::detail::overlay::operation_union
                                                                          : bg::detail::overlay::operation_intersection,
                                                           g1, g2, rescale_policy, side_strategy_type());

        typedef bg::model::ring<typename bg::point_type<G2>::type> ring_type;
        typedef std::vector<ring_type> out_vector;

        std::cout << "*** Case: " << case_id << std::endl;

        bg::detail::overlay::handle_touch(Direction, turns);

        // Check number of resulting u/u turns

        std::size_t uu_traverse = 0;
        std::size_t uu_discarded = 0;
        BOOST_FOREACH(turn_info const& turn, turns)
        {
            if (turn.both(bg::detail::overlay::operation_union))
            {
                if (turn.discarded)
                {
                    uu_discarded++;
                }
                else
                {
                    uu_traverse++;
                }
            }
        }

        BOOST_CHECK_MESSAGE(expected_traverse == uu_traverse,
                            "handle_touch: " << case_id
                            << " traverse expected: " << expected_traverse
                            << " detected: " << uu_traverse
                            << " type: " << string_from_type
                            <typename bg::coordinate_type<G1>::type>::name());
        BOOST_CHECK_MESSAGE(expected_discarded == uu_discarded,
                            "handle_touch: " << case_id
                            << " discarded expected: " << expected_discarded
                            << " detected: " << uu_discarded
                            << " type: " << string_from_type
                            <typename bg::coordinate_type<G1>::type>::name());

#if defined(TEST_WITH_SVG)
        {
            std::ostringstream filename;
            filename << "handle_touch"
                     << "_" << case_id
                     << "_" << string_from_type<typename bg::coordinate_type<G1>::type>::name()
                     << ".svg";

            std::ofstream svg(filename.str().c_str());

            bg::svg_mapper<typename bg::point_type<G2>::type> mapper(svg, 500, 500);
            mapper.add(g1);
            mapper.add(g2);

            // Input shapes in green/blue
            mapper.map(g1, "fill-opacity:0.5;fill:rgb(153,204,0);"
                       "stroke:rgb(153,204,0);stroke-width:3");
            mapper.map(g2, "fill-opacity:0.3;fill:rgb(51,51,153);"
                       "stroke:rgb(51,51,153);stroke-width:3");


            // turn points in orange, + enrichment/traversal info
            typedef typename bg::coordinate_type<G1>::type coordinate_type;

            // Simple map to avoid two texts at same place (note that can still overlap!)
            std::map<std::pair<int, int>, int> offsets;
            int index = 0;
            int const margin = 5;

            BOOST_FOREACH(turn_info const& turn, turns)
            {
                int lineheight = 8;
                mapper.map(turn.point, "fill:rgb(255,128,0);"
                           "stroke:rgb(0,0,0);stroke-width:1", 3);

                {
                    coordinate_type half = 0.5;
                    coordinate_type ten = 10;
                    // Map characteristics
                    // Create a rounded off point
                    std::pair<int, int> p
                            = std::make_pair(
                                  boost::numeric_cast<int>(half
                                                           + ten * bg::get<0>(turn.point)),
                                  boost::numeric_cast<int>(half
                                                           + ten * bg::get<1>(turn.point))
                                  );

                    std::string color = "fill:rgb(0,0,0);";
                    std::string fontsize = "font-size:8px;";

                    if (turn.both(bg::detail::overlay::operation_union))
                    {
                        // Adapt color to give visual feedback in SVG
                        if (turn.discarded)
                        {
                            color = "fill:rgb(255,0,0);"; // red
                        }
                        else
                        {
                            color = "fill:rgb(0,128,0);"; // green
                        }
                    }
                    else if (turn.discarded)
                    {
                        color = "fill:rgb(92,92,92);";
                        fontsize = "font-size:6px;";
                        lineheight = 6;
                    }
                    const std::string style = color + fontsize + "font-family:Arial;";

                    {
                        std::ostringstream out;
                        out << index
                            << ": " << bg::method_char(turn.method)
                            << std::endl
                            << "op: " << bg::operation_char(turn.operations[0].operation)
                                << " / " << bg::operation_char(turn.operations[1].operation)
                                //<< (turn.is_discarded() ? " (discarded) " : turn.blocked() ? " (blocked)" : "")
                                << std::endl;

                        if (turn.operations[0].enriched.next_ip_index != -1)
                        {
                            out << "ip: " << turn.operations[0].enriched.next_ip_index;
                        }
                        else
                        {
                            out << "vx: " << turn.operations[0].enriched.travels_to_vertex_index
                                << " -> ip: "  << turn.operations[0].enriched.travels_to_ip_index;
                        }
                        out << " / ";
                        if (turn.operations[1].enriched.next_ip_index != -1)
                        {
                            out << "ip: " << turn.operations[1].enriched.next_ip_index;
                        }
                        else
                        {
                            out << "vx: " << turn.operations[1].enriched.travels_to_vertex_index
                                << " -> ip: " << turn.operations[1].enriched.travels_to_ip_index;
                        }

                        out << std::endl;



                        offsets[p] += lineheight;
                        int offset = offsets[p];
                        offsets[p] += lineheight * 3;
                        mapper.text(turn.point, out.str(), style, margin, offset, lineheight);
                    }
                    index++;
                }
            }
        }
#endif
    }
};
}

template
<
        typename G1, typename G2,
        bg::detail::overlay::operation_type Direction,
        bool Reverse1 = false,
        bool Reverse2 = false
        >
struct test_handle_touch
{
    typedef detail::test_handle_touch
    <
    G1, G2, Direction, Reverse1, Reverse2
    > detail_test_handle_touch;

    inline static void apply(std::string const& case_id,
                             std::size_t expected_traverse,
                             std::size_t expected_discarded,
                             std::string const& wkt1, std::string const& wkt2)
    {
        if (wkt1.empty() || wkt2.empty())
        {
            return;
        }

        G1 g1;
        bg::read_wkt(wkt1, g1);

        G2 g2;
        bg::read_wkt(wkt2, g2);

        bg::correct(g1);
        bg::correct(g2);

        detail_test_handle_touch::apply(case_id,
                                        expected_traverse, expected_discarded,
                                        g1, g2);

    }
};


template <typename MultiPolygon>
void test_geometries()
{
    namespace ov = bg::detail::overlay;

    typedef test_handle_touch
            <
            MultiPolygon, MultiPolygon,
            ov::operation_intersection
            > test_handle_touch_intersection;
    typedef test_handle_touch
            <
            MultiPolygon, MultiPolygon,
            ov::operation_union
            > test_handle_touch_union;

    test_handle_touch_union::apply
            (
                "case_36", 1, 0,
                "MULTIPOLYGON(((1 0,0 3,4 2,1 0)))",
                "MULTIPOLYGON(((1 5,5 5,4 2,3 3,2 1,1 2,1 5)))"
                );
    test_handle_touch_union::apply
            (
                "case_85", 1, 0,
                "MULTIPOLYGON(((0 0,0 40,40 40,40 0,0 0),(10 10,30 10,30 30,10 30,10 10)))",
                "MULTIPOLYGON(((5 15,5 30,30 15,5 15)))"
                );

    test_handle_touch_union::apply
            (
                "uu_case_1", 0, 1,
                "MULTIPOLYGON(((4 0,2 2,4 4,6 2,4 0)))",
                "MULTIPOLYGON(((4 4,2 6,4 8,6 6,4 4)))"
                );
    test_handle_touch_union::apply
            (
                "uu_case_2", 0, 2,
                "MULTIPOLYGON(((0 0,0 2,2 4,4 2,6 4,8 2,8 0,0 0)))",
                "MULTIPOLYGON(((0 8,8 8,8 6,6 4,4 6,2 4,0 6,0 8)))"
                );

    // Provided by Menelaos (1)
    test_handle_touch_union::apply
            (
                "uu_case_3", 0, 2,
                "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((15 5,15 10,20 10,20 5,15 5)))",
                "MULTIPOLYGON(((10 0,15 5,15 0,10 0)),((10 5,10 10,15 10,15 5,10 5)))"
                );
    // Provided by Menelaos (2)
    test_handle_touch_union::apply
            (
                "uu_case_4", 1, 0,
                "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((15 5,15 10,20 10,20 5,15 5)))",
                "MULTIPOLYGON(((10 0,15 5,20 5,20 0,10 0)),((10 5,10 10,15 10,15 5,10 5)))"
                );

    // Mailed by Barend
    test_handle_touch_union::apply
            (
                "uu_case_5", 1, 0,
                "MULTIPOLYGON(((4 0,2 2,4 4,6 2,4 0)),((4 6,6 8,8 6,6 4,4 6)))",
                "MULTIPOLYGON(((4 4,2 6,4 8,6 6,4 4)),((4 2,7 6,8 3,4 2)))"
                );

    // Formerly referred to as a
    test_handle_touch_union::apply
            (
                "uu_case_6", 2, 0,
                "MULTIPOLYGON(((4 8,4 10,6 10,6 8,4 8)),((7 7,7 11,10 11,10 7,7 7)))",
                "MULTIPOLYGON(((6 6,6 8,8 8,8 6,6 6)),((6 10,6 12,8 12,8 10,6 10)),((9 9,11 9,11 2,3 2,3 9,5 9,5 3,9 3,9 9)))"
                );
    // Should result in 1 polygon with 2 holes
    // "POLYGON((4 9,4 10,6 10,6 12,8 12,8 11,10 11,10 9,11 9,11 2,3 2,3 9,4 9),(6 10,6 8,7 8,7 10,6 10),(6 8,5 8,5 3,9 3,9 7,8 7,8 6,6 6,6 8))"

    // Formerly referred to as b
    test_handle_touch_union::apply
            (
                "uu_case_7", 0, 2,
                "MULTIPOLYGON(((4 8,4 10,6 10,6 8,4 8)),((7 7,7 11,10 11,10 7,7 7)))",
                "MULTIPOLYGON(((6 6,6 8,8 8,8 6,6 6)),((6 10,6 12,8 12,8 10,6 10)))"
                );
    // Should result in 2 polygons
    // "MULTIPOLYGON(((4 8,4 10,6 10,6 8,4 8)),((7 8,7 10,6 10,6 12,8 12,8 11,10 11,10 7,8 7,8 6,6 6,6 8,7 8)))"

    // Formerly referred to as c
    test_handle_touch_union::apply
            (
                "uu_case_8", 0, 4,
                "MULTIPOLYGON(((4 8,4 10,6 10,6 8,4 8)),((8 8,8 10,10 10,10 8,8 8)),((7 11,7 13,13 13,13 5,7 5,7 7,11 7,11 11,7 11)))",
                "MULTIPOLYGON(((6 6,6 8,8 8,8 6,6 6)),((6 10,6 12,8 12,8 10,6 10)))"
                );

    // Shoud result in 3 polygons:
    // "MULTIPOLYGON(((4 8,4 10,6 10,6 8,4 8)),((8 8,8 10,10 10,10 8,8 8)),((7 12,7 13,13 13,13 5,7 5,7 6,6 6,6 8,8 8,8 7,11 7,11 11,8 11,8 10,6 10,6 12,7 12)))"

    // Formerly referred to as d
    test_handle_touch_union::apply
            (
                "uu_case_9", 0, 2,
                "MULTIPOLYGON(((2 4,2 6,4 6,4 4,2 4)),((6 4,6 6,8 6,8 4,6 4)),((1 0,1 3,9 3,9 0,1 0)))",
                "MULTIPOLYGON(((0 2,0 4,2 4,2 2,0 2)),((8 2,8 4,10 4,10 2,8 2)),((3 5,3 7,7 7,7 5,3 5)))"
                );
    // Should result in 2 polygons:
    // "MULTIPOLYGON(((2 4,2 6,3 6,3 7,7 7,7 6,8 6,8 4,6 4,6 5,4 5,4 4,2 4)),((1 0,1 2,0 2,0 4,2 4,2 3,8 3,8 4,10 4,10 2,9 2,9 0,1 0)))"

}


template <typename T>
void test_all()
{
    typedef bg::model::point<T, 2, bg::cs::cartesian> point_type;

    typedef bg::model::multi_polygon
            <
            bg::model::polygon<point_type>
            > multi_polygon;

    typedef bg::model::multi_polygon
            <
            bg::model::polygon<point_type, false>
            > multi_polygon_ccw;

    test_geometries<multi_polygon>();
    //    test_geometries<multi_polygon_ccw, true>();
}


int test_main(int, char* [])
{
    test_all<double>();

    return 0;
}