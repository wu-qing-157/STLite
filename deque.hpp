#ifndef SJTU_DEQUE_HPP
#define SJTU_DEQUE_HPP

#include "exceptions.hpp"

#include <cmath>
#include <cstddef>

namespace sjtu {

template <typename T>
class deque {
  private:
    static const size_t MIN_FOR_SPLIT{10};
    static constexpr double CONSTANT_FOR_SPLIT = 2.89;
    static constexpr double CONSTANT_FOR_NEW = 1.98;
    static constexpr double CONSTANT_FOR_MERGE = 0.31;

  private:
    struct bucket;

    struct node {
        node *prev, *next;
        T *value;

        node() : prev(nullptr), next(nullptr), value(nullptr) {}

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
            new_bucket->size = other->size;
            for (auto old_node = other->head->next; old_node != other->tail; old_node = old_node->next) {
                auto new_node = new node;
                new_node->value = new T(*(old_node->value));
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

        for (auto old_bucket = other.head->next; old_bucket != other.tail; old_bucket = old_bucket->next) {
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
        delete head->head;
        delete head->tail;
        delete head;
        delete tail->head;
        delete tail->tail;
        delete tail;
    }

    /**
     * TODO assignment operator
     */
    deque &operator=(const deque &other) {
        if (this == &other) return *this;
        clear();
        __size = other.__size;
        for (auto old_bucket = other.head->next; old_bucket != other.tail; old_bucket = old_bucket->next) {
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
        return *(cur_node->value);
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
        return *(cur_node->value);
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
    iterator begin() {
        return iterator(this, head->next, head->next->head->next);
    }

    const_iterator cbegin() const {
        return const_iterator(this, head->next, head->next->head->next);
    }

    /**
     * returns an iterator to the end.
     */
    iterator end() {
        return iterator(this, tail, tail->head->next);
    }

    const_iterator cend() const {
        return const_iterator(this, tail, tail->head->next);
    }

    /**
     * checks whether the container is empty.
     */
    bool empty() const {
        return __size == 0;
    }

    /**
     * returns the number of elements
     */
    size_t size() const {
        return __size;
    }

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
            delete cur;
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
    iterator insert(iterator pos, const T &value) {
        __size++;
        bucket *tar_bucket = pos.__bucket;
        tar_bucket->size++;
        node *pos_node = pos.__node;
        auto new_node = new node;
        new_node->value = new T(value);
        new_node->prev = pos_node->prev;
        pos_node->prev->next = new_node;
        new_node->next = pos_node;
        pos_node->prev = new_node;
        if (tar_bucket->size > MIN_FOR_SPLIT && tar_bucket->size > CONSTANT_FOR_SPLIT * std::sqrt(__size))
            tar_bucket->__split_before(tar_bucket->size >> 1);
        for (auto i = tar_bucket->head; i != tar_bucket->tail; i = i->next)
            if (i == new_node)
                return iterator(this, tar_bucket, new_node);
        return iterator(this, tar_bucket->next, new_node);
    }

    /**
     * removes specified element at pos.
     * removes the element at pos.
     * returns an iterator pointing to the following element, if pos pointing to the last element, end() will be returned.
     * throw if the container is empty, the iterator is invalid or it points to a wrong place.
     */
    iterator erase(iterator pos) {
        __size--;
        bucket *tar_bucket = pos.__bucket;
        tar_bucket->size--;
        node *tar_node = pos.__node;
        if (tar_bucket->size == 0) {
            iterator next_iterator(this, tar_bucket->next, tar_bucket->next->head->next);
            tar_bucket->clear();
            delete tar_bucket->head;
            delete tar_bucket->tail;
            tar_bucket->next->prev = tar_bucket->prev;
            tar_bucket->prev->next = tar_bucket->next;
            delete tar_bucket;
            return next_iterator;
        }
        bool next_in_next_bucket = tar_node == tar_bucket->tail->prev;
        node *next_node = next_in_next_bucket ? tar_bucket->next->head->next : tar_node->next;
        tar_node->next->prev = tar_node->prev;
        tar_node->prev->next = tar_node->next;
        delete tar_node;
        if (tar_bucket->prev != head &&
            tar_bucket->size + tar_bucket->prev->size < CONSTANT_FOR_MERGE * std::sqrt(__size)) {
            bucket *result_bucket = tar_bucket->prev;
            result_bucket->__merge_next();
            if (next_in_next_bucket)
                return iterator(this, tar_bucket->next, next_node);
            else
                return iterator(this, tar_bucket, next_node);
        } else if (tar_bucket->next != tail &&
                   tar_bucket->size + tar_bucket->next->size < CONSTANT_FOR_MERGE * std::sqrt(__size)) {
            tar_bucket->__merge_next();
            return iterator(this, tar_bucket, next_node);
        } else {
            if (next_in_next_bucket)
                return iterator(this, tar_bucket->next, next_node);
            else
                return iterator(this, tar_bucket, next_node);
        }
    }

    /**
     * adds an element to the end
     */
    void push_back(const T &value) {
        __size++;
        if (__size == 1 || (tail->prev->size > MIN_FOR_SPLIT &&
                            tail->prev->size > CONSTANT_FOR_NEW * std::sqrt(__size))) {
            auto new_bucket = new bucket;
            auto new_node = new node;
            new_node->value = new T(value);
            new_bucket->head->next = new_bucket->tail->prev = new_node;
            new_node->prev = new_bucket->head;
            new_node->next = new_bucket->tail;
            tail->prev->next = new_bucket;
            new_bucket->prev = tail->prev;
            new_bucket->next = tail;
            tail->prev = new_bucket;
            new_bucket->size = 1;
        } else {
            tail->prev->size++;
            auto new_node = new node;
            new_node->value = new T(value);
            tail->prev->tail->prev->next = new_node;
            new_node->prev = tail->prev->tail->prev;
            new_node->next = tail->prev->tail;
            tail->prev->tail->prev = new_node;
        }
    }

    /**
     * removes the last element
     *     throw when the container is empty.
     */
    void pop_back() {
        if (__size == 0) throw container_is_empty();
        __size--;
        tail->prev->size--;
        if (tail->prev->size == 0) {
            auto tar_bucket = tail->prev;
            delete tar_bucket->head->next;
            delete tar_bucket->head;
            delete tar_bucket->tail;
            tar_bucket->prev->next = tar_bucket->next;
            tar_bucket->next->prev = tar_bucket->prev;
            delete tar_bucket;
        } else {
            auto tar_node = tail->prev->tail->prev;
            tar_node->prev->next = tar_node->next;
            tar_node->next->prev = tar_node->prev;
            delete tar_node;
        }
    }

    /**
     * inserts an element to the beginning.
     */
    void push_front(const T &value) {
        __size++;
        if (__size == 1 || (head->next->size > MIN_FOR_SPLIT &&
                            head->next->size > CONSTANT_FOR_NEW * std::sqrt(__size))) {
            auto new_bucket = new bucket;
            auto new_node = new node;
            new_node->value = new T(value);
            new_bucket->head->next = new_bucket->tail->prev = new_node;
            new_node->prev = new_bucket->head;
            new_node->next = new_bucket->tail;
            head->next->prev = new_bucket;
            new_bucket->next = head->next;
            new_bucket->prev = head;
            head->next = new_bucket;
            new_bucket->size = 1;
        } else {
            head->next->size++;
            auto new_node = new node;
            new_node->value = new T(value);
            head->next->head->next->prev = new_node;
            new_node->next = head->next->head->next;
            new_node->prev = head->next->head;
            head->next->head->next = new_node;
        }
    }

    /**
     * removes the first element.
     *     throw when the container is empty.
     */
    void pop_front() {
        if (__size == 0) throw container_is_empty();
        __size--;
        head->next->size--;
        if (head->next->size == 0) {
            auto tar_bucket = head->next;
            delete tar_bucket->head->next;
            delete tar_bucket->head;
            delete tar_bucket->tail;
            tar_bucket->prev->next = tar_bucket->next;
            tar_bucket->next->prev = tar_bucket->prev;
            delete tar_bucket;
        } else {
            auto tar_node = head->next->head->next;
            tar_node->prev->next = tar_node->next;
            tar_node->next->prev = tar_node->prev;
            delete tar_node;
        }
    }

  public:
    class iterator {
        friend class const_iterator;

        friend iterator deque::insert(iterator, const T &);

        friend iterator deque::erase(iterator);

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
            new_iterator.__node = new_iterator.__bucket->head->next;
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
            new_iterator.__node = new_iterator.__bucket->tail->prev;
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
        const iterator operator++(int) {
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
        const iterator operator--(int) {
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
            new_iterator.__node = new_iterator.__bucket->head->next;
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
            new_iterator.__node = new_iterator.__bucket->tail->prev;
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
        const const_iterator operator++(int) {
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
        const const_iterator operator--(int) {
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
