/**
 * << detailed description >>
 *
 * @file Observables.h
 * @brief << brief description >>
 * @author clonker
 * @date 22.11.16
 */

#ifndef READDY_KERNEL_CPU_DENSE_OBSERVABLES_H
#define READDY_KERNEL_CPU_DENSE_OBSERVABLES_H

#include <readdy/model/Observables.h>

namespace readdy {
namespace kernel {
namespace cpu_dense {
class CPUDKernel;

namespace observables {

class CPUDParticlePosition : public readdy::model::ParticlePositionObservable {
public:
    CPUDParticlePosition(CPUDKernel *const kernel, unsigned int stride, const std::vector<std::string> &typesToCount = {});

    virtual void evaluate() override;

protected:
    CPUDKernel *const kernel;
};

class CPUDHistogramAlongAxis : public readdy::model::HistogramAlongAxisObservable {

public:
    CPUDHistogramAlongAxis(CPUDKernel *const kernel, unsigned int stride,
                       const std::vector<double> &binBorders,
                       const std::vector<std::string> &typesToCount,
                       unsigned int axis);

    virtual void evaluate() override;

protected:
    CPUDKernel *const kernel;
    size_t size;
};

class CPUDNParticles : public readdy::model::NParticlesObservable {
public:

    CPUDNParticles(CPUDKernel *const kernel, unsigned int stride, std::vector<std::string> typesToCount = {});


    virtual void evaluate() override;

protected:
    CPUDKernel *const kernel;
};

class CPUDForces : public readdy::model::ForcesObservable {
public:
    CPUDForces(CPUDKernel *const kernel, unsigned int stride, std::vector<std::string> typesToCount = {});

    virtual ~CPUDForces() {}

    virtual void evaluate() override;


protected:
    CPUDKernel *const kernel;
};


}
}
}
}
#endif //READDY_KERNEL_CPU_DENSE_OBSERVABLES_H
