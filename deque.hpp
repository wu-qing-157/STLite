#ifndef SJTU_DEQUE_HPP
#define SJTU_DEQUE_HPP

#include "exceptions.hpp"

#include <cmath>
#include <cstddef>

namespace sjtu {

/**
 * @brief   A linear data structures supporting inserting and removing
 *
 * This container supports following operation in O(1) time: accessing, inserting before, or removing the first
 * element; accessing, inserting after, or removing the last element; find previous or following element.
 * This container also supports following operation in O(sqrt n) time: accessing with indices, inserting or
 * removing anywhere; find previous or following n element; calculating the distance between two elements.
 *
 * This implementation uses "Unrolled linked list as its internal structure.
 *
 * @tparam T    The type of elements stored in this container. MUST have copy constructor.
 */
template <typename T>
class deque {
  private:
    size_t __size;

  private:
    /**
     * @brief   Several parameters for splitting and merging buckets
     *
     * If a bucket contains more than @code{SPLIT_PARA()} elements after inserting, it will be split into
     * two buckets with approximately same size.
     *
     * If a bucket contains more than @code{NEW_PARA()} elements before pushing at either side, a new bucket
     * will be constructed to store the required element.
     *
     * If a bucket and its previous(or following) container contain less than @code{MERGE_PARA()} after removing,
     * they will be merged into a single bucket.
     */
    static constexpr double MIN_FOR_SPLIT = 9.9;
    static constexpr double CONSTANT_FOR_SPLIT = 2.89;
    static constexpr double CONSTANT_FOR_NEW = 1.98;
    static constexpr double CONSTANT_FOR_MERGE = 0.48;

    static constexpr double max(double a, double b) { return a > b ? a : b; }
    double SPLIT_PARA() {
        return max(MIN_FOR_SPLIT, CONSTANT_FOR_SPLIT * std::sqrt(__size));
    }
    double NEW_PARA() {
        return max(MIN_FOR_SPLIT, CONSTANT_FOR_NEW * std::sqrt(__size));
    }
    double MERGE_PARA() {
        return CONSTANT_FOR_MERGE * std::sqrt(__size);
    }

  private:
    /**
     * @brief   A unit in the container, which stores at most approximately sqrt(n) elements
     */
    struct bucket;

    /**
     * @brief   A unit in the bucket, which stores a single element
     */
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

        /**
         * @brief   Split before the specified position in this bucket
         */
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

            new_bucket->tail->prev = tail->prev;
            tail->prev->next = new_bucket->tail;
            new_bucket->head->next = new_first;
            new_first->prev = new_bucket->head;

            tail->prev = new_last;
            new_last->next = tail;

            new_bucket->size = size - pos;
            size = pos;

            // for (auto i = new_bucket->head->next; i != nullptr; i = i->next)
            // i->__bucket = new_bucket;
        }

        /**
         * @brief   Merge the next bucket
         */
        void __merge_next() {
            node *__old_tail = tail, *__old_head = next->head;
            __old_tail->prev->next = __old_head->next;
            __old_head->next->prev = __old_tail->prev;
            delete __old_head;
            delete __old_tail;
            tail = next->tail;

            size += next->size;

            bucket *__old_bucket = next;
            __old_bucket->next->prev = this;
            next = __old_bucket->next;
            delete __old_bucket;
        }

        /**
         * @brief   Copy this bucket
         */
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

        /**
         * @brief   Clear all elements in this bucket
         */
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

  public:
    /**
     * @brief   Default constructor, which constructs a @code{deque} with no elements
     */
    deque() : head(new bucket), tail(new bucket), __size(0) {
        head->next = tail;
        tail->prev = head;
    }

    /**
     * @brief   Copy constructor
     */
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

    /**
     * @brief   Destructor
     */
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
     * @brief   Assignment operator
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
     * @brief   Access the element at specified index
     *
     * @throw   index_out_of_bound  if the index is out of bound
     */
    T &at(const size_t &pos) {
        // if (pos < 0 || pos >= __size) throw index_out_of_bound();
        return operator[](pos);
    }
    const T &at(const size_t &pos) const {
        // if (pos < 0 || pos >= __size) throw index_out_of_bound();
        return operator[](pos);
    }

    /**
     * The same as @code{at}
     */
    T &operator[](const size_t &pos) {
        if (pos < 0 || pos >= __size) throw index_out_of_bound();
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
        if (pos < 0 || pos >= __size) throw index_out_of_bound();
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
     * @brief   Access the first element
     *
     * @throw   container_is_empty  if the container is empty.
     */
    const T &front() const {
        if (__size == 0) throw container_is_empty();
        return *(head->next->head->next->value);
    }

    /**
     * @brief   Access the last element
     *
     * @throw   container_is_empty  if the container is empty.
     */
    const T &back() const {
        if (__size == 0) throw container_is_empty();
        return *(tail->prev->tail->prev->value);
    }

    /**
     * @return  A @code{iterator} pointing to the first element
     */
    iterator begin() {
        return iterator(this, head->next, head->next->head->next);
    }

    /**
     * @return  A @code{const_iterator} pointing to the first element
     */
    const_iterator cbegin() const {
        return const_iterator(this, head->next, head->next->head->next);
    }

    /**
     * @return  A @code{iterator} pointing to the next of the last element
     */
    iterator end() {
        return iterator(this, tail, tail->head->next);
    }

    /**
     * @return  A @code{const_iterator} pointing to the next of the last element
     */
    const_iterator cend() const {
        return const_iterator(this, tail, tail->head->next);
    }

    /**
     * @brief   Check whether the container is empty
     */
    bool empty() const {
        return __size == 0;
    }

    /**
     * @return  The number of elements
     */
    size_t size() const {
        return __size;
    }

    /**
     * @brief   Clear all elements
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
     * @brief   Insert an elements at the specified location(before @code{pos}) in the container.
     *
     * @return  An iterator pointing to the inserted value
     * @throw   invalid_iterator    if the iterator points to another container
     */
    iterator insert(iterator pos, const T &value) {
        if (pos.__deque != this) throw invalid_iterator();
        if (pos == end()) {
            push_back(value);
            return --end();
        }
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
        if (tar_bucket->size > SPLIT_PARA())
            tar_bucket->__split_before(tar_bucket->size >> 1);
        for (auto i = tar_bucket->head->next; i != tar_bucket->tail; i = i->next)
            if (i == new_node)
                return iterator(this, tar_bucket, new_node);
        return iterator(this, tar_bucket->next, new_node);
    }

    /**
     * @brief   Remove the specified element
     *
     * @return  An iterator pointing to the next of the specified element
     * @throw   invalid_iterator    if the iterator points to another container or it does not point to any element
     */
    iterator erase(iterator pos) {
        if (pos.__deque != this) throw invalid_iterator();
        *pos; // test whether pos is invalid
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
        bool next_in_next_bucket = tar_node->next == tar_bucket->tail;
        node *next_node = next_in_next_bucket ? tar_bucket->next->head->next : tar_node->next;
        tar_node->next->prev = tar_node->prev;
        tar_node->prev->next = tar_node->next;
        delete tar_node;
        if (tar_bucket->prev != head && tar_bucket->size + tar_bucket->prev->size < MERGE_PARA()) {
            bucket *result_bucket = tar_bucket->prev;
            result_bucket->__merge_next();
            if (next_in_next_bucket)
                return iterator(this, result_bucket->next, next_node);
            else
                return iterator(this, result_bucket, next_node);
        } else if (tar_bucket->next != tail && tar_bucket->size + tar_bucket->next->size < MERGE_PARA()) {
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
     * @brief   Add an element to the end
     */
    void push_back(const T &value) {
        __size++;
        if (__size == 1 || tail->prev->size > NEW_PARA()) {
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
     * @brief   Remove the last element
     *
     * @throw   container_is_empty  if the container is empty
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
     * @brief   Add an element to the beginning
     */
    void push_front(const T &value) {
        __size++;
        if (__size == 1 || head->next->size > NEW_PARA()) {
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
     * @brief   Remove the first element
     *
     * @throw   container_is_empty  if the container is empty
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
         * @return  A new iterator pointing to the n-next element
         * @throw   invalid_iterator    if the iterator exceeds @code{end()} after moving
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
            while (cur_diff + new_iterator.__bucket->size <= n && new_iterator.__bucket->size != 0) {
                cur_diff += new_iterator.__bucket->size;
                new_iterator.__bucket = new_iterator.__bucket->next;
            }
            if (new_iterator.__bucket->size == 0 && cur_diff != n) throw invalid_iterator();
            new_iterator.__node = new_iterator.__bucket->head->next;
            while (cur_diff < n) {
                cur_diff++;
                new_iterator.__node = new_iterator.__node->next;
            }
            return new_iterator;
        }

        /**
         * @return  A new iterator pointing to the n-previous element
         * @throw   invalid_iterator    if the iterator exceeds @code{begin()} after moving
         */
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
            while (cur_diff + new_iterator.__bucket->size <= n && new_iterator.__bucket->size != 0) {
                cur_diff += new_iterator.__bucket->size;
                new_iterator.__bucket = new_iterator.__bucket->prev;
            }
            if (new_iterator.__bucket->size == 0 && cur_diff != n) throw invalid_iterator();
            new_iterator.__node = new_iterator.__bucket->tail->prev;
            while (cur_diff < n) {
                cur_diff++;
                new_iterator.__node = new_iterator.__node->prev;
            }
            return new_iterator;
        }

      private:
        /**
         * @return  The index of this element in this deque
         */
        size_t pos() const {
            size_t size = 0;
            for (auto cur_node = __node; cur_node->prev != __bucket->head; cur_node = cur_node->prev)
                size++;
            for (auto cur_bucket = __bucket; cur_bucket->prev != __deque->head; cur_bucket = cur_bucket->prev)
                size += cur_bucket->prev->size;
            return size;
        }

      public:
        /**
         * @return  The distance between two elements
         * @throw   invalid_iterator    if they point to different container
         */
        difference_type operator-(const iterator &rhs) const {
            if (__deque != rhs.__deque) throw invalid_iterator();
            return pos() - rhs.pos();
        }

        /**
         * @brief   Move to the n-next element
         *
         * @throw   invalid_iterator    if the iterator exceeds @code{end()} after moving
         */
        iterator &operator+=(const difference_type &n) {
            iterator result = operator+(n);
            __bucket = result.__bucket;
            __node = result.__node;
            return *this;
        }

        /**
         * @brief   Move to the n-previous element
         *
         * @throw   invalid_iterator    if the iterator exceeds @code{begin()} after moving
         */
        iterator &operator-=(const int &n) {
            iterator result = operator-(n);
            __bucket = result.__bucket;
            __node = result.__node;
            return *this;
        }

        /**
         * @brief   Move to the next element
         *
         * @throw   invalid_iterator    if the iterator exceeds @code{end()} after moving
         */
        const iterator operator++(int) {
            if (*this == __deque->end()) throw invalid_iterator();
            auto backup = iterator(*this);
            __node = __node->next;
            if (__node == __bucket->tail) {
                __bucket = __bucket->next;
                __node = __bucket->head->next;
            }
            return backup;
        }

        /**
         * @brief   Move to the next element
         *
         * @throw   invalid_iterator    if the iterator exceeds @code{end()} after moving
         */
        iterator &operator++() {
            if (*this == __deque->end()) throw invalid_iterator();
            __node = __node->next;
            if (__node == __bucket->tail) {
                __bucket = __bucket->next;
                __node = __bucket->head->next;
            }
            return *this;
        }

        /**
         * @brief   Move to the previous element
         *
         * @throw   invalid_iterator    if the iterator exceeds @code{begin()} after moving
         */
        const iterator operator--(int) {
            if (*this == __deque->begin()) throw invalid_iterator();
            auto backup = iterator(*this);
            __node = __node->prev;
            if (__node == __bucket->head) {
                __bucket = __bucket->prev;
                __node = __bucket->tail->prev;
            }
            return backup;
        }

        /**
         * @brief   Move to the previous element
         *
         * @throw   invalid_iterator    if the iterator exceeds @code{begin()} after moving
         */
        iterator &operator--() {
            if (*this == __deque->begin()) throw invalid_iterator();
            __node = __node->prev;
            if (__node == __bucket->head) {
                __bucket = __bucket->prev;
                __node = __bucket->tail->prev;
            }
            return *this;
        }

        reference operator*() const {
            if (__node == nullptr || __node->value == nullptr) throw invalid_iterator();
            return *(__node->value);
        }

        pointer operator->() const noexcept {
            if (__node == nullptr || __node->value == nullptr) throw invalid_iterator();
            return __node->value;
        }

        bool operator==(const iterator &rhs) const {
            return __node == rhs.__node;
        }

        bool operator==(const const_iterator &rhs) const {
            return __node == rhs.__node;
        }

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
        const deque *__deque;
        const bucket *__bucket;
        const node *__node;

      public:
        explicit const_iterator(const deque *__deque = nullptr, const bucket *__bucket = nullptr,
                                const node *__node = nullptr) :
                __deque(__deque), __bucket(__bucket), __node(__node) {}

        const_iterator(const const_iterator &other) = default;

        /**
         * @return  A new iterator pointing to the n-next element
         * @throw   invalid_iterator    if the iterator exceeds @code{end()} after moving
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
            while (cur_diff + new_iterator.__bucket->size <= n && new_iterator.__bucket->size != 0) {
                cur_diff += new_iterator.__bucket->size;
                new_iterator.__bucket = new_iterator.__bucket->next;
            }
            if (new_iterator.__bucket->size == 0 && cur_diff != n) throw invalid_iterator();
            new_iterator.__node = new_iterator.__bucket->head->next;
            while (cur_diff < n) {
                cur_diff++;
                new_iterator.__node = new_iterator.__node->next;
            }
            return new_iterator;
        }

        /**
         * @return  A new iterator pointing to the n-previous element
         * @throw   invalid_iterator    if the iterator exceeds @code{begin()} after moving
         */
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
            while (cur_diff + new_iterator.__bucket->size <= n && new_iterator.__bucket->size != 0) {
                cur_diff += new_iterator.__bucket->size;
                new_iterator.__bucket = new_iterator.__bucket->prev;
            }
            if (new_iterator.__bucket->size == 0 && cur_diff != n) throw invalid_iterator();
            new_iterator.__node = new_iterator.__bucket->tail->prev;
            while (cur_diff < n) {
                cur_diff++;
                new_iterator.__node = new_iterator.__node->prev;
            }
            return new_iterator;
        }

      private:
        /**
         * @return  The index of this element in this deque
         */
        size_t pos() const {
            size_t size = 0;
            for (auto cur_node = __node; cur_node->prev != __bucket->head; cur_node = cur_node->prev)
                size++;
            for (auto cur_bucket = __bucket; cur_bucket->prev != __deque->head; cur_bucket = cur_bucket->prev)
                size += cur_bucket->prev->size;
            return size;
        }

      public:
        /**
         * @return  The distance between two elements
         * @throw   invalid_iterator    if they point to different container
         */
        difference_type operator-(const const_iterator &rhs) const {
            if (__deque != rhs.__deque) throw invalid_iterator();
            return pos() - rhs.pos();
        }

        /**
         * @brief   Move to the n-next element
         *
         * @throw   invalid_iterator    if the iterator exceeds @code{end()} after moving
         */
        const_iterator &operator+=(const difference_type &n) {
            const_iterator result = operator+(n);
            __bucket = result.__bucket;
            __node = result.__node;
            return *this;
        }

        /**
         * @brief   Move to the n-previous element
         *
         * @throw   invalid_iterator    if the iterator exceeds @code{begin()} after moving
         */
        const_iterator &operator-=(const int &n) {
            const_iterator result = operator-(n);
            __bucket = result.__bucket;
            __node = result.__node;
            return *this;
        }

        /**
         * @brief   Move to the next element
         *
         * @throw   invalid_iterator    if the iterator exceeds @code{end()} after moving
         */
        const const_iterator operator++(int) {
            if (*this == __deque->cend()) throw invalid_iterator();
            auto backup = const_iterator(*this);
            __node = __node->next;
            if (__node == __bucket->tail) {
                __bucket = __bucket->next;
                __node = __bucket->head->next;
            }
            return backup;
        }

        /**
         * @brief   Move to the next element
         *
         * @throw   invalid_iterator    if the iterator exceeds @code{end()} after moving
         */
        const_iterator &operator++() {
            if (*this == __deque->cend()) throw invalid_iterator();
            __node = __node->next;
            if (__node == __bucket->tail) {
                __bucket = __bucket->next;
                __node = __bucket->head->next;
            }
            return *this;
        }

        /**
         * @brief   Move to the previous element
         *
         * @throw   invalid_iterator    if the iterator exceeds @code{begin()} after moving
         */
        const const_iterator operator--(int) {
            if (*this == __deque->cbegin()) throw invalid_iterator();
            auto backup = const_iterator(*this);
            __node = __node->prev;
            if (__node == __bucket->head) {
                __bucket = __bucket->prev;
                __node = __bucket->tail->prev;
            }
            return backup;
        }

        /**
         * @brief   Move to the previous element
         *
         * @throw   invalid_iterator    if the iterator exceeds @code{begin()} after moving
         */
        const_iterator &operator--() {
            if (*this == __deque->cbegin()) throw invalid_iterator();
            __node = __node->prev;
            if (__node == __bucket->head) {
                __bucket = __bucket->prev;
                __node = __bucket->tail->prev;
            }
            return *this;
        }

        reference operator*() const {
            if (__node == nullptr || __node->value == nullptr) throw invalid_iterator();
            return *(__node->value);
        }

        pointer operator->() const noexcept {
            if (__node == nullptr || __node->value == nullptr) throw invalid_iterator();
            return __node->value;
        }

        bool operator==(const iterator &rhs) const {
            return __node == rhs.__node;
        }

        bool operator==(const const_iterator &rhs) const {
            return __node == rhs.__node;
        }

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
