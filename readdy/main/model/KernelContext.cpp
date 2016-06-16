/**
 * << detailed description >>
 *
 * @file KernelContext.cpp
 * @brief Implementation file of the KernelContext.
 * @author clonker
 * @date 18.04.16
 * @todo make proper reference to KernelContext.h, is kBT really indepdendent of t?
 */

#include <readdy/model/KernelContext.h>
#include <readdy/common/make_unique.h>
#include <unordered_map>
#include <boost/log/trivial.hpp>
#include <readdy/common/Utils.h>
#include <readdy/model/_internal/ParticleTypePair.h>

namespace readdy {
    namespace model {
        struct ParticleTypePairHasher {
            std::size_t operator()(const readdy::model::_internal::ParticleTypePair &k) const {
                return hash_value(k);
            }

            std::size_t operator()(const std::tuple<unsigned int, unsigned int> &k) const {
                std::size_t seed = 0;
                const auto& t1 = std::get<0>(k);
                const auto& t2 = std::get<1>(k);
                if(t1 <= t2) {
                    boost::hash_combine(seed, t1);
                    boost::hash_combine(seed, t2);
                } else {
                    boost::hash_combine(seed, t2);
                    boost::hash_combine(seed, t1);
                }
                return seed;
            }
        };

        struct readdy::model::KernelContext::Impl {
            uint typeCounter;
            std::unordered_map<std::string, uint> typeMapping;
            double kBT = 0;
            std::array<double, 3> box_size{};
            std::array<bool, 3> periodic_boundary{};
            std::unordered_map<uint, double> diffusionConstants{};
            std::unordered_map<uint, double> particleRadii{};
            std::unordered_map<unsigned int, std::vector<potentials::PotentialOrder1 *>> potentialO1Registry{};
            std::unordered_map<_internal::ParticleTypePair, std::vector<potentials::PotentialOrder2 *>, readdy::model::ParticleTypePairHasher> potentialO2Registry{};

            double timeStep;
        };


        double KernelContext::getKBT() const {
            return (*pimpl).kBT;
        }

        void KernelContext::setKBT(double kBT) {
            (*pimpl).kBT = kBT;
        }

        void KernelContext::setBoxSize(double dx, double dy, double dz) {
            (*pimpl).box_size = {dx, dy, dz};
        }

        void KernelContext::setPeriodicBoundary(bool pb_x, bool pb_y, bool pb_z) {
            (*pimpl).periodic_boundary = {pb_x, pb_y, pb_z};
        }

        KernelContext::KernelContext() : pimpl(std::make_unique<KernelContext::Impl>()) { }

        std::array<double, 3> &KernelContext::getBoxSize() const {
            return pimpl->box_size;
        }

        std::array<bool, 3> &KernelContext::getPeriodicBoundary() const {
            return pimpl->periodic_boundary;
        }

        KernelContext::KernelContext(const KernelContext &rhs) : pimpl(std::make_unique<KernelContext::Impl>(*rhs.pimpl)) { }

        KernelContext &KernelContext::operator=(const KernelContext &rhs) {
            *pimpl = *rhs.pimpl;
            return *this;
        }

        double KernelContext::getDiffusionConstant(const std::string &particleType) const {
            return pimpl->diffusionConstants[pimpl->typeMapping[particleType]];
        }

        void KernelContext::setDiffusionConstant(const std::string &particleType, double D) {
            pimpl->diffusionConstants[getOrCreateTypeId(particleType)] = D;
        }

        double KernelContext::getTimeStep() const {
            return pimpl->timeStep;
        }

        void KernelContext::setTimeStep(double dt) {
            pimpl->timeStep = dt;
        }

        unsigned int KernelContext::getParticleTypeID(const std::string &name) const {
            return pimpl->typeMapping[name];
        }

        double KernelContext::getDiffusionConstant(uint particleType) const {
            return pimpl->diffusionConstants[particleType];
        }

        double KernelContext::getParticleRadius(const std::string &type) const {
            return getParticleRadius(pimpl->typeMapping[type]);
        }

        double KernelContext::getParticleRadius(const unsigned int &type) const {
            return pimpl->particleRadii[type];
        }

        void KernelContext::setParticleRadius(const std::string &particleType, const double r) {
            pimpl->particleRadii[getOrCreateTypeId(particleType)] = r;
        }

        unsigned int KernelContext::getOrCreateTypeId(const std::string &particleType) {
            uint t_id;
            if (pimpl->typeMapping.find(particleType) != pimpl->typeMapping.end()) {
                t_id = pimpl->typeMapping[particleType];
            } else {
                t_id = ++(pimpl->typeCounter);
                pimpl->typeMapping.emplace(particleType, t_id);
            }
            return t_id;
        }

        void KernelContext::registerOrder2Potential(potentials::PotentialOrder2 &potential, const std::string &type1, const std::string &type2) {
            // wlog: type1 <= type2
            auto type1Id = pimpl->typeMapping[type1];
            auto type2Id = pimpl->typeMapping[type2];
            _internal::ParticleTypePair pp {type1Id, type2Id};
            if(pimpl->potentialO2Registry.find(pp) == pimpl->potentialO2Registry.end()) {
                pimpl->potentialO2Registry.emplace(pp, std::vector<potentials::PotentialOrder2*>());
            }
            potential.configureForTypes(type1Id, type2Id);
            pimpl->potentialO2Registry[pp].push_back(&potential);
        }

        std::vector<potentials::PotentialOrder2 *> KernelContext::getOrder2Potentials(const std::string &type1, const std::string &type2) const {
            return getOrder2Potentials(pimpl->typeMapping[type1], pimpl->typeMapping[type2]);
        }

        std::vector<potentials::PotentialOrder2 *> KernelContext::getOrder2Potentials(const unsigned int type1, const unsigned int type2) const {
            return pimpl->potentialO2Registry[{type1, type2}];
        }

        std::unordered_set<std::tuple<unsigned int, unsigned int>, readdy::model::ParticleTypePairHasher> KernelContext::getAllOrder2RegisteredPotentialTypes() const {
            std::unordered_set<std::tuple<unsigned int, unsigned int>, readdy::model::ParticleTypePairHasher> result {};
            for(auto it = pimpl->potentialO2Registry.begin(); it != pimpl->potentialO2Registry.end(); ++it) {
                result.insert(std::make_tuple(it->first.t1, it->first.t2));
            }
            return result;
        }

        void KernelContext::registerOrder1Potential(potentials::PotentialOrder1 &potential, const std::string &type) {
            auto typeId = pimpl->typeMapping[type];
            if(pimpl->potentialO1Registry.find(typeId) == pimpl->potentialO1Registry.end()) {
                pimpl->potentialO1Registry.emplace(typeId, std::vector<potentials::PotentialOrder1*>());
            }
            potential.configureForType(typeId);
            pimpl->potentialO1Registry[typeId].push_back(&potential);
        }

        std::vector<potentials::PotentialOrder1 *> KernelContext::getOrder1Potentials(const std::string &type) const {
            return getOrder1Potentials(pimpl->typeMapping[type]);
        }

        std::vector<potentials::PotentialOrder1 *> KernelContext::getOrder1Potentials(const unsigned int type) const {
            return pimpl->potentialO1Registry[type];
        }

        std::unordered_set<unsigned int> KernelContext::getAllOrder1RegisteredPotentialTypes() const {
            std::unordered_set<unsigned int> result {};
            for(auto it = pimpl->potentialO1Registry.begin(); it != pimpl->potentialO1Registry.end(); ++it) {
                result.insert(it->first);
            }
            return result;
        }

        struct delete_potential : public std::unary_function<const readdy::model::potentials::Potential*, bool> {
            boost::uuids::uuid id;
            delete_potential(const boost::uuids::uuid &id) : id(id) { }
            bool operator()(const readdy::model::potentials::Potential* potential) {
                return id == potential->getId();
            }
        };

        void KernelContext::deregisterPotential(const boost::uuids::uuid &potential) {
            for(auto it = pimpl->potentialO1Registry.begin(); it != pimpl->potentialO1Registry.end(); ++it) {
                std::remove_if(it->second.begin(), it->second.end(), delete_potential(potential));
            }
            for(auto it = pimpl->potentialO2Registry.begin(); it != pimpl->potentialO2Registry.end(); ++it) {
                std::remove_if(it->second.begin(), it->second.end(), delete_potential(potential));
            }
        }


        KernelContext &KernelContext::operator=(KernelContext &&rhs) = default;

        KernelContext::KernelContext(KernelContext &&rhs) = default;

        KernelContext::~KernelContext() = default;
    }
}





