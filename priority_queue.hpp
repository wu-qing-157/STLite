#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {

/**
 * @brief   A priority_queue supporting merging
 *
 * This container supports following operations in O(log n) time: adding element; querying the top element;
 * removing the top element; merging two priority_queues.
 *
 * This implementation uses "leftist" as its internal structure.
 *
 * @tparam  T       The type of elements
 * @tparam  Compare The class used to compare elements. The instance of @code{Compare} must
 *                  implement operator()(T, T), which returns true iff the first parameter is smaller than the second.
 *                  @code{std::less<T>} is used by default.
 */
template <typename T, class Compare = std::less<T>>
class priority_queue {
  private:
    /**
     * @brief   A node in the leftist
     *
     * @param   value   The element stored in this node
     * @param   dist    The distance to the nearest node who have less than two child
     */
    struct leftist_node {
        leftist_node *left_child, *right_child;
        T value;
        int dist;

        explicit leftist_node(const T &value) : left_child(nullptr), right_child(nullptr), value(value), dist(0) {}

        /**
         * @brief   merge two subtrees on leftist
         * @return  A pointer to the root of the result subtree
         */
        static leftist_node *join(leftist_node *a, leftist_node *b) {
            if (b == nullptr) return a;
            if (a == nullptr) return b;

            // make sure a.value > b.value
            if (Compare()(a->value, b->value))
                std::swap(a, b);

            a->right_child = join(a->right_child, b);

            // keep the properties of leftist
            if (a->left_child != nullptr && b->right_child != nullptr && a->left_child->dist < a->right_child->dist)
                std::swap(a->left_child, a->right_child);
            if (a->left_child == nullptr && b->right_child != nullptr)
                std::swap(a->left_child, a->right_child);

            // re-calculate dist
            a->dist = a->right_child == nullptr ? 0 : a->right_child->dist + 1;

            return a;
        }

        /**
         * @brief   copy a subtree on leftist
         * @return  A pointer to the root of the result subtree
         */
        static leftist_node *_copy_subtree(const leftist_node *src) {
            if (src == nullptr) return nullptr;
            auto new_node = new leftist_node(src->value);
            new_node->dist = src->dist;
            new_node->left_child = _copy_subtree(src->left_child);
            new_node->right_child = _copy_subtree(src->right_child);
            return new_node;
        }

        /**
         * @brief   free the memory used by a subtree on leftist
         */
        static void _delete_subtree(leftist_node *node) {
            if (node == nullptr) return;
            _delete_subtree(node->left_child);
            _delete_subtree(node->right_child);
            delete node;
        }
    };

  private:
    /**
     * @param   leftist_node    The root of the leftist bound for this priority_queue
     * @param   _size           storage the size of this priority_queue
     */
    leftist_node *root;
    size_t _size;

  public:
    /**
     * @brief   Default constructor, which constructs a @code{priority_queue} with no elements
     */
    priority_queue() : root(nullptr), _size(0) {}

    /**
     * @brief   Copy constructor
     */
    priority_queue(const priority_queue &other) : _size(other._size) {
        root = leftist_node::_copy_subtree(other.root);
    }

    /**
     * @brief   Destructor
     */
    ~priority_queue() {
        leftist_node::_delete_subtree(root);
    };

    /**
     * @brief   Assignment operator
     */
    priority_queue &operator=(const priority_queue &other) {
        if (this == &other) return *this;
        _size = other._size;
        leftist_node::_delete_subtree(root);
        root = leftist_node::_copy_subtree(other.root);
        return *this;
    }

    /**
     * @brief   get the top element of the priority_queue
     *
     * @return  a const reference of the top element
     *
     * @throw   container_is_empty  if the priority_queue is empty
     */
    const T &top() const {
        if (empty()) throw container_is_empty();
        return root->value;
    }

    /**
     * @brief   push new element to the priority queue.
     */
    void push(const T &e) {
        root = leftist_node::join(root, new leftist_node(e));
        _size++;
    }

    /**
     * @brief   remove the top element.
     * @throw   container_is_empty  if the priority_queue is empty
     */
    void pop() {
        if (empty()) throw container_is_empty();
        auto old_root = root;
        root = leftist_node::join(root->left_child, root->right_child);
        delete old_root;
        _size--;
    }

    /**
     * @brief   get the number of elements
     */
    size_t size() const {
        return _size;
    }

    /**
     * @brief   check if the container is empty
     */
    bool empty() const {
        return _size == 0;
    }

    /**
     * @brief   merge another priority_queue
     *
     * Merge another priority_queue into this priority_queue. Note that the other priority will
     * become empty after this operation.
     */
    void merge(priority_queue &other) {
        _size += other._size;
        other._size = 0;

        root = leftist_node::join(root, other.root);
        other.root = nullptr;
    }
};

}

#endif
