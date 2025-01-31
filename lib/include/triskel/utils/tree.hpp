#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

namespace triskel {

template <typename T>
struct Tree {
    struct Node {
        auto operator->() -> T* { return &label; }
        auto operator->() const -> const T* { return &label; }

        auto operator==(const Node& other) const -> bool {
            return id == other.id;
        }

        /// @brief Is this node the root
        [[nodiscard]] auto is_root() const -> bool {
            return parent_ == nullptr;
        }

        /// @brief Finds the root of this tree
        [[nodiscard]] auto root() -> Node& {
            if (is_root()) {
                return *this;
            }

            return parent_->is_root();
        }

        /// @brief The parent of this node. The node shouldn't be the root
        [[nodiscard]] auto parent() -> Node& {
            assert(!is_root());
            return *parent_;
        }

        /// @brief The parent of this node. The node shouldn't be the root
        [[nodiscard]] auto parent() const -> const Node& {
            assert(!is_root());
            return *parent_;
        }

        /// @brief Is this node the root
        [[nodiscard]] auto children() const -> const std::vector<Node*>& {
            return children_;
        }

        /// @brief Adds a child to this node
        void add_child(Node* node) {
            node->parent_ = this;
            node->depth   = depth + 1;

            children_.push_back(node);
        }

        void remove_child(const Node* node) {
            children_.erase(
                std::remove(children_.begin(), children_.end(), node),
                children_.end());
        }

        /// @brief The data in the label
        T label;

        size_t id;

        // The depth of this node in the tree
        size_t depth = 0;

       private:
        Node* parent_;
        std::vector<Node*> children_;

        template <typename U>
        friend struct Tree;
    };

    [[nodiscard]] auto make_node() -> Node& {
        nodes.push_back(std::make_unique<Node>());
        auto& node = nodes.back();

        node->id = nodes.size() - 1;
        return *node;
    }

    void remove_node(Node* node) {
        assert(node != root);
        assert(node->children_.empty());

        node->parent().remove_child(node);
        nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                                   [node](const std::unique_ptr<Node>& a) {
                                       return a.get() == node;
                                   }),
                    nodes.end());
    }

    Node* root;
    std::vector<std::unique_ptr<Node>> nodes;
};

}  // namespace triskel