//
// Created by fatih on 8/20/18.
//

#pragma once

extern "C"
{
#include <cwpack.h>
}

#include <string.h>
#include <tos/span.hpp>

namespace tos
{
    namespace msgpack
    {
        class packer;

        class map_packer
        {
        public:
            template <class ValT>
            void insert(const char* name, ValT val);

        private:
            friend class packer;
            map_packer(packer& p, size_t len);

            size_t m_len;
            size_t m_done;
            packer& m_packer;
        };

        class arr_packer
        {
        public:
            template <class T>
            void insert(T val);

            map_packer insert_map(size_t len);
            arr_packer insert_arr(size_t len);

        private:
            friend class packer;
            arr_packer(packer& p, size_t len);

            size_t m_len;
            size_t m_done;
            packer& m_packer;
        };

        class packer
        {
        public:
            explicit packer(span<char> buffer);

            map_packer insert_map(size_t len);

            arr_packer insert_arr(size_t len);

            span<const char> get()
            {
                auto len = m_ctx.current - m_ctx.start;
                return m_buffer.slice(0, len);
            }

            void insert(float val);
            void insert(int32_t val);
            void insert(uint32_t val);
            void insert(uint64_t val);

            void insert(uint8_t val) { insert((uint32_t)val); }

            void insert(const char *str);
            void insert(span<const char> str);

            void insert(span<const uint8_t> binary);

        private:
            span<char> m_buffer;
            cw_pack_context m_ctx;
        };
    }
};

/// IMPL

namespace tos
{
    namespace msgpack
    {
        inline packer::packer(span<char> buffer) : m_buffer{buffer} {
            cw_pack_context_init(&m_ctx, buffer.data(), buffer.size(), nullptr);
        }

        inline void packer::insert(float val) {
            cw_pack_float(&m_ctx, val);
        }

        inline void packer::insert(int32_t val) {
            cw_pack_signed(&m_ctx, val);
        }

        inline void packer::insert(uint32_t val) {
            cw_pack_unsigned(&m_ctx, val);
        }

        inline void packer::insert(uint64_t val) {
            cw_pack_unsigned(&m_ctx, val);
        }

        inline void packer::insert(const char *str) {
            cw_pack_str(&m_ctx, str, strlen(str));
        }

        inline void packer::insert(tos::span<const char> str) {
            cw_pack_str(&m_ctx, str.data(), str.size());
        }

        inline void packer::insert(span <const uint8_t> binary) {
            cw_pack_bin(&m_ctx, binary.data(), binary.size());
        }

        inline arr_packer packer::insert_arr(size_t len) {
            cw_pack_array_size(&m_ctx, len);
            return {*this, len};
        }

        inline map_packer packer::insert_map(size_t len) {
            cw_pack_map_size(&m_ctx, len);
            return {*this, len};
        }

        inline map_packer::map_packer(packer &p, size_t len)
                : m_len{len}, m_done{0}, m_packer(p) {
        }

        template<class ValT>
        void map_packer::insert(const char *name, ValT val) {
            if (m_done == m_len)
            {
                //TODO: error
            }
            m_packer.insert(name);
            m_packer.insert(val);
            ++m_done;
        }

        template<class T>
        void arr_packer::insert(T val) {
            if (m_done == m_len)
            {
                //TODO: error
            }
            m_packer.insert(val);
            ++m_done;
        }

        arr_packer::arr_packer(packer &p, size_t len) : m_len{len}, m_done{0}, m_packer(p) {

        }

        map_packer arr_packer::insert_map(size_t len) {
            return m_packer.insert_map(len);
        }

        arr_packer arr_packer::insert_arr(size_t len) {
            return m_packer.insert_arr(len);
        }
    }
};