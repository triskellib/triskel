#include <triskel/datatypes/rtree.hpp>

#include <vector>

#include <fmt/printf.h>
#include <gtest/gtest.h>

void test_tree(RTree<char, size_t>& rtree) {
    ASSERT_EQ(rtree.root.children.size(), 1);

    auto& node_r = rtree.root.children.at('r');
    ASSERT_EQ(node_r->radix, std::vector<char>{'r'});
    ASSERT_EQ(node_r->data.size(), 0);
    ASSERT_EQ(node_r->children.size(), 2);

    auto& node_rom = node_r->children.at('o');
    ASSERT_EQ(node_rom->radix, (std::vector<char>{'o', 'm'}));
    ASSERT_EQ(node_rom->data.size(), 0);
    ASSERT_EQ(node_rom->children.size(), 2);

    auto& node_romulus = node_rom->children.at('u');
    ASSERT_EQ(node_romulus->radix,
              (std::vector<char>{'u', 'l', 'u', 's', '\x00'}));
    ASSERT_EQ(node_romulus->data.size(), 1);
    ASSERT_EQ(node_romulus->data.front(), 3);
    ASSERT_EQ(node_romulus->children.size(), 0);

    auto& node_roman = node_rom->children.at('a');
    ASSERT_EQ(node_roman->radix, (std::vector<char>{'a', 'n'}));
    ASSERT_EQ(node_roman->data.size(), 0);
    ASSERT_EQ(node_roman->children.size(), 2);

    auto& node_romane = node_roman->children.at('e');
    ASSERT_EQ(node_romane->radix, (std::vector<char>{'e', '\x00'}));
    ASSERT_EQ(node_romane->data.size(), 1);
    ASSERT_EQ(node_romane->data.front(), 1);
    ASSERT_EQ(node_romane->children.size(), 0);

    auto& node_romanus = node_roman->children.at('u');
    ASSERT_EQ(node_romanus->radix, (std::vector<char>{'u', 's', '\x00'}));
    ASSERT_EQ(node_romanus->data.size(), 1);
    ASSERT_EQ(node_romanus->data.front(), 2);
    ASSERT_EQ(node_romanus->children.size(), 0);

    auto& node_rub = node_r->children.at('u');
    ASSERT_EQ(node_rub->radix, (std::vector<char>{'u', 'b'}));
    ASSERT_EQ(node_rub->data.size(), 0);
    ASSERT_EQ(node_rub->children.size(), 2);

    auto& node_rube = node_rub->children.at('e');
    ASSERT_EQ(node_rube->radix, (std::vector<char>{'e'}));
    ASSERT_EQ(node_rube->data.size(), 0);
    ASSERT_EQ(node_rube->children.size(), 2);

    auto& node_ruben = node_rube->children.at('n');
    ASSERT_EQ(node_ruben->radix, (std::vector<char>{'n', 's', '\x00'}));
    ASSERT_EQ(node_ruben->data.size(), 1);
    ASSERT_EQ(node_ruben->data.front(), 4);
    ASSERT_EQ(node_ruben->children.size(), 0);

    auto& node_ruber = node_rube->children.at('r');
    ASSERT_EQ(node_ruber->radix, (std::vector<char>{'r', '\x00'}));
    ASSERT_EQ(node_ruber->data.size(), 1);
    ASSERT_EQ(node_ruber->data.front(), 5);
    ASSERT_EQ(node_ruber->children.size(), 0);

    auto& node_rubic = node_rub->children.at('i');
    ASSERT_EQ(node_rubic->radix, (std::vector<char>{'i', 'c'}));
    ASSERT_EQ(node_rubic->data.size(), 0);
    ASSERT_EQ(node_rubic->children.size(), 2);

    auto& node_rubicon = node_rubic->children.at('o');
    ASSERT_EQ(node_rubicon->radix, (std::vector<char>{'o', 'n', '\x00'}));
    ASSERT_EQ(node_rubicon->data.size(), 1);
    ASSERT_EQ(node_rubicon->data.front(), 6);
    ASSERT_EQ(node_rubicon->children.size(), 0);

    auto& node_rubicundus = node_rubic->children.at('u');
    ASSERT_EQ(node_rubicundus->radix,
              (std::vector<char>{'u', 'n', 'd', 'u', 's', '\x00'}));
    ASSERT_EQ(node_rubicundus->data.size(), 2);
    ASSERT_EQ(node_rubicundus->children.size(), 0);
}

TEST(RTree, Smoke) {
    auto rtree = RTree<char, size_t>();

    // From https://en.wikipedia.org/wiki/Radix_tree
    rtree.insert(1, "romane");
    rtree.insert(2, "romanus");
    rtree.insert(3, "romulus");
    rtree.insert(4, "rubens");
    rtree.insert(5, "ruber");
    rtree.insert(6, "rubicon");
    rtree.insert(7, "rubicundus");
    rtree.insert(8, "rubicundus");

    rtree.dump();

    test_tree(rtree);
}

TEST(RTree, Smoke2) {
    auto rtree = RTree<char, size_t>();

    // From https://en.wikipedia.org/wiki/Radix_tree
    rtree.insert(8, "rubicundus");
    rtree.insert(7, "rubicundus");
    rtree.insert(6, "rubicon");
    rtree.insert(5, "ruber");
    rtree.insert(4, "rubens");
    rtree.insert(3, "romulus");
    rtree.insert(2, "romanus");
    rtree.insert(1, "romane");

    rtree.dump();

    test_tree(rtree);
}

TEST(RTree, Smoke3) {
    auto rtree = RTree<char, size_t>();

    // From https://en.wikipedia.org/wiki/Radix_tree
    rtree.insert(7, "rubicundus");
    rtree.insert(2, "romanus");
    rtree.insert(5, "ruber");
    rtree.insert(8, "rubicundus");
    rtree.insert(3, "romulus");
    rtree.insert(1, "romane");
    rtree.insert(6, "rubicon");
    rtree.insert(4, "rubens");

    rtree.dump();

    test_tree(rtree);
}
