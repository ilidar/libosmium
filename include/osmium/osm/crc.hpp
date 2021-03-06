#ifndef OSMIUM_OSM_CRC_HPP
#define OSMIUM_OSM_CRC_HPP

/*

This file is part of Osmium (http://osmcode.org/libosmium).

Copyright 2013-2015 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <cstdint>

#include <osmium/osm/area.hpp>
#include <osmium/osm/changeset.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/node_ref_list.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/util/endian.hpp>

namespace osmium {

    namespace util {

        inline uint16_t byte_swap_16(uint16_t value) noexcept {
# if defined(__GNUC__) || defined(__clang__)
            return __builtin_bswap16(value);
# else
            return (value >> 8) | (value << 8);
# endif
        }

        inline uint32_t byte_swap_32(uint32_t value) noexcept {
# if defined(__GNUC__) || defined(__clang__)
            return __builtin_bswap32(value);
# else
            return  (value >> 24) |
                   ((value >>  8) & 0x0000FF00) |
                   ((value <<  8) & 0x00FF0000) |
                    (value << 24);
# endif
        }

        inline uint64_t byte_swap_64(uint64_t value) noexcept {
# if defined(__GNUC__) || defined(__clang__)
            return __builtin_bswap64(value);
# else
            uint64_t val1 = byte_swap_32(value & 0xFFFFFFFF);
            uint64_t val2 = byte_swap_32(value >> 32);
            return (val1 << 32) | val2;
# endif
        }

    }

    template <class TCRC>
    class CRC {

        TCRC m_crc;

    public:

        TCRC& operator()() {
            return m_crc;
        }

        const TCRC& operator()() const {
            return m_crc;
        }

        void update_bool(const bool value) {
            m_crc.process_byte(value);
        }

        void update_int8(const uint8_t value) {
            m_crc.process_byte(value);
        }

        void update_int16(const uint16_t value) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
            m_crc.process_bytes(&value, sizeof(uint16_t));
#else
            uint16_t v = osmium::util::byte_swap_16(value);
            m_crc.process_bytes(&v, sizeof(uint16_t));
#endif
        }

        void update_int32(const uint32_t value) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
            m_crc.process_bytes(&value, sizeof(uint32_t));
#else
            uint32_t v = osmium::util::byte_swap_32(value);
            m_crc.process_bytes(&v, sizeof(uint32_t));
#endif
        }

        void update_int64(const uint64_t value) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
            m_crc.process_bytes(&value, sizeof(uint64_t));
#else
            uint64_t v = osmium::util::byte_swap_64(value);
            m_crc.process_bytes(&v, sizeof(uint64_t));
#endif
        }

        void update_string(const char* str) {
            while (*str) {
                m_crc.process_byte(*str++);
            }
        }

        void update(const Timestamp& timestamp) {
            update_int32(uint32_t(timestamp));
        }

        void update(const osmium::Location& location) {
            update_int32(location.x());
            update_int32(location.y());
        }

        void update(const osmium::Box& box) {
            update(box.bottom_left());
            update(box.top_right());
        }

        void update(const NodeRef& node_ref) {
            update_int64(node_ref.ref());
        }

        void update(const NodeRefList& node_refs) {
            for (const NodeRef& node_ref : node_refs) {
                update(node_ref);
            }
        }

        void update(const TagList& tags) {
            for (const Tag& tag : tags) {
                update_string(tag.key());
                update_string(tag.value());
            }
        }

        void update(const osmium::RelationMember& member) {
            update_int64(member.ref());
            update_int16(uint16_t(member.type()));
            update_string(member.role());
        }

        void update(const osmium::RelationMemberList& members) {
            for (const RelationMember& member : members) {
                update(member);
            }
        }

        void update(const osmium::OSMObject& object) {
            update_int64(object.id());
            update_bool(object.visible());
            update_int32(object.version());
            update(object.timestamp());
            update_int32(object.uid());
            update_string(object.user());
            update(object.tags());
        }

        void update(const osmium::Node& node) {
            update(static_cast<const osmium::OSMObject&>(node));
            update(node.location());
        }

        void update(const osmium::Way& way) {
            update(static_cast<const osmium::OSMObject&>(way));
            update(way.nodes());
        }

        void update(const osmium::Relation& relation) {
            update(static_cast<const osmium::OSMObject&>(relation));
            update(relation.members());
        }

        void update(const osmium::Area& area) {
            update(static_cast<const osmium::OSMObject&>(area));
            for (auto it = area.cbegin(); it != area.cend(); ++it) {
                if (it->type() == osmium::item_type::outer_ring ||
                    it->type() == osmium::item_type::inner_ring) {
                    update(static_cast<const osmium::NodeRefList&>(*it));
                }
            }
        }

        void update(const osmium::Changeset& changeset) {
            update_int64(changeset.id());
            update(changeset.created_at());
            update(changeset.closed_at());
            update(changeset.bounds());
            update_int32(changeset.num_changes());
            update_int32(changeset.uid());
            update_string(changeset.user());
            update(changeset.tags());
        }

    }; // class CRC

} // namespace osmium

#endif // OSMIUM_OSM_CRC
