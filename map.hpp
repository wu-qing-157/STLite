#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template <typename K, typename V, typename __Compare = std::less<K>>
class map {

  public:
    using value_type = pair<const K, V>;

  public:
    class iterator;

    class const_iterator;

  private:

    struct rbt_node {
        using which_e = bool;
        enum color_e {
            RED, BLACK
        };
        rbt_node *prev, *next;
        rbt_node *father, *child[2];
        which_e which;
        color_e color;
        value_type *value;

        // used for head, tail, unnecessary to initialize components
        rbt_node() { value = nullptr; }

        ~rbt_node() {
            delete value;
        }

        // used for clone, clone the subtree
        // only update father's child component and previous/next's next/previous component
        // component child will be updated when constructing children
        rbt_node(rbt_node *other, rbt_node *father, rbt_node *prev, rbt_node *next) :
                value(new value_type(*(other->value))), father(father), prev(prev), next(next),
                which(other->which), color(other->color) {
            child[LEFT] = child[RIGHT] = nullptr;
            prev->next = next->prev = this;
            if (other->child[LEFT] != nullptr)
                child[LEFT] = new rbt_node(other->child[LEFT], this, prev, this);
            else
                child[LEFT] = nullptr;
            if (other->child[RIGHT] != nullptr)
                child[RIGHT] = new rbt_node(other->child[RIGHT], this, this, next);
            else
                child[RIGHT] = nullptr;
        }

        void update_nearby() {
            next->prev = prev->next = this;
            if (father != nullptr) father->child[which] = this;
            if (child[LEFT] != nullptr) {
                child[LEFT]->father = this;
                child[LEFT]->which = LEFT;
            }
            if (child[RIGHT] != nullptr) {
                child[RIGHT]->father = this;
                child[RIGHT]->which = RIGHT;
            }
        }

        rbt_node(const value_type &value, rbt_node *prev, rbt_node *next,
                 rbt_node *father, rbt_node *left_child, rbt_node *right_child, which_e which) :
                value(new value_type(value)), prev(prev), next(next), father(father), which(which), color(RED) {
            child[LEFT] = left_child, child[RIGHT] = right_child;
            update_nearby();
        }

        rbt_node(const K &key, rbt_node *prev, rbt_node *next,
                 rbt_node *father, rbt_node *left_child, rbt_node *right_child, which_e which) :
                rbt_node(value_type(key, V()), prev, next, father, left_child, right_child, which) {}

        rbt_node *brother() {
            return father->child[!which];
        }
    };

  private:
    rbt_node *head, *tail, *root;
    __Compare comparing_function;
    size_t __size;

    static constexpr typename rbt_node::color_e BLACK = rbt_node::BLACK;
    static constexpr typename rbt_node::color_e RED = rbt_node::RED;
    static constexpr typename rbt_node::which_e LEFT = false;
    static constexpr typename rbt_node::which_e RIGHT = true;

    static void link_node(rbt_node *a, rbt_node *b) {
        a->next = b;
        b->prev = a;
    }

    template <typename T>
    static void __swap(T &a, T &b) {
        T temp = a;
        a = b;
        b = temp;
    }

    void swap_node(rbt_node *a, rbt_node *b) {
        if (root == a) root = b;
        else if (root == b) root = a;
        __swap(a->prev, b->prev);
        __swap(a->next, b->next);
        __swap(a->father, b->father);
        __swap(a->child[LEFT], b->child[LEFT]);
        __swap(a->child[RIGHT], b->child[RIGHT]);
        __swap(a->color, b->color);
        __swap(a->which, b->which);
        if (a->prev == a) a->prev = b;
        if (a->next == a) a->next = b;
        if (a->father == a) a->father = b;
        if (a->child[LEFT] == a) a->child[LEFT] = b;
        if (a->child[RIGHT] == a) a->child[RIGHT] = b;
        if (b->prev == b) b->prev = a;
        if (b->next == b) b->next = a;
        if (b->father == b) b->father = a;
        if (b->child[LEFT] == b) b->child[LEFT] = a;
        if (b->child[RIGHT] == b) b->child[RIGHT] = a;
        a->update_nearby();
        b->update_nearby();
    }

    void rotate(rbt_node *x, typename rbt_node::which_e which) {
        rbt_node *y = x->child[!which];
        if (root == x) root = y;
        x->child[!which] = y->child[which];
        y->father = x->father;
        y->which = x->which;
        x->father = y;
        x->which = which;
        x->update_nearby();
        y->update_nearby();
    }

    void insert_fix(rbt_node *target) {
        rbt_node *father = target->father;
        if (father == nullptr) {
            target->color = BLACK;
            return;
        }
        if (father->color == BLACK) return;
        rbt_node *grandpa = father->father; // non-null
        rbt_node *uncle = target->father->brother();
        if (uncle == nullptr || uncle->color == BLACK) {
            if (target->which == father->which) {
                father->color = BLACK;
                grandpa->color = RED;
                rotate(grandpa, !(target->which));
            } else {
                target->color = BLACK;
                grandpa->color = RED;
                rotate(father, !(target->which));
                rotate(grandpa, !(target->which));
            }
        } else {
            father->color = uncle->color = BLACK;
            grandpa->color = RED;
            insert_fix(grandpa);
        }
    }

    void erase_fix(rbt_node *target, bool recursive = false) {
        if (target->color == RED && !recursive) return;
        rbt_node *child = target->child[target->child[LEFT] == nullptr];
        if (child != nullptr && child->color == RED && !recursive) {
            child->color = BLACK;
            return;
        }
        if (root == target) {
            target->color = BLACK;
            return;
        }
        rbt_node *father = target->father; // non-null
        rbt_node *brother = target->brother(); // non-null
        rbt_node **cousin = brother->child;
        if (father->color == BLACK && brother->color == BLACK &&
            (cousin[LEFT] == nullptr || cousin[LEFT]->color == BLACK) &&
            (cousin[RIGHT] == nullptr || cousin[RIGHT]->color == BLACK)) {
            brother->color = RED;
            erase_fix(father, true);
            return;
        }
        if (brother->color == RED) {
            father->color = RED;
            brother->color = BLACK;
            rotate(father, target->which);
            brother = target->brother();
            cousin = brother->child;
        }
        if (father->color == RED && brother->color == BLACK &&
            (cousin[LEFT] == nullptr || cousin[LEFT]->color == BLACK) &&
            (cousin[RIGHT] == nullptr || cousin[RIGHT]->color == BLACK)) {
            father->color = BLACK;
            brother->color = RED;
            return;
        }
        if (cousin[!(target->which)] == nullptr || cousin[!(target->which)]->color == BLACK) {
            cousin[target->which]->color = BLACK;
            brother->color = RED;
            rotate(brother, !(target->which));
            brother = target->brother();
            cousin = brother->child;
        }
        __swap(father->color, brother->color);
        cousin[!(target->which)]->color = BLACK;
        rotate(father, target->which);
    }

    template <typename U>
    pair<rbt_node *, bool> __insert(const K &key, const U &value) {
        if (root == nullptr) {
            auto new_node = new rbt_node(value, head, tail, nullptr, nullptr, nullptr, LEFT);
            root = new_node;
            __size++;
            insert_fix(new_node);
            return {new_node, true};
        }
        rbt_node *cur_node = root;
        typename rbt_node::which_e which;
        while (true) {
            which = comparing_function(cur_node->value->first, key);
            if (!which && !comparing_function(key, cur_node->value->first)) return {cur_node, false};
            if (cur_node->child[which] == nullptr) break;
            cur_node = cur_node->child[which];
        }
        auto new_node = new rbt_node(value, which ? cur_node : cur_node->prev, which ? cur_node->next : cur_node,
                                     cur_node, nullptr, nullptr, which);
        __size++;
        insert_fix(new_node);
        return {new_node, true};
    }

    pair<rbt_node *, bool> __insert(const value_type &value) {
        return __insert(value.first, value);
    }

    rbt_node *__insert(const K &key) {
        return __insert(key, key).first;
    }

    void erase(rbt_node *target) {
        __size--;
        if (target->child[LEFT] != nullptr && target->child[RIGHT] != nullptr) {
            swap_node(target, target->next);
        }
        erase_fix(target);
        link_node(target->prev, target->next);
        rbt_node *child = target->child[target->child[LEFT] == nullptr];
        if (target == root) root = child;
        else target->father->child[target->which] = child;
        if (child != nullptr) {
            child->father = target->father;
            child->which = target->which;
        }
        delete target;
    }

  public:
    map() : head(new rbt_node), tail(new rbt_node), root(nullptr), comparing_function(), __size(0) {
        head->next = tail;
        tail->prev = head;
    }

    map(const map &other) : map() {
        if (other.__size == 0) return;
        __size = other.__size;
        root = new rbt_node(other.root, nullptr, head, tail);
    }

    map &operator=(const map &other) {
        if (this == &other) return *this;
        clear();
        __size = other.__size;
        if (other.__size == 0) return *this;
        root = new rbt_node(other.root, nullptr, head, tail);
        return *this;
    }

    ~map() {
        clear();
        delete head;
        delete tail;
    }

  public:
    void clear() {
        __size = 0;
        auto cur = head->next;
        while (cur != tail) {
            auto temp = cur;
            cur = cur->next;
            delete temp;
        }
        head->next = tail;
        tail->prev = head;
        root = nullptr;
    }

    bool empty() const {
        return __size == 0;
    }

    size_t size() const {
        return __size;
    }

    iterator begin() {
        return iterator(this, head->next);
    }

    const_iterator cbegin() const {
        return const_iterator(this, head->next);
    }

    iterator end() {
        return iterator(this, tail);
    }

    const_iterator cend() const {
        return const_iterator(this, tail);
    }

    iterator find(const K &key) {
        auto cur = root;
        while (cur != nullptr) {
            if (comparing_function(key, cur->value->first))
                cur = cur->child[LEFT];
            else if (comparing_function(cur->value->first, key))
                cur = cur->child[RIGHT];
            else
                return iterator(this, cur);
        }
        return end();
    }

    const_iterator find(const K &key) const {
        auto cur = root;
        while (cur != nullptr) {
            if (comparing_function(key, cur->value->first))
                cur = cur->child[LEFT];
            else if (comparing_function(cur->value->first, key))
                cur = cur->child[RIGHT];
            else
                return const_iterator(this, cur);
        }
        return cend();
    }

    size_t count(const K &key) const {
        return find(key) == cend() ? 0 : 1;
    }

    V &at(const K &key) {
        iterator it = find(key);
        if (it == end()) throw index_out_of_bound();
        return it->second;
    }

    const V &at(const K &key) const {
        const_iterator it = find(key);
        if (it == cend()) throw index_out_of_bound();
        return it->second;
    }

    V &operator[](const K &key) {
        return __insert(key)->value->second;
    }

    const V &operator[](const K &key) const {
        return at(key);
    }

    pair<iterator, bool> insert(const value_type &value) {
        auto result = __insert(value);
        return {iterator(this, result.first), result.second};
    }

    void erase(iterator pos) {
        if (pos.__map != this || pos == end()) throw invalid_iterator();
        erase(pos.node);
    }

  public:
    class iterator {
        friend const_iterator;

        friend void map::erase(iterator);

      public:
        using value_type = pair<const K, V>;
        using reference = value_type &;
        using pointer = value_type *;

      private:
        map *__map;
        rbt_node *node;

      public:
        iterator() : __map(nullptr), node(nullptr) {}

        iterator(map *__map, rbt_node *node) : __map(__map), node(node) {}

        iterator(const iterator &other) = default;

        iterator &operator=(const iterator &other) {
            __map = other.__map;
            node = other.node;
        }

        operator const_iterator() {
            return const_iterator(*this);
        }

        const iterator operator++(int) {
            iterator backup(*this);
            operator++();
            return backup;
        }

        iterator &operator++() {
            if (node == nullptr || node == __map->tail) throw invalid_iterator();
            node = node->next;
            return *this;
        }

        const iterator operator--(int) {
            iterator backup(*this);
            operator--();
            return backup;
        }

        iterator &operator--() {
            if (node == nullptr || node == __map->head->next) throw invalid_iterator();
            node = node->prev;
            return *this;
        }

        reference operator*() const {
            return *operator->();
        }

        pointer operator->() const noexcept {
            if (node == nullptr || node == __map->tail) throw invalid_iterator();
            return node->value;
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
        using value_type = const pair<const K, V>;
        using reference = value_type &;
        using pointer = value_type *;

      private:
        const map *__map;
        const rbt_node *node;

      public:
        const_iterator() : __map(nullptr), node(nullptr) {}

        const_iterator(const map *__map, const rbt_node *node) : __map(__map), node(node) {}

        const_iterator(const const_iterator &other) = default;

        explicit const_iterator(const iterator &other) : __map(other.__map), node(other.node) {}

        const_iterator &operator=(const const_iterator &other) {
            __map = other.__map;
            node = other.node;
        }

        const const_iterator operator++(int) {
            const_iterator backup(*this);
            operator++();
            return backup;
        }

        const_iterator &operator++() {
            if (node == nullptr || node == __map->tail) throw invalid_iterator();
            node = node->next;
            return *this;
        }

        const const_iterator operator--(int) {
            const_iterator backup(*this);
            operator--();
            return backup;
        }

        const_iterator &operator--() {
            if (node == nullptr || node == __map->head->next) throw invalid_iterator();
            node = node->prev;
            return *this;
        }

        reference operator*() const {
            return *operator->();
        }

        pointer operator->() const noexcept {
            if (node == nullptr || node == __map->tail) throw invalid_iterator();
            return node->value;
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
};

}

#endif
