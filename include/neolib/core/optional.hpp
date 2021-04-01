// optional.hpp - v1.3
/*
 *  Copyright (c) 2007 Leigh Johnston.
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
#include <optional>
#include <neolib/core/reference_counted.hpp>
#include <neolib/core/i_optional.hpp>

namespace neolib
{
    template<typename T>
    class optional : public reference_counted<i_optional<abstract_t<T>>>
    {
        typedef optional<T> self_type;
        typedef reference_counted<i_optional<abstract_t<T>>> base_type;
        typedef std::optional<T> container_type;
        // exceptions
    public:
        using typename base_type::not_valid;
        // types
    public:
        typedef i_optional<abstract_t<T>> abstract_type;
        typedef T value_type;
        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef typename base_type::value_type abstract_value_type;
        typedef typename base_type::pointer abstract_pointer;
        typedef typename base_type::const_pointer abstract_const_pointer;
        typedef typename base_type::reference abstract_reference;
        typedef typename base_type::const_reference abstract_const_reference;
        // construction
    public:
        optional() : iValue{ std::nullopt } {}
        optional(abstract_type const& rhs) : iValue{ rhs.valid() ? container_type{ rhs.get() } : container_type{ std::nullopt } } {}
        optional(container_type const& rhs) : iValue{ rhs } {}
        optional(const_reference value) : iValue{ value } {}
        template <typename SFINAE = sfinae>
        optional(abstract_const_reference value, std::enable_if_t<!std::is_same_v<value_type, abstract_value_type>, SFINAE> = sfinae{}) : iValue{ value } {}
        // state
    public:
        bool valid() const override
        {
            return iValue != std::nullopt;
        }
        bool invalid() const override
        {
            return !valid();
        }
        operator bool() const override
        { 
            return valid();
        }
        // element access
    public:
        reference get() override
        {
            if (valid())
                return iValue.value();
            throw not_valid();
        }
        const_reference get() const override
        {
            if (valid())
                return iValue.value();
            throw not_valid();
        }
        reference operator*() override
        { 
            return get();
        }
        const_reference operator*() const override
        { 
            return get();
        }
        pointer operator->() override
        { 
            return &get(); 
        }
        const_pointer operator->() const override
        { 
            return &get(); 
        }
        // modifiers
    public:
        template <typename... Args>
        reference emplace(Args&&... aArgs)
        {
            return iValue.emplace(std::forward<Args>(aArgs)...);
        }
        void reset() override
        { 
            iValue = std::nullopt;
        }
        optional& operator=(std::nullopt_t const&) override
        {
            iValue = std::nullopt;
            return *this;
        }
        optional& operator=(abstract_type const& rhs) override
        { 
            *this = rhs.get();
            return *this;
        }
        optional& operator=(abstract_value_type const& value) override
        {
            iValue = value;
            return *this;
        }
        void swap(optional& rhs)
        {
            iValue.swap(rhs.iValue);
        }
        // container
    public:
        container_type const& container() const
        {
            return iValue;
        }
        container_type& container()
        {
            return iValue;
        }
    private:
        container_type iValue;
    };

    template <typename T>
    inline bool operator<(optional<T> const& lhs, optional<T> const& rhs)
    {
        if (lhs.valid() != rhs.valid())
            return lhs.valid() < rhs.valid();
        if (!lhs.valid())
            return false;
        return lhs.get() < rhs.get();
    }

    template <typename T>
    inline bool operator==(optional<T> const& lhs, optional<T> const& rhs)
    {
        if (lhs.valid() != rhs.valid())
            return false;
        if (lhs.valid())
            return *lhs == *rhs;
        else
            return true;
    }

    template <typename T>
    inline bool operator!=(optional<T> const& lhs, optional<T> const& rhs)
    {
        return !operator==(lhs, rhs);
    }

    template <typename T>
    struct optional_type
    {
        typedef T type;
        static constexpr bool optional = false;
    };
    template <typename T>
    struct optional_type<std::optional<T>>
    {
        typedef T type;
        static constexpr bool optional = true;
    };
    template <typename T>
    struct optional_type<optional<T>>
    {
        typedef T type;
        static constexpr bool optional = true;
    };

    template <typename T>
    using optional_t = typename optional_type<T>::type;
    template <typename T>
    inline constexpr bool is_optional_v = optional_type<T>::optional;
}
