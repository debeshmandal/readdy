/********************************************************************
 * Copyright © 2016 Computational Molecular Biology Group,          * 
 *                  Freie Universität Berlin (GER)                  *
 *                                                                  *
 * This file is part of ReaDDy.                                     *
 *                                                                  *
 * ReaDDy is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU Lesser General Public License as   *
 * published by the Free Software Foundation, either version 3 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU Lesser General Public License for more details.              *
 *                                                                  *
 * You should have received a copy of the GNU Lesser General        *
 * Public License along with this program. If not, see              *
 * <http://www.gnu.org/licenses/>.                                  *
 ********************************************************************/


/**
 * This header file contains:
 *     * the `readdy::model::particle_flavor` type which takes values as defined in the `readdy::model::particleflavor`
 *       namespace
 *     * the definition of ParticleTypeInfo, backing structure for all particle types
 *     * the definition of ParticleTypeRegistry, an object managing all particle types for a reaction diffusion system
 *
 * @file ParticleTypeRegistry.h
 * @brief Header file containing definitions for particle flavors, particle type info and the particle type registry.
 * @author clonker
 * @date 29.03.17
 * @copyright GNU Lesser General Public License v3.0
 */

#pragma once

#include <readdy/common/common.h>

#include "Particle.h"

NAMESPACE_BEGIN(readdy)
NAMESPACE_BEGIN(model)

using particle_flavor = std::uint8_t;
NAMESPACE_BEGIN(particleflavor)
static constexpr particle_flavor NORMAL = 0;
static constexpr particle_flavor TOPOLOGY = 1;
static constexpr particle_flavor MEMBRANE = 2;

inline static std::string particle_flavor_to_str(particle_flavor flavor) {
    switch(flavor) {
        case model::particleflavor::NORMAL: return "NORMAL";
        case model::particleflavor::TOPOLOGY: return "TOPOLOGY";
        case model::particleflavor::MEMBRANE: return "MEMBRANE";
        default: return "UNKNOWN";
    }
}
NAMESPACE_END(particleflavor)

struct ParticleTypeInfo {
    std::string name;
    scalar diffusionConstant;
    particle_flavor flavor;
    particle_type_type typeId;

    ParticleTypeInfo(const std::string &name, scalar diffusionConstant,
                     particle_flavor flavor, Particle::type_type typeId);
};

class ParticleTypeRegistry {
public:

    using type_map = std::unordered_map<std::string, particle_type_type>;

    ParticleTypeRegistry() = default;

    ParticleTypeRegistry(const ParticleTypeRegistry &) = default;

    ParticleTypeRegistry &operator=(const ParticleTypeRegistry &) = default;

    ParticleTypeRegistry(ParticleTypeRegistry &&) = default;

    ParticleTypeRegistry &operator=(ParticleTypeRegistry &&) = default;

    ~ParticleTypeRegistry() = default;

    particle_type_type idOf(const std::string &name) const;

    particle_type_type operator()(const std::string &name) const;

    void add(const std::string &name, scalar diffusionConst, particle_flavor flavor = particleflavor::NORMAL);

    const ParticleTypeInfo &infoOf(const std::string &name) const;

    const ParticleTypeInfo &infoOf(Particle::type_type type) const;

    scalar diffusionConstantOf(const std::string &particleType) const;

    scalar diffusionConstantOf(particle_type_type particleType) const;

    const std::size_t &nTypes() const;

    std::vector<particle_type_type> typesFlat() const;

    std::string nameOf(particle_type_type id) const;

    const type_map &typeMapping() const;

    std::string describe() const;

    void configure();

private:

    particle_type_type _idOf(const std::string &name) const;

    std::size_t n_types_ = 0;
    particle_type_type type_counter_ = 0;
    type_map type_mapping_ {};
    std::unordered_map<particle_type_type, ParticleTypeInfo> particle_info_ {};

};

NAMESPACE_END(model)
NAMESPACE_END(readdy)
