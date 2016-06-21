//
// Created by clonker on 08.04.16.
//

#include <readdy/common/make_unique.h>
#include <readdy/kernel/singlecpu/programs/SingleCPUProgramFactory.h>
#include <readdy/kernel/singlecpu/programs/SingleCPUTestProgram.h>
#include <readdy/kernel/singlecpu/programs/SingleCPUAddParticleProgram.h>
#include <readdy/kernel/singlecpu/programs/SingleCPUDiffuseProgram.h>
#include <readdy/kernel/singlecpu/programs/SingleCPUUpdateStateModelProgram.h>
#include <readdy/kernel/singlecpu/programs/SingleCPUDefaultReactionProgram.h>

namespace readdy {
    namespace kernel {
        namespace singlecpu {
            namespace programs {
                SingleCPUProgramFactory::SingleCPUProgramFactory(SingleCPUKernel *kernel) : kernel(kernel) {
                    namespace core_p = readdy::model::programs;
                    factory[core_p::getProgramName<core_p::TestProgram>()] = [] { return new SingleCPUTestProgram(); };
                    factory[core_p::getProgramName<core_p::AddParticleProgram>()] = [kernel] {
                        return new SingleCPUAddParticleProgram(kernel);
                    };
                    factory[core_p::getProgramName<core_p::DiffuseProgram>()] = [kernel] {
                        return new SingleCPUDiffuseProgram(kernel);
                    };
                    factory[core_p::getProgramName<core_p::UpdateStateModelProgram>()] = [kernel] {
                        return new SingleCPUUpdateStateModelProgram(kernel);
                    };
                    factory[core_p::getProgramName<core_p::DefaultReactionProgram>()] = [kernel] {
                        return new SingleCPUDefaultReactionProgram(kernel);
                    };
                }
            }
        }
    }
}


