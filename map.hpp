#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<typename _Key, typename _Tp, class _Compare = std::less<_Key>>
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
        enum __color {
            BLACK, RED
        };
        __rbt_node *father, *left_child, *right_child;
        value_type *value;
        __color color;

        explicit __rbt_node(const _Key &key) :
                value(new value_type(key, _Tp())),
                father(nullptr), left_child(nullptr), right_child(nullptr), color(RED) {
            __list_node::__has_rbt_node_info = true;
        }

        explicit __rbt_node(const value_type &value) :
                value(new value_type(value)),
                father(nullptr), left_child(nullptr), right_child(nullptr), color(RED) {
            __list_node::__has_rbt_node_info = true;
        }

        bool __is_left_child() {
            return father->left_child == this;
        }

        __rbt_node *brother() {
            if (__is_left_child())
                return father->right_child;
            else
                return father->left_child;
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
            target->color = __rbt_node::BLACK;
        } else if (target->father->color == __rbt_node::BLACK) {
            // do nothing
        } else if (target->father->brother()->color == __rbt_node::RED) {
            target->father->color = __rbt_node::BLACK;
            target->father->brother()->color = __rbt_node::BLACK;
            target->father->father->color = __rbt_node::RED;
            __insert_fix(target->father->father);
        } else {
            if (target->__is_left_child() != target->father->__is_left_child()) {
                if (target->__is_left_child()) {
                    __right_rotate(target->father);
                    __left_rotate(target->father);
                } else {
                    __left_rotate(target->father);
                    __right_rotate(target->father);
                }
            } else {
                if (target->__is_left_child()) {
                    __right_rotate(target->father->father);
                } else {
                    __left_rotate(target->father->father);
                }
            }
        }
    }

    pair<iterator, bool> __insert(__rbt_node *new_node) {
        if (root == nullptr) {
            new_node->color = __rbt_node::BLACK;
            root = new_node;
            head->next = tail->prev = new_node;
            __size++;
            return pair<__rbt_node *, bool>(new_node, true);
        }

        __rbt_node *y = root, *x = root;
        while (x != nullptr) {
            y = x;
            if (__compare_function(new_node->value->first, x->value->first))
                x = x->left_child;
            else if (__compare_function(x->value->first, new_node->value->first))
                x = x->right_child;
            else
                return pair<__rbt_node *, bool>(x, false);
        }

        if (__compare_function(new_node->value->first, y->value->first)) {
            y->left_child = new_node;
            new_node->father = y;

            y->prev->next = new_node;
            new_node->prev = y->prev;
            y->prev = new_node;
            new_node->next = y;
        } else {
            y->right_child = new_node;
            new_node->father = y;

            y->next->prev = new_node;
            new_node->next = y->next;
            y->next = new_node;
            new_node->prev = y;
        }

        __insert_fix(y);

        return pair<iterator, bool>(iterator(new_node), true);
    }

    void __erase_leaf(__rbt_node *node) {
        __rbt_node *child = node->left_child == nullptr ? node->right_child : node->left_child;

        child->father = nullptr;
        delete node;

        if (node->color == __rbt_node::RED) {
            // do nothing
        } else if (child->color == __rbt_node::RED) {
            child->color = __rbt_node::BLACK;
        } else if (child->father == nullptr) {
            // do nothing
        } else if () {

        }
    }

    void __erase(__rbt_node *node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;

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

  public:
    map() : head(new __list_node), tail(new __list_node), root(nullptr), __compare_function(_Compare()), __size(0) {
        head->next = tail;
        tail->prev = head;
    }

    // TODO
    map(const map &other) = default;

    // TODO
    /**
     * TODO assignment operator
     */
    map &operator=(const map &other) {}

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
        typedef pair<const _Key, _Tp> value_type;

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
    pair<iterator, bool> insert(const pair<_Key, _Tp> &value) {}

    /**
     * access specified element
     * Returns a reference to the value that is mapped to a key equivalent to key,
     *   performing an insertion if such key does not already exist.
     */
    _Tp &operator[](const _Key &key) {
        try {
            return at(key);
        } catch (index_out_of_bound &) {
            return __insert(new __rbt_node(key)).first->second;
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
