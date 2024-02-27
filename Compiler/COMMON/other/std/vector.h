#pragma once

extern "C" {
#include <stddef.h>
#include <string.h>
}

#include "srl_memory.hpp"
#include "utils.h"

namespace std
{
    /**
     * @brief A simple vector implementation.
     * @tparam T The type of elements stored in the vector.
     */
    template <typename T>
    class vector
    {
        
    private:
        size_t capacity_; /**< The capacity of the vector. */
        size_t size_;     /**< The size of the vector. */
        T* data_;         /**< Pointer to the data array. */

    public:
        /**
         * @brief Iterator class for vector.
         */
        class Iterator
        {
            friend class vector;

        public:
            /**
             * @brief Constructor for Iterator.
             * @param ptr Pointer to the element.
             */
            Iterator(T* ptr) : ptr(ptr) {}

            /**
             * @brief Overloaded dereference operator.
             * @return Reference to the pointed element.
             */
            T& operator*() const
            {
                return *ptr;
            }

            /**
             * @brief Overloaded pre-increment operator.
             * @return Reference to the incremented iterator.
             */
            Iterator& operator++()
            {
                ++ptr;
                return *this;
            }

            /**
             * @brief Overloaded inequality operator.
             * @param other Another iterator for comparison.
             * @return True if iterators are not equal, false otherwise.
             */
            bool operator!=(const Iterator& other) const
            {
                return ptr != other.ptr;
            }

            /**
             * @brief Overloaded equality operator.
             * @param other Another iterator for comparison.
             * @return True if iterators are equal, false otherwise.
             */
            bool operator==(const Iterator& other) const
            {
                return ptr == other.ptr;
            }

            /**
             * @brief Overloaded subtraction operator.
             * @param n Number of positions to move backward.
             * @return Iterator moved back by n positions.
             */
            Iterator operator-(size_t n) const
            {
                return Iterator(ptr - n);
            }

        private:
            T* ptr; /**< Pointer to the element. */
        };

        /**
         * @brief Default constructor for the vector.
         */
        vector() : capacity_(0), size_(0), data_(nullptr) {}

        /**
         * @brief Destructor for the vector.
         */
        ~vector()
        {
            clear();
        }

        /**
         * @brief Copy constructor for the vector.
         * @param other Another vector to copy.
         */
        vector(const vector& other) : capacity_(other.capacity_), size_(other.size_)
        {
            data_ = new T[capacity_];
            for (size_t i = 0; i < size_; ++i)
            {
                data_[i] = other.data_[i];
            }
        }

        /**
         * @brief Move constructor for the vector.
         * @param other Another vector to move.
         */
        vector(vector&& other) noexcept : capacity_(0), size_(0), data_(nullptr)
        {
            swap(*this, other);
        }

        /**
         * @brief Copy assignment operator for the vector.
         * @param other Another vector to copy.
         * @return Reference to the modified vector.
         */
        vector& operator=(vector other) noexcept
        {
            swap(*this, other);
            return *this;
        }

        /**
         * @brief Adds an element to the end of the vector.
         * @param value The value to be added.
         * @return True if successful, false otherwise.
         */
        bool push_back(T&& value)
        {
            if (size_ == capacity_)
            {
                size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
                if (!reserve(new_capacity))
                {
                    return false;
                }
            }
            data_[size_] = std::move(value);
            ++size_;
            return true;
        }

        /**
         * @brief Adds a copy of an element to the end of the vector.
         * @param value The value to be added.
         * @return True if successful, false otherwise.
         */
        bool push_back(const T& value)
        {
            if (size_ == capacity_)
            {
                size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
                if (!reserve(new_capacity))
                {
                    return false;
                }
            }
            data_[size_] = value;
            ++size_;
            return true;
        }

        /**
         * @brief Removes the last element from the vector.
         */
        void pop_back()
        {
            if (size_ > 0)
            {
                --size_;
            }
        }

        /**
         * @brief Overloaded subscript operator.
         * @param index The index of the element to access.
         * @return Reference to the element at the specified index.
         */
        T& operator[](size_t index)
        {
            return data_[index];
        }

        /**
         * @brief Reserves space for at least a specified number of elements.
         * @param new_capacity The new capacity to reserve.
         * @return True if successful, false otherwise.
         */
        bool reserve(size_t new_capacity)
        {
            if (new_capacity > capacity_)
            {
                T* new_data = new T[new_capacity];
                if (!new_data)
                {
                    return false;
                }

                for (size_t i = 0; i < size_; ++i)
                {
                    new_data[i] = std::move(data_[i]);
                }

                delete[] data_;
                data_ = new_data;
                capacity_ = new_capacity;
            }
            return true;
        }

        /**
         * @brief Reduces the capacity to fit the current size.
         * @return True if successful, false otherwise.
         */
        bool shrink_to_fit()
        {
            if (size_ < capacity_)
            {
                T* new_data = new T[size_];
                if (!new_data)
                {
                    return false;
                }

                for (size_t i = 0; i < size_; ++i)
                {
                    new_data[i] = std::move(data_[i]);  // Use move semantics here
                }

                delete[] data_;
                data_ = new_data;
                capacity_ = size_;
            }
            return true;
        }

        /**
         * @brief Resizes the vector to contain a specified number of elements.
         * @param new_size The new size of the vector.
         */
        void resize(size_t new_size)
        {
            if (new_size > capacity_)
            {
                reserve(new_size);
            }
            size_ = new_size;
        }

        /**
         * @brief Clears the vector, removing all elements.
         */
        void clear()
        {
            delete[] data_;
            data_ = nullptr;
            capacity_ = 0;
            size_ = 0;
        }

        /**
         * @brief Removes an element at a specified index.
         * @param index The index of the element to remove.
         */
        void erase(size_t index)
        {
            erase(begin() + index);
        }

        /**
         * @brief Removes the element at a specified position.
         * @param position Iterator pointing to the element to remove.
         */
        void erase(Iterator position)
        {
            if (position.ptr < data_ || position.ptr >= data_ + size_)
            {
                return;
            }

            Iterator last = end() - 1;

            if (position != last)
            {
                *position = std::move(*last);
            }

            --size_;
        }

        /**
         * @brief Returns the size of the vector.
         * @return The size of the vector.
         */
        size_t size() const
        {
            return size_;
        }

        /**
         * @brief Checks if the vector is empty.
         * @return True if empty, false otherwise.
         */
        bool empty() const
        {
            return size_ == 0;
        }

        /**
         * @brief Returns the current capacity of the vector.
         * @return The capacity of the vector.
         */
        size_t capacity() const
        {
            return capacity_;
        }

        /**
         * @brief Returns an iterator pointing to the first element of the vector.
         * @return Iterator pointing to the first element.
         */
        Iterator begin()
        {
            return Iterator(data_);
        }

        /**
         * @brief Returns an iterator pointing to the position one past the last element of the vector.
         * @return Iterator pointing to one past the last element.
         */
        Iterator end()
        {
            return Iterator(data_ + size_);
        }

        /**
         * @brief Returns a reference to the last element of the vector.
         * @return Reference to the last element.
         */
        T& back()
        {
            return data_[size_ - 1];
        }

        /**
         * @brief Swaps the content of two vectors.
         * @param first The first vector to swap.
         * @param second The second vector to swap.
         */
        friend void swap(vector& first, vector& second) noexcept
        {
            using std::swap;
            swap(first.capacity_, second.capacity_);
            swap(first.size_, second.size_);
            swap(first.data_, second.data_);
        }

    };
}
