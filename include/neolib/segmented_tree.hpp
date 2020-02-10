// segmented_tree.h
/*
 *  Copyright (c) 2020 Leigh Johnston.
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
#include <neolib/utility.hpp>
#include <neolib/segmented_array.hpp>

namespace neolib 
{
    template <typename T, size_t N = 64, typename Alloc = std::allocator<T> >
    class segmented_tree
    {
        typedef segmented_tree<T, N, Alloc> self_type;
        typedef self_type tree_type;
    public:
        typedef T value_type;
        typedef Alloc allocator_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef typename std::allocator_traits<allocator_type>::pointer pointer;
        typedef typename std::allocator_traits<allocator_type>::const_pointer const_pointer;
        typedef typename std::allocator_traits<allocator_type>::difference_type difference_type;
        typedef typename std::allocator_traits<allocator_type>::size_type size_type;
    private:
        class node
        {
            friend class segmented_tree<T, N, Alloc>;
        public:
            typedef typename std::allocator_traits<allocator_type>:: template rebind_alloc<node> node_allocator_type;
            typedef segmented_array<node, N, node_allocator_type> child_list;
        private:
            struct root_place_holder {};
        public:
            node() : 
                iParent{ nullptr }, 
                iContents{ root_place_holder{} }
            {}
            node(const node& other) :
                iParent{ other.iParent },
                iChildren( other.iChildren ),
                iContents{ root_place_holder{} }
            {
                if (!is_root())
                    new (&iContents.value) value_type{ other.iContents.value };
            }
            node(node& parent, const value_type& value) :
                iParent{ &parent }, 
                iContents { value }
            {}
            ~node()
            {
                if (!is_root())
                    iContents.value.~value_type();
            }
        public:
            node& operator=(const node& other)
            {
                (*this).~node();
                iParent = other.iParent;
                iChildren = other.iChildren;
                if (!other.is_root())
                    iContents.value = other.iContents.value;
                else
                    iContents.root = other.iContents.root;
                return *this;
            }
        private:
            bool is_root() const 
            { 
                return iParent == nullptr; 
            }
            const node& parent() const
            {
                if (!is_root())
                    return *iParent;
                return *this;
            }
            node& parent()
            {
                if (!is_root())
                    return *iParent;
                return *this;
            }
            const value_type& value() const 
            { 
                return iContents.value; 
            }
            value_type& value() 
            { 
                return iContents.value; 
            }
            const child_list& children() const 
            { 
                return iChildren; 
            }
            child_list& children() 
            { 
                return iChildren; 
            }
            bool empty() const 
            { 
                return iChildren.empty(); 
            }
            std::size_t depth() const
            {
                std::size_t result = 0;
                node const* n = this;
                while (!n->parent().is_root())
                {
                    ++result;
                    n = &n->parent();
                }
                return result;
            }
        private:

        private:
            node* iParent;
            child_list iChildren;
            union contents
            {
                value_type value;
                root_place_holder root;
                contents(const value_type& value) : value{ value } {}
                contents(const root_place_holder& root) : root{ root } {}
                ~contents() {}
            } iContents;
        };
        typedef typename node::child_list node_child_list;
        typedef typename node::child_list::const_iterator node_child_list_const_iterator;
        typedef typename node::child_list::iterator node_child_list_iterator;
    public:
        enum class iterator_type
        {
            Normal,
            Sibling
        };
        template <iterator_type Type>
        class basic_const_iterator;
        template <iterator_type Type>
        class basic_iterator
        {
            typedef basic_iterator<Type> self_type;
            friend class segmented_tree<T, N, Alloc>;
        public:
            typedef typename tree_type::value_type value_type;
            typedef typename tree_type::difference_type difference_type;
            typedef typename tree_type::pointer pointer;
            typedef typename tree_type::reference reference;
        public:
            basic_iterator() : iParentNode{}, iBaseIterator{} {}
            basic_iterator(node& parentNode, node_child_list_iterator childIterator) : iParentNode{ &parentNode }, iBaseIterator{ childIterator } {}
            template <iterator_type Type2>
            basic_iterator(basic_iterator<Type2> const& other) : iParentNode{ other.iParentNode }, iBaseIterator{ other.iBaseIterator } {}
        public:
            template <iterator_type Type2>
            bool operator==(basic_iterator<Type2> const& other)
            {
                return iParentNode == other.iParentNode && iBaseIterator == other.iBaseIterator;
            }
            template <iterator_type Type2>
            bool operator!=(basic_iterator<Type2> const& other)
            {
                return !(*this == other);
            }
        public:
            self_type& operator++()
            {
                if constexpr (Type == iterator_type::Sibling)
                {
                    ++iBaseIterator;
                }
                else
                {
                    if (children().empty())
                    {
                        ++iBaseIterator;
                        while (iBaseIterator == parent().children().end() && !parent().is_root())
                        {
                            auto& oldParent = parent();
                            iParentNode = &parent().parent();
                            iBaseIterator = std::next(parent().children().iter(oldParent));
                        }
                    }
                    else
                    {
                        iParentNode = &*base();
                        iBaseIterator = children().begin();
                    }
                }
                return *this;
            }
            self_type operator++(int) const
            {
                self_type temp = *this;
                ++temp;
                return temp;
            }
            self_type& operator--()
            {
                if constexpr (Type == iterator_type::Sibling)
                {
                    --iBaseIterator;
                }
                else
                {
                    // todo
                }
                return *this;
            }
            self_type operator--(int) const
            {
                self_type temp = *this;
                --temp;
                return temp;
            }
        public:
            reference operator*() const
            {
                return base()->value();
            }
            pointer operator->() const
            {
                return &(*this);
            }
        public:
            basic_const_iterator<iterator_type::Sibling> cbegin() const
            {
                return basic_const_iterator<iterator_type::Sibling>{ *base(), children().begin() };
            }
            basic_const_iterator<iterator_type::Sibling> begin() const
            {
                return cbegin();
            }
            basic_iterator<iterator_type::Sibling> begin()
            {
                return basic_iterator<iterator_type::Sibling>{ *base(), children().begin() };
            }
            basic_const_iterator<iterator_type::Sibling> cend() const
            {
                return basic_const_iterator<iterator_type::Sibling>{ *base(), children().end() };
            }
            basic_const_iterator<iterator_type::Sibling> end() const
            {
                return cend();
            }
            basic_iterator<iterator_type::Sibling> end()
            {
                return basic_iterator<iterator_type::Sibling>{ *base(), children().end() };
            }
        public:
            std::size_t depth() const
            {
                return base()->depth();
            }
        private:
            node& parent() const { return *iParentNode; }
            node_child_list_iterator base() const { return iBaseIterator; }
            node_child_list& children() const { return base()->children(); }
        private:
            node* iParentNode;
            node_child_list_iterator iBaseIterator;
        };
        template <iterator_type Type>
        class basic_const_iterator
        {
            typedef basic_const_iterator<Type> self_type;
            friend class segmented_tree<T, N, Alloc>;
        public:
            typedef typename tree_type::value_type value_type;
            typedef typename tree_type::difference_type difference_type;
            typedef typename tree_type::const_pointer pointer;
            typedef typename tree_type::const_reference reference;
        public:
            basic_const_iterator() : iParentNode{}, iBaseIterator{} {}
            basic_const_iterator(node const& parentNode, node_child_list_const_iterator childIterator) : iParentNode{ &parentNode }, iBaseIterator{ childIterator } {}
            template <iterator_type Type2>
            basic_const_iterator(basic_const_iterator<Type2> const& other) : iParentNode{ other.iParentNode }, iBaseIterator{ other.iBaseIterator } {}
            template <iterator_type Type2>
            basic_const_iterator(basic_iterator<Type2> const& other) : iParentNode{ other.iParentNode }, iBaseIterator{ other.iBaseIterator } {}
        public:
            template <iterator_type Type2>
            bool operator==(basic_const_iterator<Type2> const& other)
            {
                return iParentNode == other.iParentNode && iBaseIterator == other.iBaseIterator;
            }
            template <iterator_type Type2>
            bool operator!=(basic_const_iterator<Type2> const& other)
            {
                return !(*this == other);
            }
        public:
            self_type& operator++()
            {
                if constexpr (Type == iterator_type::Sibling)
                {
                    ++iBaseIterator;
                }
                else
                {
                    if (children().empty())
                    {
                        ++iBaseIterator;
                        while (iBaseIterator == parent().children().end() && !parent().is_root())
                        {
                            auto& oldParent = parent();
                            iParentNode = &parent().parent();
                            iBaseIterator = std::next(parent().children().iter(oldParent));
                        }
                    }
                    else
                    {
                        iParentNode = &*base();
                        iBaseIterator = children().begin();
                    }
                }
                return *this;
            }
            self_type operator++(int) const
            {
                self_type temp = *this;
                ++temp;
                return temp;
            }
            self_type& operator--()
            {
                if constexpr (Type == iterator_type::Sibling)
                {
                    --iBaseIterator;
                }
                else
                {
                    // todo
                }
                return *this;
            }
            self_type operator--(int) const
            {
                self_type temp = *this;
                --temp;
                return temp;
            }
        public:
            reference operator*() const
            {
                return base()->value();
            }
            pointer operator->() const
            {
                return &(*this);
            }
        public:
            basic_const_iterator<iterator_type::Sibling> cbegin() const
            {
                return basic_const_iterator<iterator_type::Sibling>{ *base(), children().begin() };
            }
            basic_const_iterator<iterator_type::Sibling> begin() const
            {
                return cbegin();
            }
            basic_const_iterator<iterator_type::Sibling> cend() const
            {
                return basic_const_iterator<iterator_type::Sibling>{ *base(), children().end() };
            }
            basic_const_iterator<iterator_type::Sibling> end() const
            {
                return cend();
            }
        public:
            std::size_t depth() const
            {
                return base()->depth();
            }
        private:
            node const& parent() const { return *iParentNode; }
            node_child_list_const_iterator base() const { return iBaseIterator; }
            node_child_list const& children() const { return base()->children(); }
        private:
            node const* iParentNode;
            node_child_list_const_iterator iBaseIterator;
        };
        typedef basic_iterator<iterator_type::Normal> iterator;
        typedef basic_const_iterator<iterator_type::Normal> const_iterator;
        typedef basic_iterator<iterator_type::Sibling> sibling_iterator;
        typedef basic_const_iterator<iterator_type::Sibling> const_sibling_iterator;

        // construction
    public:
        segmented_tree()
        {
        }
        ~segmented_tree() 
        {
        }

        // traversals
    public:
        bool empty() const 
        { 
            return root().empty();
        }

        // modifiers
    public:
        const_iterator cbegin() const
        {
            return const_iterator{ root(), root().children().begin() };
        }
        const_iterator begin() const
        {
            return cbegin();
        }
        iterator begin()
        {
            return iterator{ root(), root().children().begin() };
        }
        const_iterator cend() const
        {
            return const_iterator{ root(), root().children().end() };
        }
        const_iterator end() const
        {
            return cend();
        }
        iterator end()
        {
            return iterator{ root(), root().children().end() };
        }
        const_sibling_iterator csbegin() const
        {
            return const_sibling_iterator{ root(), root().children().begin() };
        }
        const_sibling_iterator sbegin() const
        {
            return csbegin();
        }
        sibling_iterator sbegin()
        {
            return sibling_iterator{ root(), root().children().begin() };
        }
        const_sibling_iterator csend() const
        {
            return const_sibling_iterator{ root(), root().children().end() };
        }
        const_sibling_iterator send() const
        {
            return csend();
        }
        sibling_iterator send()
        {
            return sibling_iterator{ root(), root().children().end() };
        }
        sibling_iterator insert(const_sibling_iterator position, const value_type& value)
        {
            auto& parent = const_cast<node&>(position.parent());
            auto& children = const_cast<node_child_list&>(position.parent().children());
            return sibling_iterator{ parent, children.emplace_insert(position.base(), parent, value) };
        }
        void push_back(const value_type& value)
        {
            push_back(send(), value);
        }
        void push_back(const_iterator parent, const value_type& value)
        {
            auto& parentNode = const_cast<node&>(*parent.base());
            parentNode.children().emplace_back(parentNode, value);
        }
        void push_front(const value_type& value)
        {
            push_font(sbegin(), value);
        }
        void push_front(const_iterator parent, const value_type& value)
        {
            auto& parentNode = const_cast<node&>(*parent.base());
            parentNode.children().emplace_front(parentNode, value);
        }

        // implementation
    private:
        node const& root() const
        {
            return iRoot;
        }
        node& root()
        {
            return iRoot;
        }
        node const& first_node() const
        {
            auto n = &root();
            while (!n->empty())
                n = &n->children()[0];
            return *n;
        }
        node& first_node()
        {
            return const_cast<node&>(const_cast<const self_type&>(*this).first_node());
        }
        node const& last_node() const
        {
            auto n = &root();
            while (!n->empty())
                n = &*std::prev(n->children().end());
            return *n;
        }
        node& last_node()
        {
            return const_cast<node&>(const_cast<const self_type&>(*this).last_node());
        }

        // state
    private:
        node iRoot;
    };
}
