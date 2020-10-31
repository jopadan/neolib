// i_logger.hpp
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
#include <neolib/core/string.hpp>

namespace neolib
{
    namespace logger
    {
        enum class severity
        {
            Debug       = 0,
            Info        = 1,
            Warning     = 2,
            Critical    = 3,
            Fatal       = 4
        };

        struct endl_t {};
        struct flush_t {};

        const endl_t endl;
        const flush_t flush;

        class client_logger_buffers
        {
        protected:
            typedef std::ostringstream buffer_t;
        private:
            typedef std::map<std::thread::id, buffer_t> buffer_list_t;
        public:
            static client_logger_buffers& instance()
            {
                static client_logger_buffers sIntance;
                return sIntance;
            }
        public:
            buffer_t& buffer()
            {
                std::lock_guard<std::recursive_mutex> lg{ mutex() };
                thread_local struct cleanup
                {
                    client_logger_buffers& parent;
                    ~cleanup()
                    {
                        std::lock_guard<std::recursive_mutex> lg{ parent.mutex() };
                        parent.buffers().erase(std::this_thread::get_id());
                    }
                } cleanup{ *this };
                return iBuffers[std::this_thread::get_id()];
            }
            buffer_list_t& buffers()
            {
                return iBuffers;
            }
        private:
            std::recursive_mutex& mutex() const
            {
                return iMutex;
            }
        private:
            mutable std::recursive_mutex iMutex;
            buffer_list_t iBuffers;
        };

        class i_logger
        {
        public:
            virtual ~i_logger() = default;
        public:
            virtual void create_logging_thread() = 0;
        public:
            virtual severity filter_severity() const = 0;
            virtual void set_filter_severity(severity aSeverity) = 0;
        public:
            virtual i_logger& operator<<(severity aSeverity) = 0;
        public:
            i_logger& operator<<(endl_t)
            {
                auto& buffer = client_logger_buffers::instance().buffer();
                buffer << std::endl;
                flush(string{ buffer.str() });
                buffer.str({});
                return *this;
            }
            i_logger& operator<<(flush_t)
            {
                auto& buffer = client_logger_buffers::instance().buffer();
                buffer << std::flush;
                flush(string{ buffer.str() });
                buffer.str({});
                return *this;
            }
            template<typename T>
            i_logger& operator<<(T const& aValue)
            {
                client_logger_buffers::instance().buffer() << aValue;
                return *this;
            }
        public:
            virtual void commit() = 0;
        protected:
            virtual void flush(i_string const& aMessage) = 0;
        };
    }
}