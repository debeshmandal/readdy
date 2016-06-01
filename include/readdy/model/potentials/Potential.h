/**
 * << detailed description >>
 *
 * @file Potential.h
 * @brief << brief description >>
 * @author clonker
 * @date 31.05.16
 */

#ifndef READDY_MAIN_POTENTIAL_H
#define READDY_MAIN_POTENTIAL_H

#include <string>
#include <array>
#include <readdy/model/Particle.h>

namespace readdy {
    namespace model {
        namespace potentials {

            class Potential {
                const unsigned int id;
                const std::string name;
                const int order;

            public:
                Potential(const unsigned int id, const std::string &name, const int order) : id(id), name(name), order(order) {
                }

                const unsigned int getId() const {
                    return id;
                }

                const std::string &getName() const {
                    return name;
                }

                const int getOrder() const {
                    return order;
                }

                virtual void evaluate() = 0;
            };

            namespace _internal {
                template<typename T>
                struct PotentialName { };
            }
        }
    }
}
#endif //READDY_MAIN_POTENTIAL_H
