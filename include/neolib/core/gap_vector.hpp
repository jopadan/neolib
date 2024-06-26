// gap_vector.hpp
/*
 *  Copyright (c) 2023 Leigh Johnston.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *     * Neither the name of Leigh Johnston nor the names of any
 *       other contributors to this software may be used to endorse or
 *       promote products derived from this software without specific prior
 *       written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <neolib/neolib.hpp>
#include <algorithm>
#include <memory>

namespace neolib
{
    namespace detail
    {
        template <typename T, typename U, bool = std::is_const_v<U>> struct const_if_const {};
        template <typename T, typename U> struct const_if_const<T, U, true> { using type = T const; };
        template <typename T, typename U> struct const_if_const<T, U, false> { using type = T; };
        template <typename T, typename U>
        using const_if_const_t = typename const_if_const<T, U>::type;
    }

    template <typename T, std::size_t DefaultGapSize_ = 256, std::size_t NearnessFactor_ = 2, typename Allocator = std::allocator<T>>
    class gap_vector
    {
    public:
        using value_type = T;
        using allocator_type = Allocator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type&;
        using const_reference = value_type const&;
        using pointer = std::allocator_traits<Allocator>::pointer;
        using const_pointer = std::allocator_traits<Allocator>::const_pointer;
    public:
        static constexpr size_type DefaultGapSize = DefaultGapSize_;
        static constexpr size_type NearnessFactor = NearnessFactor_;
    private:
        template <typename U>
        class iterator_impl
        {
            friend class gap_vector;
        public:
            using qualified_container_type = detail::const_if_const_t<gap_vector, U>;
            using base_iterator = U*;
            using reference = U&;
            using pointer = U*;
            using difference_type = typename gap_vector::difference_type;
            using value_type = typename gap_vector::value_type;
            using iterator_category = std::random_access_iterator_tag;
        public:
            iterator_impl() :
                iContainer{},
                iBase{}
            {
            }
        public:
            iterator_impl(qualified_container_type& aContainer, base_iterator aBase) :
                iContainer{ &aContainer },
                iBase{ aBase }
            {
            }
            iterator_impl(iterator_impl<std::remove_const_t<U>> const& aOther) :
                iContainer{ aOther.iContainer },
                iBase{ aOther.iBase }
            {
            }
            iterator_impl(iterator_impl<std::remove_const_t<U>>&& aOther) :
                iContainer{ aOther.iContainer },
                iBase{ aOther.iBase }
            {
                aOther.iContainer = nullptr;
                aOther.iBase = nullptr;
            }
        public:
            iterator_impl& operator=(iterator_impl<std::remove_const_t<U>> const& aOther)
            {
                iContainer = &aOther.c();
                iBase = aOther.base();
                return *this;
            }
            iterator_impl& operator=(iterator_impl<std::remove_const_t<U>>&& aOther)
            {
                iContainer = &aOther.c();
                iBase = aOther.base();
                return *this;
            }
        public:
            reference operator*() const
            {
                return *iBase;
            }
            pointer operator->() const
            {
                return iBase;
            }
        public:
            iterator_impl& operator++()
            {
                return (*this += 1);
            }
            iterator_impl& operator--()
            {
                return (*this -= 1);
            }
            iterator_impl operator++(int)
            {
                iterator_impl temp = *this;
                operator++();
                return temp;
            }
            iterator_impl operator--(int)
            {
                iterator_impl temp = *this;
                operator--();
                return temp;
            }
            iterator_impl& operator+=(difference_type aDifference)
            {
                if (aDifference == 0)
                    return *this;
                else if (aDifference < 0)
                    return (*this -= -aDifference);
                auto const current = iBase;
                auto const next = current + aDifference;
                auto const gapActive = c().gap_active();
                auto const currentBeforeGap = (current < c().iGapStart);
                auto const nextBeforeGap = (next < c().iGapStart);
                auto const nextEndAtGapStart = (c().iGapEnd == c().iDataEnd && next == c().iGapStart);
                auto const nextAfterGap = (next > c().iGapEnd);
                if (!gapActive || nextBeforeGap || nextEndAtGapStart || (nextAfterGap && !currentBeforeGap))
                    iBase += aDifference;
                else
                    iBase += (aDifference + c().gap_size());
                return *this;
            }
            iterator_impl& operator-=(difference_type aDifference)
            {
                if (aDifference == 0)
                    return *this;
                else if (aDifference < 0)
                    return (*this += -aDifference);
                auto const current = iBase;
                auto const prev = current - aDifference;
                auto const gapActive = c().gap_active();
                auto const currentBeforeGap = (current <= c().iGapStart);
                auto const currentAfterGap = (current >= c().iGapEnd);
                auto const prevAfterGap = (prev >= c().iGapEnd);
                if (!gapActive || currentBeforeGap || (currentAfterGap && prevAfterGap))
                    iBase -= aDifference;
                else
                    iBase -= (aDifference + c().gap_size());
                return *this;
            }
            iterator_impl operator+(difference_type aDifference) const
            { 
                iterator_impl result{ *this };
                result += aDifference; 
                return result; 
            }
            iterator_impl operator-(difference_type aDifference) const
            { 
                iterator_impl result{ *this };
                result -= aDifference; 
                return result; 
            }
        public:
            reference operator[](difference_type aDifference) const 
            { 
                return *((*this) + aDifference); 
            }
        public:
            difference_type operator-(const iterator_impl& aOther) const
            {
                auto result = iBase - aOther.base();
                if (c().gap_active())
                {
                    if (c().before_gap(aOther.base()) && c().after_gap(iBase))
                        result -= c().gap_size();
                    else if (c().before_gap(iBase) && c().after_gap(aOther.base()))
                        result += c().gap_size();
                }
                return result;
            }
        public:
            template <typename U2>
            constexpr bool operator==(const iterator_impl<U2>& that) const noexcept
            {
                return iBase == that.base();
            }
            template <typename U2>
            constexpr std::strong_ordering operator<=>(const iterator_impl<U2>& that) const noexcept
            {
                return iBase <=> that.base();
            }        
        protected:
            constexpr qualified_container_type& c() const noexcept
            {
                return *iContainer;
            }
            constexpr base_iterator const& base() const noexcept
            {
                return iBase;
            }
            constexpr base_iterator& base() noexcept
            {
                return iBase;
            }
        private:
            qualified_container_type* iContainer = nullptr;
            base_iterator iBase = {};
        };
    public:
        using iterator = iterator_impl<value_type>;
        using const_iterator = iterator_impl<value_type const>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    public:
        constexpr gap_vector() noexcept(noexcept(Allocator()))
        {
        }
        constexpr explicit gap_vector(const Allocator& alloc) noexcept : iAlloc{ alloc }
        {
        }
        constexpr gap_vector(size_type count, const value_type& value, const Allocator& alloc = Allocator()) : iAlloc{ alloc }
        {
            resize(count, value);
        }
        constexpr explicit gap_vector(size_type count, const Allocator& alloc = Allocator()) : iAlloc{ alloc }
        {
            resize(count);
        }
        template<class InputIt>
        constexpr gap_vector(InputIt first, InputIt last, const Allocator& alloc = Allocator()) : iAlloc{ alloc }
        {
            (void)insert(begin(), first, last);
        }
        constexpr gap_vector(const gap_vector& other)
        {
            (void)insert(begin(), other.begin(), other.end());
        }
        constexpr gap_vector(const gap_vector& other, const Allocator& alloc) : iAlloc{ alloc }
        {
            (void)insert(begin(), other.begin(), other.end());
        }
        constexpr gap_vector(gap_vector&& other) noexcept
        {
            swap(other);
        }
        constexpr gap_vector(gap_vector&& other, const Allocator& alloc) : iAlloc{ alloc }
        {
            swap(other);
        }
        constexpr gap_vector(std::initializer_list<T> init, const Allocator& alloc = Allocator()) : iAlloc{ alloc }
        {
            (void)insert(begin(), init);
        }
        ~gap_vector()
        {
            clear();
            if (iData != nullptr)
                std::allocator_traits<allocator_type>::deallocate(iAlloc, iData, capacity());
        }
    public:
        constexpr gap_vector& operator=(const gap_vector& other)
        {
            assign(other.begin(), other.end());
            return *this;
        }
        constexpr gap_vector& operator=(gap_vector&& other) noexcept
        {
            gap_vector temp;
            temp.swap(other);
            temp.swap(*this);
            return *this;
        }
        constexpr gap_vector& operator=(std::initializer_list<value_type> ilist)
        {
            assign(ilist);
            return *this;
        }
        constexpr void assign(size_type count, const value_type& value)
        {
            clear();
            insert(begin(), count, value);
        }
        template<class InputIt>
        constexpr void assign(InputIt first, InputIt last) 
        {
            clear();
            insert(begin(), first, last);
        }
        constexpr void assign(std::initializer_list<value_type> ilist)
        {
            clear();
            insert(begin(), ilist);
        }
    public:
        constexpr void swap(gap_vector& other) noexcept
        {
            std::swap(iData, other.iData);
            std::swap(iDataEnd, other.iDataEnd);
            std::swap(iStorageEnd, other.iStorageEnd);
            std::swap(iGapStart, other.iGapStart);
            std::swap(iGapEnd, other.iGapEnd);
        }
    public:
        constexpr allocator_type get_allocator() const noexcept
        {
            return iAlloc;
        }
    public:
        constexpr const_reference at(size_type pos) const
        {
            pos = adjusted_index(pos);
            if (pos < iDataEnd - iData)
                return iData[pos];
            throw std::out_of_range("neolib::gap_vector::at");
        }
        constexpr reference at(size_type pos)
        {
            pos = adjusted_index(pos);
            if (pos < iDataEnd - iData)
                return iData[pos];
            throw std::out_of_range("neolib::gap_vector::at");
        }
        constexpr const_reference operator[](size_type pos) const
        {
            return iData[adjusted_index(pos)];
        }
        constexpr reference operator[](size_type pos)
        {
            return iData[adjusted_index(pos)];
        }
        constexpr const_reference front() const
        {
            return *begin();
        }
        constexpr reference front()
        {
            return *begin();
        }
        constexpr const_reference back() const
        {
            return *std::prev(end());
        }
        constexpr reference back()
        {
            return *std::prev(end());
        }
        constexpr T const* data() const
        {
            unsplit();
            return iData;
        }
        constexpr T* data()
        {
            unsplit();
            return iData;
        }
    public:
        constexpr const_iterator begin() const noexcept
        {
            return const_iterator{ *this, !gap_active() || iData != iGapStart ? iData : iGapEnd };
        }
        constexpr const_iterator cbegin() const noexcept
        {
            return begin();
        }
        constexpr iterator begin() noexcept
        {
            return iterator{ *this, !gap_active() || iData != iGapStart ? iData : iGapEnd };
        }
        constexpr const_iterator end() const noexcept
        {
            return const_iterator{ *this, !gap_active() || iDataEnd != iGapEnd ? iDataEnd : iData == iGapStart ? iGapEnd : iGapStart };
        }
        constexpr const_iterator cend() const noexcept
        {
            return end();
        }
        constexpr iterator end() noexcept
        {
            return iterator{ *this, !gap_active() || iDataEnd != iGapEnd ? iDataEnd : iData == iGapStart ? iGapEnd : iGapStart };
        }
        constexpr const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator{ end() };
        }
        constexpr const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator{ cend() };
        }
        constexpr reverse_iterator rbegin() noexcept
        {
            return reverse_iterator{ end() };
        }
        constexpr const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator{ begin() };
        }
        constexpr const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator{ cbegin() };
        }
        constexpr reverse_iterator rend() noexcept
        {
            return reverse_iterator{ begin() };
        }
    public:
        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return begin() == end();
        }
        constexpr size_type size() const noexcept
        {
            return (iDataEnd - iData) - gap_size();
        }
        constexpr size_type max_size() const noexcept
        {
            return std::numeric_limits<difference_type>::max();
        }
        constexpr void reserve(size_type aNewCapacity)
        {
            if (aNewCapacity <= capacity())
                return;

            unsplit();

            auto const existingCapacity = capacity();
            auto const existingData = iData;
            auto const existingDataEnd = iDataEnd;
            auto const existingStorageEnd = iStorageEnd;

            pointer newStorage = std::allocator_traits<allocator_type>::allocate(iAlloc, aNewCapacity);

            try
            {
                iData = newStorage;
                iDataEnd = newStorage;
                iStorageEnd = newStorage + aNewCapacity;

                iDataEnd = std::uninitialized_move(existingData, existingDataEnd, iData);

                std::allocator_traits<allocator_type>::deallocate(iAlloc, existingData, existingCapacity);
            }
            catch(...)
            {
                std::allocator_traits<allocator_type>::deallocate(iAlloc, newStorage, aNewCapacity);
                iData = existingData;
                iDataEnd = existingDataEnd;
                iStorageEnd = existingStorageEnd;

                throw;
            }
        }
        constexpr size_type capacity() const noexcept
        {
            return iStorageEnd - iData;
        }
        constexpr void shrink_to_fit()
        {
            gap_vector{ *this }.swap(*this);
        }
    public:
        constexpr void clear() noexcept
        {
            unsplit();
            for (auto e = iData; e != iDataEnd; ++e)
                std::allocator_traits<allocator_type>::destroy(iAlloc, e);
            iDataEnd = iData;
        }
        constexpr iterator erase(const_iterator pos)
        {
            return erase(pos, std::next(pos));
        }
        constexpr iterator erase(const_iterator first, const_iterator last)
        {
            if (first == last)
                return std::next(begin(), std::distance(cbegin(), last));
            auto const firstIndex = std::distance(cbegin(), first);
            auto const lastIndex = std::distance(cbegin(), last);
            auto const firstPos = std::next(begin(), firstIndex).base();
            auto const lastPos = std::next(begin(), lastIndex).base();
            auto const garbageCount = lastIndex - firstIndex;
            if (near_gap(firstPos) || near_gap(lastPos))
            {
                if (before_gap(std::prev(lastPos)))
                {
                    for (auto src = lastPos, dest = firstPos; src < iGapStart; ++src, ++dest)
                        *dest = std::move(*src);
                    for (auto garbage = iGapStart - garbageCount; garbage != iGapStart; ++garbage)
                        std::allocator_traits<allocator_type>::destroy(iAlloc, garbage);
                    std::advance(iGapStart, -garbageCount);
                }
                else if (after_gap(firstPos))
                {
                    for (auto src = std::prev(firstPos), dest = std::prev(lastPos); src >= iGapEnd; --src, --dest)
                        *dest = std::move(*src);
                    for (auto garbage = iGapEnd; garbage != iGapEnd + garbageCount; ++garbage)
                        std::allocator_traits<allocator_type>::destroy(iAlloc, garbage);
                    std::advance(iGapEnd, garbageCount);
                }
                else
                {
                    for (auto garbage = firstPos; garbage != lastPos; ++garbage)
                    {
                        if (garbage < iGapStart)
                            std::allocator_traits<allocator_type>::destroy(iAlloc, garbage);
                        else if (garbage >= iGapEnd)
                            std::allocator_traits<allocator_type>::destroy(iAlloc, garbage);
                    }
                    iGapStart = firstPos;
                    iGapEnd = lastPos;
                }
                return std::next(begin(), firstIndex);
            }
            else
            {
                unsplit();
                auto const newFirstPos = std::next(begin(), firstIndex).base();
                for (auto src = std::next(newFirstPos, garbageCount), dest = newFirstPos; src != iDataEnd; ++src, ++dest)
                    *dest = std::move(*src);
                for (auto garbage = iDataEnd - garbageCount; garbage != iDataEnd; ++garbage)
                    std::allocator_traits<allocator_type>::destroy(iAlloc, garbage);
                std::advance(iDataEnd, -garbageCount);
                return iterator{ *this, newFirstPos };
            }
        }
        constexpr void pop_back()
        {
            (void)erase(std::prev(end()));
        }
        constexpr iterator insert(const_iterator pos, value_type const& value)
        {
            auto const memory = allocate_from_gap(pos, 1);
            std::allocator_traits<allocator_type>::construct(iAlloc, memory, value);
            return iterator{ *this, memory };
        }
        constexpr iterator insert(const_iterator pos, value_type&& value)
        {
            auto const memory = allocate_from_gap(pos, 1);
            std::allocator_traits<allocator_type>::construct(iAlloc, memory, std::move(value));
            return iterator{ *this, memory };
        }
        constexpr iterator insert(const_iterator pos, size_type count, value_type const& value)
        {
            auto const memory = allocate_from_gap(pos, count);
            std::uninitialized_fill_n(memory, count, value);
            return iterator{ *this, memory };
        }
        template<class InputIt>
        constexpr iterator insert(const_iterator pos, InputIt first, InputIt last)
        {
            auto const insertPosIndex = std::distance(cbegin(), pos);
            for (auto nextPosIndex = insertPosIndex; first != last; ++nextPosIndex)
            {
                auto const nextPos = std::next(cbegin(), nextPosIndex);
                auto const memory = allocate_from_gap(nextPos, 1);
                std::uninitialized_copy_n(first++, 1, memory);
            }
            return std::next(begin(), insertPosIndex);
        }
        constexpr iterator insert(const_iterator pos, std::initializer_list<value_type> list)
        {
            auto const memory = allocate_from_gap(pos, list.size());
            std::uninitialized_copy(list.begin(), list.end(), memory);
            return iterator{ *this, memory };
        }
        template<class... Args>
        constexpr iterator emplace(const_iterator pos, Args&&... args)
        {
            auto const memory = allocate_from_gap(pos, 1);
            std::allocator_traits<allocator_type>::construct(iAlloc, memory, std::forward<Args>(args)...);
            return iterator{ *this, memory };
        }
        constexpr void push_back(value_type const& value)
        {
            (void)insert(end(), value);
        }
        constexpr void push_back(value_type&& value)
        {
            (void)insert(end(), std::move(value));
        }
        template<class... Args>
        constexpr void emplace_back(Args&&... args)
        {
            (void)emplace(end(), std::forward<Args>(args)...);
        }
        constexpr void resize(size_type count)
        {
            if (count < size())
                (void)erase(std::prev(end(), size() - count), end());
            else if (count > size())
                (void)insert(end(), count - size(), value_type{});
        }
        constexpr void resize(size_type count, value_type const& value)
        {
            if (count < size())
                (void)erase(std::prev(end(), size() - count), end());
            else if (count > size())
                (void)insert(end(), count - size(), value);
        }
    public:
        constexpr void unsplit() const
        {
            if (!gap_active())
                return;
            auto const next = std::uninitialized_move(iGapEnd, std::min(iGapEnd + gap_size(), iDataEnd), iGapStart);
            auto const newEnd = std::move(std::min(iGapEnd + gap_size(), iDataEnd), iDataEnd, next);
            for (auto garbage = std::max(newEnd, iGapEnd); garbage != iDataEnd; ++garbage)
                std::allocator_traits<allocator_type>::destroy(iAlloc, garbage);
            iDataEnd = newEnd;
            iGapStart = nullptr;
            iGapEnd = nullptr;
        }
    private:
        constexpr size_type room() const noexcept
        {
            return capacity() - size();
        }
        constexpr size_type adjusted_index(size_type pos) const noexcept
        {
            if (!gap_active() || pos < static_cast<size_type>(iGapStart - iData))
                return pos;
            return pos + gap_size();
        }
        constexpr bool gap_active() const noexcept
        {
            return iGapStart != iGapEnd;
        }
        constexpr size_type gap_size() const noexcept
        {
            return iGapEnd - iGapStart;
        }
        constexpr bool before_gap(const_pointer aPosition) const noexcept
        {
            return gap_active() && aPosition < iGapStart;
        }
        constexpr bool after_gap(const_pointer aPosition) const noexcept
        {
            return gap_active() && aPosition >= iGapEnd;
        }
        constexpr bool near_gap(const_pointer aPosition) const noexcept
        {
            if (!gap_active())
                return false;
            return std::abs(aPosition - iGapStart) <= DefaultGapSize * NearnessFactor || 
                std::abs(aPosition - iGapEnd) <= DefaultGapSize * NearnessFactor;
        }
        constexpr bool before_gap(const_iterator aPosition) const noexcept
        {
            return before_gap(aPosition.base());
        }
        constexpr bool after_gap(const_iterator aPosition) const noexcept
        {
            return after_gap(aPosition.base());
        }
        constexpr bool near_gap(const_iterator aPosition) const noexcept
        {
            return near_gap(aPosition.base());
        }
        constexpr pointer allocate_from_gap(const_iterator aPosition, size_type aCount)
        {
            if (near_gap(aPosition) && aCount <= gap_size())
            {
                auto const posIndex = std::distance(cbegin(), aPosition);
                auto const pos = std::next(begin(), posIndex).base();
                if (before_gap(aPosition))
                {
                    auto dest = std::next(iGapStart, aCount - 1);
                    auto src = iGapStart;
                    do
                    {
                        --src;
                        if (dest >= iGapStart)
                            std::allocator_traits<allocator_type>::construct(iAlloc, dest--, std::move(*src));
                        else
                            *dest-- = std::move(*src);
                        if (src < pos + aCount)
                            std::allocator_traits<allocator_type>::destroy(iAlloc, src);
                    } while (src != pos);
                    iGapStart += aCount;
                    return pos;
                }
                else if (after_gap(aPosition))
                {
                    if (pos != iGapEnd)
                    {
                        auto dest = std::prev(iGapEnd, aCount);
                        auto src = iGapEnd;
                        do
                        {
                            if (dest < iGapEnd)
                                std::allocator_traits<allocator_type>::construct(iAlloc, dest++, std::move(*src));
                            else
                                *dest++ = std::move(*src);
                            if (src >= pos - aCount)
                                std::allocator_traits<allocator_type>::destroy(iAlloc, src);
                            ++src;
                        } while (src != pos);
                    }
                    iGapEnd -= aCount;
                    return pos - aCount;
                }
                // pos should be iGapStart
                iGapStart += aCount;
                return pos;
            }
            else
            {
                // todo: consider optimising by removing the need to call unsplit
                auto const posIndex = std::distance(cbegin(), aPosition);
                unsplit();
                if (room() < aCount)
                    grow(aCount);
                auto const pos = std::next(iData, posIndex);
                auto const initialGapSize = std::min(room(), DefaultGapSize);
                auto src = iDataEnd;
                auto dest = std::next(iDataEnd, initialGapSize);
                while (src != pos)
                {
                    --src;
                    --dest;
                    if (dest >= iDataEnd)
                        std::allocator_traits<allocator_type>::construct(iAlloc, dest, std::move(*src));
                    else
                        *dest = std::move(*src);
                };
                auto const garbageCount = std::min<size_type>(aCount, iDataEnd - pos);
                for (auto garbage = std::next(pos, garbageCount); garbage != pos;)
                    std::allocator_traits<allocator_type>::destroy(iAlloc, --garbage);
                std::advance(iDataEnd, initialGapSize);

                iGapStart = std::next(pos, aCount);
                iGapEnd = std::next(pos, initialGapSize);
                return pos;
            }
        }
        constexpr void grow(size_type aCount)
        {
            double constexpr GrowthFactor = 1.5;
            reserve(static_cast<size_type>((capacity() + DefaultGapSize + aCount) * GrowthFactor));
        }
    private:
        mutable allocator_type iAlloc = {};
        mutable pointer iData = nullptr;
        mutable pointer iDataEnd = nullptr;
        mutable pointer iStorageEnd = nullptr;
        mutable pointer iGapStart = nullptr;
        mutable pointer iGapEnd = nullptr;
    };

    template <typename T, std::size_t DefaultGapSize, std::size_t NearnessFactor, typename Allocator>
    inline constexpr bool operator==(
        gap_vector<T, DefaultGapSize, NearnessFactor, Allocator> const& lhs, 
        gap_vector<T, DefaultGapSize, NearnessFactor, Allocator> const& rhs)
    {
        return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template <typename T, std::size_t DefaultGapSize, std::size_t NearnessFactor, typename Allocator>
    inline constexpr auto operator<=>(
        gap_vector<T, DefaultGapSize, NearnessFactor, Allocator> const& lhs,
        gap_vector<T, DefaultGapSize, NearnessFactor, Allocator> const& rhs)
    {
        return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
}
