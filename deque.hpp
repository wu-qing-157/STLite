#ifndef SJTU_DEQUE_HPP
#define SJTU_DEQUE_HPP

#include "exceptions.hpp"

#include <cstddef>

namespace sjtu {

template <typename T>
class deque {
  private:
    struct bucket;

    struct node {
        bucket *__bucket;
        node *prev, *next;
        T *value;

        ~node() {
            delete value;
        }
    };

    struct bucket {
        bucket *prev, *next;
        node *head, *tail;
        size_t size;

        bucket() : head(new node), tail(new node), size(0) {
            head->next = tail;
            tail->prev = head;
        }

        void __split_before(size_t pos) {
            node *new_last = head;
            for (size_t i = 0; i < pos; i++)
                new_last = new_last->next;
            node *new_first = new_last->next;

            auto new_bucket = new bucket;
            new_bucket->next = next;
            new_bucket->prev = this;
            next->prev = new_bucket;
            next = new_bucket;

            new_bucket->tail = tail;
            new_bucket->head = new node;
            new_bucket->head->next = new_first;
            new_first->prev = new_bucket->head;

            tail->prev = new_last;
            new_last->next = tail;

            new_bucket->size = size - pos;
            size = pos;

            // for (auto i = new_bucket->head->next; i != nullptr; i = i->next)
            // i->__bucket = new_bucket;
        }

        void __merge_next() {
            node *__old_tail = tail, *__old_head = next->head;
            __old_tail->prev->next = __old_head->next;
            __old_head->next->prev = __old_tail->prev;
            delete __old_head;
            delete __old_tail;

            size += next->size;

            bucket *__old_bucket = next;
            __old_bucket->next->prev = this;
            next = __old_bucket->next;
            delete __old_bucket;
        }

        static bucket *__copy_bucket(bucket *other) {
            auto new_bucket = new bucket;
            for (auto old_node = other->head; old_node != nullptr; old_node = old_node->next) {
                auto new_node = new node;
                new_node->value = new T(old_node->value);
                new_bucket->tail->prev->next = new_node;
                new_node->prev = new_bucket->tail->prev;
                new_node->next = new_bucket->tail;
                new_bucket->tail->prev = new_node;
            }
            return new_bucket;
        }

        void clear() {
            node *cur = head->next;
            while (cur != tail) {
                node *next_node = cur->next;
                delete cur;
                cur = next_node;
            }
            size = 0;
        }
    };

  public:
    class const_iterator;

    class iterator;

  private:
    bucket *head, *tail;
    size_t __size;

  public:
    deque() : head(new bucket), tail(new bucket), __size(0) {
        head->next = tail;
        tail->prev = head;
    }

    // TODO
    deque(const deque &other) : head(new bucket), tail(new bucket), __size(other.__size) {
        head->next = tail;
        tail->prev = head;

        for (auto old_bucket = other.head->next; old_bucket != nullptr; old_bucket = old_bucket->next) {
            auto new_bucket = bucket::__copy_bucket(old_bucket);
            tail->prev->next = new_bucket;
            new_bucket->prev = tail->prev;
            tail->prev = new_bucket;
            new_bucket->next = tail;
        }
    }

    // TODO
    ~deque() {
        clear();
        delete head;
        delete tail;
    }

    /**
     * TODO assignment operator
     */
    deque &operator=(const deque &other) {
        if (this == &other) return;
        clear();
        __size = other.__size;
        for (auto old_bucket = other.head->next; old_bucket != nullptr; old_bucket = old_bucket->next) {
            auto new_bucket = bucket::__copy_bucket(old_bucket);
            tail->prev->next = new_bucket;
            new_bucket->prev = tail->prev;
            tail->prev = new_bucket;
            new_bucket->next = tail;
        }
        return *this;
    }

  public:
    /**
     * access specified element with bounds checking
     * throw index_out_of_bound if out of bound.
     */
    T &at(const size_t &pos) {
        if (pos < 0 || pos >= __size) throw index_out_of_bound();
        return operator[](pos);
    }

    const T &at(const size_t &pos) const {
        if (pos < 0 || pos >= __size) throw index_out_of_bound();
        return operator[](pos);
    }

    T &operator[](const size_t &pos) {
        bucket *cur_bucket = head->next;
        size_t cur_pos = 0;
        while (cur_pos + cur_bucket->size <= pos) {
            cur_pos += cur_bucket->size;
            cur_bucket = cur_bucket->next;
        }
        node *cur_node = cur_bucket->head->next;
        while (cur_pos < pos) {
            cur_pos++;
            cur_node = cur_node->next;
        }
        return *cur_node;
    }

    const T &operator[](const size_t &pos) const {
        bucket *cur_bucket = head->next;
        size_t cur_pos = 0;
        while (cur_pos + cur_bucket->size <= pos) {
            cur_pos += cur_bucket->size;
            cur_bucket = cur_bucket->next;
        }
        node *cur_node = cur_bucket->head->next;
        while (cur_pos < pos) {
            cur_pos++;
            cur_node = cur_node->next;
        }
        return *cur_node;
    }

    /**
     * access the first element
     * throw container_is_empty when the container is empty.
     */
    const T &front() const {
        if (__size == 0) throw container_is_empty();
        return *(head->next->head->next);
    }

    /**
     * access the last element
     * throw container_is_empty when the container is empty.
     */
    const T &back() const {
        if (__size == 0) throw container_is_empty();
        return *(tail->prev->tail->prev);
    }

    /**
     * returns an iterator to the beginning.
     */
    iterator begin() {}

    const_iterator cbegin() const {}

    /**
     * returns an iterator to the end.
     */
    iterator end() {}

    const_iterator cend() const {}

    /**
     * checks whether the container is empty.
     */
    bool empty() const {}

    /**
     * returns the number of elements
     */
    size_t size() const {}

    /**
     * clears the contents
     */
    void clear() {
        bucket *cur = head->next;
        while (cur != tail) {
            bucket *next_bucket = cur->next;
            cur->clear();
            delete cur->head;
            delete cur->tail;
            cur = next_bucket;
        }
        head->next = tail;
        tail->prev = head;
        __size = 0;
    }

    /**
     * inserts elements at the specified location in the container.
     * inserts value before pos
     * returns an iterator pointing to the inserted value
     *     throw if the iterator is invalid or it point to a wrong place.
     */
    iterator insert(iterator pos, const T &value) {}

    /**
     * removes specified element at pos.
     * removes the element at pos.
     * returns an iterator pointing to the following element, if pos pointing to the last element, end() will be returned.
     * throw if the container is empty, the iterator is invalid or it points to a wrong place.
     */
    iterator erase(iterator pos) {}

    /**
     * adds an element to the end
     */
    void push_back(const T &value) {}

    /**
     * removes the last element
     *     throw when the container is empty.
     */
    void pop_back() {}

    /**
     * inserts an element to the beginning.
     */
    void push_front(const T &value) {}

    /**
     * removes the first element.
     *     throw when the container is empty.
     */
    void pop_front() {}

  public:
    class iterator {
        friend class const_iterator;

      public:
        using value_type = T;
        using pointer = T *;
        using reference = T &;
        using size_type = size_t;
        using difference_type = std::ptrdiff_t;

      private:
        deque *__deque;
        bucket *__bucket;
        node *__node;

      public:
        explicit iterator(deque *__deque = nullptr, bucket *__bucket = nullptr, node *__node = nullptr) :
                __deque(__deque), __bucket(__bucket), __node(__node) {}

        iterator(const iterator &other) = default;

        explicit iterator(const const_iterator &other) :
                __deque(other.__deque), __bucket(__bucket), __node(__node) {}

        /**
         * return a new iterator which pointer n-next elements
         *   even if there are not enough elements, the behaviour is **undefined**.
         * as well as operator-
         */
        iterator operator+(const difference_type &n) const {
            if (n < 0) return operator-(-n);
            iterator new_iterator(*this);
            difference_type cur_diff = 0;
            while (cur_diff < n && new_iterator.__node->next != new_iterator.__bucket->tail) {
                cur_diff++;
                new_iterator.__node = new_iterator.__node->next;
            }
            if (cur_diff == n) return new_iterator;
            cur_diff++;
            new_iterator.__bucket = new_iterator.__bucket->next;
            while (cur_diff + new_iterator.__bucket->size <= n) {
                cur_diff += new_iterator.__bucket->size;
                new_iterator.__bucket = new_iterator.__bucket->next;
            }
            new_iterator.__node = __bucket->head->next;
            while (cur_diff < n) {
                cur_diff++;
                new_iterator.__node = new_iterator.__node->next;
            }
            return new_iterator;
        }

        iterator operator-(const difference_type &n) const {
            if (n < 0) return operator+(-n);
            iterator new_iterator(*this);
            difference_type cur_diff = 0;
            while (cur_diff < n && new_iterator.__node->prev != new_iterator.__bucket->head) {
                cur_diff++;
                new_iterator.__node = new_iterator.__node->prev;
            }
            if (cur_diff == n) return new_iterator;
            cur_diff++;
            new_iterator.__bucket = new_iterator.__bucket->prev;
            while (cur_diff + new_iterator.__bucket->size <= n) {
                cur_diff += new_iterator.__bucket->size;
                new_iterator.__bucket = new_iterator.__bucket->prev;
            }
            new_iterator.__node = __bucket->tail->prev;
            while (cur_diff < n) {
                cur_diff++;
                new_iterator.__node = new_iterator.__node->prev;
            }
            return new_iterator;
        }

      private:
        size_t pos() const {
            size_t size = 0;
            for (auto cur_node = __node; cur_node->prev != __bucket->head; cur_node = cur_node->prev)
                size++;
            for (auto cur_bucket = __bucket; cur_bucket->prev != __deque->head; cur_bucket = cur_bucket->prev)
                size += cur_bucket->prev->size;
            return size;
        }

      public:
        // return th distance between two iterator,
        // if these two iterators points to different vectors, throw invalid_iterator.
        difference_type operator-(const iterator &rhs) const {
            return pos() - rhs.pos();
        }

        iterator &operator+=(const difference_type &n) {
            iterator result = operator+(n);
            __bucket = result.__bucket;
            __node = result.__bucket;
            return *this;
        }

        iterator &operator-=(const int &n) {
            iterator result = operator-(n);
            __bucket = result.__bucket;
            __node = result.__bucket;
            return *this;
        }

        /**
         * TODO iter++
         */
        iterator operator++(int) {
            iterator new_iterator(*this);
            new_iterator.__node = new_iterator.__node->next;
            if (new_iterator.__node == new_iterator.__bucket->tail) {
                new_iterator.__bucket = new_iterator.__bucket->next;
                new_iterator.__node = new_iterator.__bucket->head->next;
            }
            return new_iterator;
        }

        /**
         * TODO ++iter
         */
        iterator &operator++() {
            __node = __node->next;
            if (__node == __bucket->tail) {
                __bucket = __bucket->next;
                __node = __bucket->head->next;
            }
            return *this;
        }

        /**
         * TODO iter--
         */
        iterator operator--(int) {
            iterator new_iterator(*this);
            new_iterator.__node = new_iterator.__node->prev;
            if (new_iterator.__node == new_iterator.__bucket->head) {
                new_iterator.__bucket = new_iterator.__bucket->prev;
                new_iterator.__node = new_iterator.__bucket->tail->prev;
            }
            return new_iterator;
        }

        /**
         * TODO --iter
         */
        iterator &operator--() {
            __node = __node->next;
            if (__node == __bucket->head) {
                __bucket = __bucket->prev;
                __node = __bucket->tail->prev;
            }
            return *this;
        }

        /**
         * TODO *it
         */
        reference operator*() const {
            return *(__node->value);
        }

        /**
         * TODO it->field
         */
        pointer operator->() const noexcept {
            return __node->value;
        }

        /**
         * a operator to check whether two iterators are same (pointing to the same memory).
         */
        bool operator==(const iterator &rhs) const {
            return __node == rhs.__node;
        }

        bool operator==(const const_iterator &rhs) const {
            return __node == rhs.__node;
        }

        /**
         * some other operator for iterator.
         */
        bool operator!=(const iterator &rhs) const {
            return __node != rhs.__node;
        }

        bool operator!=(const const_iterator &rhs) const {
            return __node != rhs.__node;
        }
    };

    class const_iterator {
        friend class iterator;

      public:
        using value_type = const T;
        using pointer = const T *;
        using reference = const T &;
        using size_type = size_t;
        using difference_type = std::ptrdiff_t;

      private:
        deque *__deque;
        bucket *__bucket;
        node *__node;

      public:
        explicit const_iterator(deque *__deque = nullptr, bucket *__bucket = nullptr, node *__node = nullptr) :
                __deque(__deque), __bucket(__bucket), __node(__node) {}

        const_iterator(const const_iterator &other) = default;

        /**
         * return a new iterator which pointer n-next elements
         *   even if there are not enough elements, the behaviour is **undefined**.
         * as well as operator-
         */
        const_iterator operator+(const difference_type &n) const {
            if (n < 0) return operator-(-n);
            const_iterator new_iterator(*this);
            difference_type cur_diff = 0;
            while (cur_diff < n && new_iterator.__node->next != new_iterator.__bucket->tail) {
                cur_diff++;
                new_iterator.__node = new_iterator.__node->next;
            }
            if (cur_diff == n) return new_iterator;
            cur_diff++;
            new_iterator.__bucket = new_iterator.__bucket->next;
            while (cur_diff + new_iterator.__bucket->size <= n) {
                cur_diff += new_iterator.__bucket->size;
                new_iterator.__bucket = new_iterator.__bucket->next;
            }
            new_iterator.__node = __bucket->head->next;
            while (cur_diff < n) {
                cur_diff++;
                new_iterator.__node = new_iterator.__node->next;
            }
            return new_iterator;
        }

        const_iterator operator-(const difference_type &n) const {
            if (n < 0) return operator+(-n);
            const_iterator new_iterator(*this);
            difference_type cur_diff = 0;
            while (cur_diff < n && new_iterator.__node->prev != new_iterator.__bucket->head) {
                cur_diff++;
                new_iterator.__node = new_iterator.__node->prev;
            }
            if (cur_diff == n) return new_iterator;
            cur_diff++;
            new_iterator.__bucket = new_iterator.__bucket->prev;
            while (cur_diff + new_iterator.__bucket->size <= n) {
                cur_diff += new_iterator.__bucket->size;
                new_iterator.__bucket = new_iterator.__bucket->prev;
            }
            new_iterator.__node = __bucket->tail->prev;
            while (cur_diff < n) {
                cur_diff++;
                new_iterator.__node = new_iterator.__node->prev;
            }
            return new_iterator;
        }

      private:
        size_t pos() const {
            size_t size = 0;
            for (auto cur_node = __node; cur_node->prev != __bucket->head; cur_node = cur_node->prev)
                size++;
            for (auto cur_bucket = __bucket; cur_bucket->prev != __deque->head; cur_bucket = cur_bucket->prev)
                size += cur_bucket->prev->size;
            return size;
        }

      public:
        // return th distance between two iterator,
        // if these two iterators points to different vectors, throw invalid_iterator.
        difference_type operator-(const const_iterator &rhs) const {
            return pos() - rhs.pos();
        }

        const_iterator &operator+=(const difference_type &n) {
            const_iterator result = operator+(n);
            __bucket = result.__bucket;
            __node = result.__bucket;
            return *this;
        }

        const_iterator &operator-=(const int &n) {
            const_iterator result = operator-(n);
            __bucket = result.__bucket;
            __node = result.__bucket;
            return *this;
        }

        /**
         * TODO iter++
         */
        const_iterator operator++(int) {
            const_iterator new_iterator(*this);
            new_iterator.__node = new_iterator.__node->next;
            if (new_iterator.__node == new_iterator.__bucket->tail) {
                new_iterator.__bucket = new_iterator.__bucket->next;
                new_iterator.__node = new_iterator.__bucket->head->next;
            }
            return new_iterator;
        }

        /**
         * TODO ++iter
         */
        const_iterator &operator++() {
            __node = __node->next;
            if (__node == __bucket->tail) {
                __bucket = __bucket->next;
                __node = __bucket->head->next;
            }
            return *this;
        }

        /**
         * TODO iter--
         */
        const_iterator operator--(int) {
            const_iterator new_iterator(*this);
            new_iterator.__node = new_iterator.__node->prev;
            if (new_iterator.__node == new_iterator.__bucket->head) {
                new_iterator.__bucket = new_iterator.__bucket->prev;
                new_iterator.__node = new_iterator.__bucket->tail->prev;
            }
            return new_iterator;
        }

        /**
         * TODO --iter
         */
        const_iterator &operator--() {
            __node = __node->next;
            if (__node == __bucket->head) {
                __bucket = __bucket->prev;
                __node = __bucket->tail->prev;
            }
            return *this;
        }

        /**
         * TODO *it
         */
        reference operator*() const {
            return *(__node->value);
        }

        /**
         * TODO it->field
         */
        pointer operator->() const noexcept {
            return __node->value;
        }

        /**
         * a operator to check whether two iterators are same (pointing to the same memory).
         */
        bool operator==(const iterator &rhs) const {
            return __node == rhs.__node;
        }

        bool operator==(const const_iterator &rhs) const {
            return __node == rhs.__node;
        }

        /**
         * some other operator for iterator.
         */
        bool operator!=(const iterator &rhs) const {
            return __node != rhs.__node;
        }

        bool operator!=(const const_iterator &rhs) const {
            return __node != rhs.__node;
        }
    };
};

}

#endif
