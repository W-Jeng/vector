#pragma once

#include "custom_allocator.h"
#include <memory>
#include <algorithm>

namespace ctm 
{

template <typename T, typename Alloc = ctm::allocator<T>>
class vector 
{
public:
    using value_type = T;
    using size_type = std::size_t;
    using allocator_type = Alloc;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = T&;
    using iterator = T*;
    using const_iterator = const T*;

    vector():
        data_(nullptr),
        size_(0),
        capacity_(0) {}

    ~vector()
    {
        for (size_t i = 0; i < size_; ++i)
        {
            allocator_.destroy(data_ + i);
        }
        if (data_)
        {
            allocator_.deallocate(data_, capacity_);
        }
        return;
    }
    
    // ELEMENT ACCESS
    reference at(std::size_t index)
    {
        if (index < 0 || index >= size_)
        {
            throw std::out_of_range("Indexing out of range");
        }
        return data_[index];
    }
    
    const_reference at(std::size_t index) const
    {
        if (index < 0 || index >= size_)
        {
            throw std::out_of_range("Indexing out of range");
        }
        return data_[index];
    }

    reference operator[](std::size_t index)
    {
        return data_[index];
    }

    // accessed when using const vector
    const_reference operator[](std::size_t index) const
    {
        return data_[index];
    }

    reference front() 
    {
        return data_[0];
    }

    const_reference front() const 
    {
        return data_[0];
    }

    reference back()
    {
        return data_[size_ - 1];
    }

    const_reference back() const
    {
        return data_[size_ - 1];
    }

    T* data() 
    {
        return data_;
    }

    const T* data() const
    {
        return data_;
    }

    // Iterators
    iterator begin()
    {
        return data_;
    }

    const_iterator begin() const
    {
        return data_;
    }

    const_iterator cbegin() const
    {
        return data_;
    }

    iterator end()
    {
        return data_ + size_;
    }

    const_iterator end() const
    {
        return data_ + size_;
    }

    const_iterator cend() const
    {
        return data_ + size_;
    }

    // CAPACITY
    bool empty() const
    {
        return (size_ == 0);
    }

    size_type size() const
    {
        return size_;
    }

    size_type max_size() const
    {
        return std::numeric_limits<size_type>::max()/sizeof(T);
    }

    void reserve(std::size_t new_capacity)
    {
        if (new_capacity <= capacity_)
        {
            return;
        }

        T* new_data = allocator_.allocate(new_capacity);

        for (std::size_t i = 0; i < size_; ++i)
        {
            allocator_.construct(new_data + i, std::move(data_[i]));
            allocator_.destroy(data_ + i);
        }

        if (data_)
        {
            allocator_.deallocate(data_, capacity_);
        }

        data_ = new_data;
        capacity_ = new_capacity;
        return;
    }

    size_type capacity() const
    {
        return capacity_;
    }

    // MODIFIERS
    void clear() 
    {
        for (std::size_t i = 0; i < size_; ++i) 
        {
            allocator_.destroy(data_ + i);
        }

        size_ = 0;
        // capacity is not released as per the standard's implementation
    }

    iterator insert(const_iterator pos, const T& value)
    {
        return insert(pos, 1, value);
    }

    iterator insert(const_iterator pos, T&& value)
    {
        if (pos == end())
        {
            push_back(std::move(value));
            return end()-1;
        }

        const difference_type index = pos - begin();

        if (!(0 <= index && index < size()))
        {
            throw new std::out_of_range("Inserting index is out of range (more than size.");
        }

        if (size() + 1 > capacity())
        {
            reserve(2* capacity());
        }

        iterator it_move_begin = &data_[index];
        std::move(it_move_begin, end(), it_move_begin + 1);
        *it_move_begin = std::move(value);
        ++size_;
        return it_move_begin;
    }


    iterator insert(const_iterator pos, size_type count, const T& value)
    {
        if (pos == end())
        {
            for (size_type i = 0; i < count; ++i)
            {
                push_back(value);
            }
            return end()-count;
        }

        const difference_type index = pos - begin();

        if (!(0 <= index && index < size()))
        {
            throw new std::out_of_range("Inserting index is out of range (more than size.");
        }

        if (size() + count > capacity())
        {
            reserve(next_capacity_power_of_two(size() + count));
        }

        iterator it_move_begin = &data_[index];
        std::move(it_move_begin, end(), it_move_begin + count);

        for (size_type i = index; i < index + count; ++i)
        {
            data_[i] = value;
        }

        size_ += count;
        return it_move_begin;
    }

    template <typename InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last)
    {
        const difference_type count = static_cast<difference_type>(last-first);

        if (pos == end())
        {
            while (first != last)
            {
                push_back(*first);
                ++first;
            }

            return end()-count;
        }

        difference_type index = pos - begin();
        
        if (!(0 <= index && index < size()))
        {
            throw new std::out_of_range("Inserting index is out of range (more than size.");
        }

        if (size() + count > capacity())
        {
            reserve(next_capacity_power_of_two(size() + count));
        }

        iterator it_move_begin = &data_[index];
        std::move(it_move_begin, end(), it_move_begin + count);

        while (first != last)
        {
            data_[index] = *first;
            ++index;
            ++first;
        }           

        size_ += count;
        return it_move_begin;
    }

    iterator insert(const_iterator pos, std::initializer_list<T> ilist)
    {
        return insert(pos, ilist.begin(), ilist.end());
    }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args)
    {
        if (pos == end())
        {
            push_back(T(std::forward<Args>(std::move(args))...));
            return end()-1;
        }

        const difference_type index = pos - begin();

        if (!(0 <= index && index < size()))
        {
            throw new std::out_of_range("Emplace position is out of range (more than size.");
        }

        if (size() + 1 > capacity())
        {
            reserve(2 * capacity());
        }

        iterator it_move_begin = &data_[index];
        std::move(it_move_begin, end(), it_move_begin + 1);


        *it_move_begin = T(std::forward<Args>(std::move(args))...);
        ++size_;

        return it_move_begin;
    }

    void push_back(const T& value)
    {
        if (size_ == capacity_)
        {
            reserve(capacity_ == 0 ? 1 : capacity_ * 2);
        }

        allocator_.construct(data_ + size_, value);
        ++size_;
        return;
    }

private:
    T* data_;
    std::size_t size_;  
    std::size_t capacity_;
    Alloc allocator_;

    size_type next_capacity_power_of_two(size_type new_capacity)
    {
        const size_type current_capacity = capacity();

        if (new_capacity  <= current_capacity)
        {
            return current_capacity;
        }

        size_type to_change_capacity = 2 * current_capacity;

        while (to_change_capacity < new_capacity)
        {
            to_change_capacity *= 2;
        }

        return to_change_capacity;
    }

};


};