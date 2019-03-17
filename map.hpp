#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template <typename _Key, typename _Tp, class _Compare = std::less<_Key>>
class map {
  public:
    typedef pair<const _Key, _Tp> value_type;
  public:
    class iterator;

    class const_iterator;

  private:
    struct __list_node {
        __list_node *prev, *next;
        bool __has_rbt_node_info;

        __list_node() : prev(nullptr), next(nullptr) {
            __has_rbt_node_info = false;
        }

        virtual ~__list_node() = default;
    };

    struct __rbt_node : public __list_node {
        enum color_e {
            BLACK, RED
        };
        enum which_child_e {
            LEFT_CHILD, RIGHT_CHILD
        };
        __rbt_node *father, *left_child, *right_child;
        value_type *value;
        color_e __color;



        explicit __rbt_node(const value_type &value) :
                value(new value_type(value)),
                father(nullptr), left_child(nullptr), right_child(nullptr), __color(RED) {
            __list_node::__has_rbt_node_info = true;
        }

        explicit __rbt_node(const _Key &key) : __rbt_node(value_type(key, _Tp())) {}

        which_child_e which_child() {
            if (father->left_child == this)
                return LEFT_CHILD;
            else
                return RIGHT_CHILD;
        }

        __rbt_node *brother() {
            switch (which_child()) {
                case LEFT_CHILD:
                    return father->right_child;
                case RIGHT_CHILD:
                    return father->left_child;
            }
        }

        static color_e color(__rbt_node *node) {
            if (node == nullptr) return BLACK;
            return node->__color;
        }

        ~__rbt_node() override {
            delete value;
        }
    };

  private:
    __list_node *head, *tail;
    __rbt_node *root;
    _Compare __compare_function;
    size_t __size;

  private:
    void __link_node(__list_node *a, __list_node *b) {
        a->next = b;
        b->prev = a;
    }

    void __link_three_node(__list_node *a, __list_node *b, __list_node *c) {
        __link_node(a, b);
        __link_node(b, c);
    }

    void __right_rotate(__rbt_node *y) {
        __rbt_node *x = y->left_child;
        y->left_child = x->right_child;
        if (x->right_child != nullptr) y->right_child->father = y;
        x->father = y->father;
        if (y->father != nullptr) {
            if (y->father->left_child == y)
                y->father->left_child = x;
            else
                y->father->right_child = x;
        } else {
            root = x;
        }
        x->right_child = y;
        y->father = x;
    }

    void __left_rotate(__rbt_node *x) {
        __rbt_node *y = x->right_child;
        x->right_child = y->left_child;
        if (y->left_child != nullptr) y->left_child->father = x;
        y->father = x->father;
        if (x->father != nullptr) {
            if (x->father->left_child == x)
                x->father->left_child = y;
            else
                x->father->right_child = y;
        } else {
            root = y;
        }
        y->left_child = x;
        x->father = y;
    }

    void __insert_fix(__rbt_node *target) {
        if (target->father == nullptr) {
            target->__color = __rbt_node::BLACK;
        } else if (__rbt_node::color(target->father) == __rbt_node::BLACK) {
            // do nothing
        } else if (__rbt_node::color(target->father->brother()) == __rbt_node::RED) {
            target->father->__color = __rbt_node::BLACK;
            target->father->brother()->__color = __rbt_node::BLACK;
            target->father->father->__color = __rbt_node::RED;
            __insert_fix(target->father->father);
        } else {
            if (target->which_child() != target->father->which_child()) {
                switch (target->which_child()) {
                    case __rbt_node::LEFT_CHILD:
                        __right_rotate(target->father);
                        __left_rotate(target->father);
                        target->__color = __rbt_node::BLACK;
                        target->left_child->__color = __rbt_node::RED;
                        break;
                    case __rbt_node::RIGHT_CHILD:
                        __left_rotate(target->father);
                        __right_rotate(target->father);
                        target->__color = __rbt_node::BLACK;
                        target->right_child->__color = __rbt_node::RED;
                        break;
                }
            } else {
                switch (target->which_child()) {
                    case __rbt_node::LEFT_CHILD:
                        __right_rotate(target->father->father);
                        target->father->__color = __rbt_node::BLACK;
                        target->brother()->__color = __rbt_node::RED;
                        break;
                    case __rbt_node::RIGHT_CHILD:
                        __left_rotate(target->father->father);
                        target->father->__color = __rbt_node::BLACK;
                        target->brother()->__color = __rbt_node::RED;
                        break;
                }
            }
        }
    }

    void __insert(__rbt_node *new_node) {
        if (root == nullptr) {
            new_node->__color = __rbt_node::BLACK;
            root = new_node;
            __link_three_node(head, new_node, tail);
            __size++;
            return;
        }

        __rbt_node *y = root, *x = root;
        while (x != nullptr) {
            y = x;
            if (__compare_function(new_node->value->first, x->value->first))
                x = x->left_child;
            else
                x = x->right_child;
        }

        if (__compare_function(new_node->value->first, y->value->first)) {
            y->left_child = new_node;
            new_node->father = y;
            __link_three_node(y->prev, new_node, y);
        } else {
            y->right_child = new_node;
            new_node->father = y;
            __link_three_node(y, new_node, y->next);
        }

        __insert_fix(new_node);
    }

    void __delete_fix(__rbt_node *node, __rbt_node *father, __rbt_node *brother) {
        if (father == nullptr) {
            node->__color = __rbt_node::BLACK;
            return;
        }
        if (__rbt_node::color(brother) == __rbt_node::RED) {
            father->__color = __rbt_node::RED;
            brother->__color = __rbt_node::BLACK;
            switch (brother->which_child()) {
                case __rbt_node::LEFT_CHILD:
                    __right_rotate(father);
                    brother = brother->left_child;
                    break;
                case __rbt_node::RIGHT_CHILD:
                    __left_rotate(father);
                    brother = brother->right_child;
                    break;
            }
        }
        if (__rbt_node::color(father) == __rbt_node::BLACK && __rbt_node::color(brother) == __rbt_node::BLACK &&
            __rbt_node::color(brother->left_child) == __rbt_node::BLACK &&
            __rbt_node::color(brother->right_child) == __rbt_node::BLACK) {
            brother->__color = __rbt_node::RED;
            __delete_fix(father, father->father, father->father == nullptr ? nullptr : father->brother());
            return;
        }
        if (__rbt_node::color(father) == __rbt_node::RED && __rbt_node::color(brother) == __rbt_node::BLACK &&
            (brother->left_child == nullptr || brother->left_child->__color == __rbt_node::BLACK) &&
            (brother->right_child == nullptr || brother->right_child->__color == __rbt_node::BLACK)) {
            father->__color = __rbt_node::BLACK;
            brother->__color = __rbt_node::RED;
            return;
        }
        if (brother->which_child() == __rbt_node::LEFT_CHILD &&
            __rbt_node::color(brother->left_child) == __rbt_node::BLACK) {
            brother->right_child->__color = __rbt_node::BLACK;
            brother->__color = __rbt_node::RED;
            __left_rotate(brother);
            brother = brother->father;
        }
        if (brother->which_child() == __rbt_node::RIGHT_CHILD &&
            __rbt_node::color(brother->right_child) == __rbt_node::BLACK) {
            brother->left_child->__color = __rbt_node::BLACK;
            brother->__color = __rbt_node::RED;
            __right_rotate(brother);
            brother = brother->father;
        }
        if (__rbt_node::color(father) == __rbt_node::RED) {
            father->__color = __rbt_node::BLACK;
            brother->__color = __rbt_node::RED;
        }
        switch (brother->which_child()) {
            case __rbt_node::LEFT_CHILD:
                brother->left_child->__color = __rbt_node::BLACK;
                __right_rotate(father);
                break;
            case __rbt_node::RIGHT_CHILD:
                brother->right_child->__color = __rbt_node::BLACK;
                break;
        }
    }

    void __erase_leaf(__rbt_node *node) {
        __link_node(node->prev, node->next);

        __rbt_node *father = node->father;
        __rbt_node *child = node->left_child == nullptr ? node->right_child : node->left_child;
        if (father == nullptr && child == nullptr) {
            root = nullptr;
            delete node;
            return;
        }
        if (father == nullptr) {
            child->father = nullptr;
            root = child;
            child->__color = __rbt_node::BLACK;
            delete node;
            return;
        }
        // father exists
        switch (node->which_child()) {
            case __rbt_node::LEFT_CHILD:
                father->left_child = child;
                break;
            case __rbt_node::RIGHT_CHILD:
                father->right_child = child;
                break;
        }
        if (child != nullptr) child->father = father;
        if (__rbt_node::color(node) == __rbt_node::RED) {
            delete node;
            return;
        }
        // node is black, brother exists
        __rbt_node *brother = node->brother();
        // assert(brother != nullptr);
        if (__rbt_node::color(child) == __rbt_node::RED) {
            child->__color = __rbt_node::BLACK;
            delete node;
            return;
        }
        delete node;
        __delete_fix(child, father, brother);
    }

    void __erase(__rbt_node *node) {
        if (node->left_child == nullptr || node->right_child == nullptr) {
            __erase_leaf(node);
            return;
        }
        __rbt_node *replace_node = node->left_child;
        while (replace_node->right_child != nullptr)
            replace_node = replace_node->right_child;

        node->value = replace_node->value;
        replace_node->value = nullptr;

        __erase_leaf(replace_node);
    }

    __rbt_node *__copy_subtree(__rbt_node *node, __rbt_node *father) {
        if (node == nullptr) return nullptr;
        auto new_node = new __rbt_node(*(node->value));
        new_node->__color = node->__color;
        new_node->father = father;
        new_node->left_child = __copy_subtree(node->left_child, new_node);
        new_node->right_child = __copy_subtree(node->right_child, new_node);
        return new_node;
    }

    void __link_list(__rbt_node *node, typename __rbt_node::which_child_e which_child) {
        switch (which_child) {
            case __rbt_node::LEFT_CHILD:
                __link_three_node(node->father->prev, node, node->father);
                break;
            case __rbt_node::RIGHT_CHILD:
                __link_three_node(node->father, node, node->father->next);
                break;
        }
        if (node->left_child != nullptr) __link_list(node->left_child, __rbt_node::LEFT_CHILD);
        if (node->right_child != nullptr) __link_list(node->right_child, __rbt_node::RIGHT_CHILD);
    }

  public:
    map() : head(new __list_node), tail(new __list_node), root(nullptr), __compare_function(_Compare()), __size(0) {
        __link_node(head, tail);
    }

    // TODO
    map(const map &other) : head(new __list_node), tail(new __list_node),
                            root(__copy_subtree(other.root, nullptr)),
                            __compare_function(_Compare()), __size(other.__size) {
        if (root == nullptr) {
            __link_node(head, tail);
        } else {
            __link_three_node(head, root, tail);
            if (root->left_child != nullptr) __link_list(root->left_child, __rbt_node::LEFT_CHILD);
            if (root->right_child != nullptr) __link_list(root->left_child, __rbt_node::RIGHT_CHILD);
        }
    }

    map &operator=(const map &other) {
        if (this == &other) return *this;
        clear();
        delete head;
        delete tail;
        head = new __list_node;
        tail = new __list_node;
        root = __copy_subtree(other.root, nullptr);
        __size = other.__size;
        if (root == nullptr) {
            __link_node(head, tail);
        } else {
            __link_three_node(head, root, tail);
            if (root->left_child != nullptr) __link_list(root->left_child, __rbt_node::LEFT_CHILD);
            if (root->right_child != nullptr) __link_list(root->right_child, __rbt_node::RIGHT_CHILD);
        }
        return *this;
    }

    ~map() {
        clear();
        delete head;
        delete tail;
    }

  public:
    class iterator {
        friend const_iterator;

        friend void map::erase(iterator);

      public:
        using value_type = pair<const _Key, _Tp>;
        using pointer = value_type *;
        using reference = value_type &;

      private:
        __list_node *node;

      public:
        iterator() : node(nullptr) {}

        explicit iterator(__list_node *node) : node(node) {}

        iterator(const iterator &other) = default;

        const iterator operator++(int) {
            if (node->next == nullptr) throw invalid_iterator();
            iterator temp = *this;
            node = node->next;
            return temp;
        }

        iterator &operator++() {
            if (node == nullptr) throw invalid_iterator();
            node = node->next;
            return *this;
        }

        const iterator operator--(int) {
            if (node == nullptr) throw invalid_iterator();
            iterator temp = *this;
            node = node->prev;
            return temp;
        }

        iterator &operator--() {
            if (node == nullptr) throw invalid_iterator();
            node = node->prev;
            return *this;
        }

        value_type &operator*() const {
            if (node == nullptr) throw invalid_iterator();
            auto __rbt_node_pointer = (__rbt_node *) node;
            return *(__rbt_node_pointer->value);
        }

        value_type *operator->() const noexcept {
            if (node == nullptr) throw invalid_iterator();
            if (!node->__has_rbt_node_info) throw invalid_iterator();
            auto __rbt_node_pointer = (__rbt_node *) node;
            return __rbt_node_pointer->value;
        }

        bool operator==(const iterator &rhs) const {
            return node == rhs.node;
        }

        bool operator==(const const_iterator &rhs) const {
            return node == rhs.node;
        }

        bool operator!=(const iterator &rhs) const {
            return node != rhs.node;
        }

        bool operator!=(const const_iterator &rhs) const {
            return node != rhs.node;
        }
    };

    class const_iterator {
        friend iterator;

      public:
        typedef pair<const _Key, _Tp> value_type;

      private:
        __list_node *node;

      public:
        const_iterator() : node(nullptr) {}

        explicit const_iterator(__list_node *node) : node(node) {}

        const_iterator(const const_iterator &other) = default;

        explicit const_iterator(const iterator &other) : node(other.node) {}

        const const_iterator operator++(int) {
            if (node->next == nullptr) throw invalid_iterator();
            const_iterator temp = *this;
            node = node->next;
            return temp;
        }

        const_iterator &operator++() {
            if (node == nullptr) throw invalid_iterator();
            node = node->next;
            return *this;
        }

        const const_iterator operator--(int) {
            if (node == nullptr) throw invalid_iterator();
            const_iterator temp = *this;
            node = node->prev;
            return temp;
        }

        const_iterator &operator--() {
            if (node == nullptr) throw invalid_iterator();
            node = node->prev;
            return *this;
        }

        const value_type &operator*() const {
            if (node == nullptr) throw invalid_iterator();
            if (!node->__has_rbt_node_info) throw invalid_iterator();
            auto __rbt_node_pointer = (__rbt_node *) node;
            return *(__rbt_node_pointer->value);
        }

        const value_type *operator->() const noexcept {
            if (node == nullptr) throw invalid_iterator();
            if (!node->__has_rbt_node_info) throw invalid_iterator();
            auto __rbt_node_pointer = (__rbt_node *) node;
            return __rbt_node_pointer->value;
        }

        bool operator==(const iterator &rhs) const {
            return node == rhs.node;
        }

        bool operator==(const const_iterator &rhs) const {
            return node == rhs.node;
        }

        bool operator!=(const iterator &rhs) const {
            return node != rhs.node;
        }

        bool operator!=(const const_iterator &rhs) const {
            return node != rhs.node;
        }
    };

  public:
    iterator begin() {
        return iterator(head->next);
    }

    const_iterator cbegin() const {
        return const_iterator(head->next);
    }

    iterator end() {
        return iterator(tail);
    }

    const_iterator cend() const {
        return const_iterator(tail);
    }

    iterator find(const _Key &key) {
        __rbt_node *cur = root;
        while (cur != nullptr) {
            if (__compare_function(key, cur->value->first))
                cur = cur->left_child;
            else if (__compare_function(cur->value->first, key))
                cur = cur->right_child;
            else
                return iterator(cur);
        }
        return end();
    }

    const_iterator find(const _Key &key) const {
        __rbt_node *cur = root;
        while (cur != nullptr) {
            if (__compare_function(key, cur->value->first))
                cur = cur->left_child;
            else if (__compare_function(cur->value->first, key))
                cur = cur->right_child;
            else
                return const_iterator(cur);
        }
        return cend();
    }

  public:
    /**
     * access specified element with bounds checking
     * Returns a reference to the mapped value of the element with key equivalent to key.
     * If no such element exists, an exception of type `index_out_of_bound'
     */
    _Tp &at(const _Key &key) {
        iterator it = find(key);
        if (it == end()) throw index_out_of_bound();
        return it->second;
    }

    const _Tp &at(const _Key &key) const {
        const_iterator it = find(key);
        if (it == cend()) throw index_out_of_bound();
        return it->second;
    }

    // TODO
    /**
     * insert an element.
     * return a pair, the first of the pair is
     *   the iterator to the new element (or the element that prevented the insertion),
     *   the second one is true if insert successfully, or false.
     */
    pair<iterator, bool> insert(const pair<_Key, _Tp> &value) {
        iterator it = find(value.first);
        if (it == end()) {
            auto new_node = new __rbt_node(value);
            __insert(new_node);
            return {iterator(new_node), true};
        } else {
            return {it, false};
        }
    }

    /**
     * access specified element
     * Returns a reference to the value that is mapped to a key equivalent to key,
     *   performing an insertion if such key does not already exist.
     */
    _Tp &operator[](const _Key &key) {
        try {
            return at(key);
        } catch (index_out_of_bound &) {
            auto new_node = new __rbt_node(key);
            __insert(new_node);
            return new_node->value->second;
        }
    }

    /**
     * behave like at() throw index_out_of_bound if such key does not exist.
     */
    const _Tp &operator[](const _Key &key) const {
        return at(key);
    }

    /**
     * checks whether the container is empty
     * return true if empty, otherwise false.
     */
    bool empty() const {
        return __size == 0;
    }

    /**
     * returns the number of elements.
     */
    size_t size() const {
        return __size;
    }

    /**
     * clears the contents
     */
    void clear() {
        __size = 0;
        __list_node *cur = head->next;
        while (cur != tail) {
            __list_node *temp = cur;
            cur = cur->next;
            delete temp;
        }
    }

    /**
     * erase the element at pos.
     *
     * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
     */
    void erase(iterator pos) {
        if (pos.node == nullptr) throw invalid_iterator();
        if (!pos.node->__has_rbt_node_info) throw invalid_iterator();
        __erase((__rbt_node *) pos.node);
    }

    /**
     * Returns the number of elements with key
     *   that compares equivalent to the specified argument,
     *   which is either 1 or 0
     *     since this container does not allow duplicates.
     * The default method of check the equivalence is !(a < b || b > a)
     */
    size_t count(const _Key &key) const {
        if (find(key) == cend()) return 0;
        return 1;
    }
};

}

#endif
