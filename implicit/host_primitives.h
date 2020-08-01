#include <stdint.h>
#pragma warning(push)
#pragma warning(disable : 26812)
extern "C" 
{
#define FLT_TYPE float
#define UINT32_TYPE uint32_t
#define UINT8_TYPE uint8_t
#define PACKED

#pragma pack(push, 1)
#include "primitives.h"
#pragma pack(pop)

#undef FLT_TYPE
#undef UINT32_TYPE
#undef UINT8_TYPE
};

#include <glm.hpp>
#include <optional>
#include <iostream>
#include <algorithm>
#include <memory>
#include <unordered_map>

constexpr size_t MAX_ENTITY_COUNT = 32;

namespace entities
{
    struct entity;
    typedef std::shared_ptr<entity> ent_ref;
    
    struct entity
    {
    protected:
        entity() = default;

    public:
        virtual uint8_t type() const = 0;
        virtual bool simple() const = 0;
        virtual void render_data_size(size_t& nBytes, size_t& nEntities, size_t& nSteps) const = 0;
        virtual void copy_render_data(
            uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps,
            size_t& entityIndex, size_t& currentOffset, std::optional<uint32_t> reg = std::nullopt) const = 0;

        template <typename T> static ent_ref wrap_simple(const T& simple)
        {
            return ent_ref(dynamic_cast<entity*>(new T(simple)));
        };
    };

    struct comp_entity : public entity
    {
        ent_ref left;
        ent_ref right;
        op_defn op;

    private:
        comp_entity(ent_ref l, ent_ref r, op_defn op);
        comp_entity(ent_ref, op_defn op);

    public:
        virtual bool simple() const;
        virtual uint8_t type() const;
        virtual void render_data_size(size_t& nBytes, size_t& nEntities, size_t& nSteps) const;
        virtual void copy_render_data(
            uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps,
            size_t& entityIndex, size_t& currentOffset, std::optional<uint32_t> reg = std::nullopt) const;

        comp_entity(const comp_entity&) = delete;
        const comp_entity& operator=(const comp_entity&) = delete;

        template <typename T1, typename T2>
        static ent_ref make_csg(T1 l, T2 r, op_defn op)
        {
            ent_ref ls(l);
            ent_ref rs(r);
            return ent_ref(new comp_entity(ls, rs, op));
        }

        template <typename T1, typename T2>
        static ent_ref make_csg(T1 l, T2 r, op_type op)
        {
            op_defn opdef;
            opdef.type = op;
            opdef.data.blend_radius = 0;
            return make_csg(l, r, opdef);
        };

        template <typename T>
        static ent_ref make_offset(T ent, float distance)
        {
            ent_ref ep(ent);
            op_defn op;
            op.type = op_type::OP_OFFSET;
            op.data.offset_distance = distance;
            return ent_ref(new comp_entity(ep, op));
        };

        template <typename T>
        static ent_ref make_linblend(T l, T r, glm::vec3 p1, glm::vec3 p2)
        {
            op_defn op;
            op.type = op_type::OP_LINBLEND;
            op.data.lin_blend =
            {
                {p1.x, p1.y, p1.z},
                {p2.x, p2.y, p2.z},
            };
            return ent_ref(new comp_entity(l, r, op));
        };

        template <typename T>
        static ent_ref make_smoothblend(T l, T r, glm::vec3 p1, glm::vec3 p2)
        {
            op_defn op;
            op.type = op_type::OP_SMOOTHBLEND;
            op.data.smooth_blend =
            {
                {p1.x, p1.y, p1.z},
                {p2.x, p2.y, p2.z},
            };
            return ent_ref(new comp_entity(l, r, op));
        };
    };

    struct simp_entity : public entity
    {
        const simp_entity& operator=(const simp_entity&) = delete;
    protected:
        simp_entity() = default;

        virtual bool simple() const;
        virtual void render_data_size(size_t& nBytes, size_t& nEntities, size_t& nSteps) const;
        virtual size_t num_render_bytes() const = 0;
        virtual void write_render_bytes(uint8_t*& bytes) const = 0;
        virtual void copy_render_data(
            uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps,
            size_t& entityIndex, size_t& currentOffset, std::optional<uint32_t> reg = std::nullopt) const;
    };

    struct box3 : public simp_entity
    {
        glm::vec3 min;
        glm::vec3 max;
        box3(float xmin, float ymin, float zmin, float xmax, float ymax, float zmax);

        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };

    struct sphere3 : public simp_entity
    {
        glm::vec3 center;
        float radius;
        sphere3(float xcenter, float ycenter, float zcenter, float radius);
        
        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };

    struct cylinder3 : public simp_entity
    {
        glm::vec3 point1;
        glm::vec3 point2;
        float radius;

        cylinder3(float p1x, float p1y, float p1z, float p2x, float p2y, float p2z, float radius);

        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };

    struct gyroid : public simp_entity
    {
        float scale;
        float thickness;
        gyroid(float scale, float thickness);
        
        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };

    struct schwarz : public simp_entity
    {
        float scale;
        float thickness;
        schwarz(float scale, float thickness);

        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };

    struct halfspace : public simp_entity
    {
        glm::vec3 origin;
        glm::vec3 normal;
        halfspace(glm::vec3, glm::vec3);

        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };
}
#pragma warning(pop)