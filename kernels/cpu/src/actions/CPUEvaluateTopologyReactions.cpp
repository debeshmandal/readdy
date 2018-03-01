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
 * << detailed description >>
 *
 * @file CPUEvaluateTopologyReactions.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 15.06.17
 * @copyright GNU Lesser General Public License v3.0
 */

#include <readdy/kernel/cpu/actions/CPUEvaluateTopologyReactions.h>
#include <readdy/common/algorithm.h>

namespace readdy {
namespace kernel {
namespace cpu {
namespace actions {
namespace top {

CPUEvaluateTopologyReactions::CPUEvaluateTopologyReactions(CPUKernel *const kernel, scalar timeStep)
        : EvaluateTopologyReactions(timeStep), kernel(kernel) {}

/**
 * Struct holding information about a topology reaction event.
 */
struct CPUEvaluateTopologyReactions::TREvent {
    using index_type = CPUStateModel::data_type::size_type;

    rate_t cumulativeRate{0};
    rate_t rate{0};
    std::size_t topology_idx{0};
    // for topology-topology fusion only
    std::ptrdiff_t topology_idx2{-1};

    std::size_t reaction_idx{0};
    particle_type_type t1{0}, t2{0};
    // idx1 is always the particle that belongs to a topology
    index_type idx1{0}, idx2{0};
    bool spatial {false};

};

template<bool approximated>
bool performReactionEvent(scalar rate, scalar timeStep);

template<>
bool performReactionEvent<true>(const scalar rate, const scalar timeStep) {
    return readdy::model::rnd::uniform_real() < rate * timeStep;
}

template<>
bool performReactionEvent<false>(const scalar rate, const scalar timeStep) {
    return readdy::model::rnd::uniform_real() < 1 - std::exp(-rate * timeStep);
}

void CPUEvaluateTopologyReactions::perform(const util::PerformanceNode &node) {
    auto t = node.timeit();
    auto &model = kernel->getCPUKernelStateModel();
    const auto &context = kernel->context();
    auto &topologies = model.topologies();

    if (!topologies.empty()) {

        auto events = gatherEvents();

        if (!events.empty()) {

            std::vector<readdy::model::top::GraphTopology> new_topologies;

            {
                auto shouldEval = [this](const TREvent &event) {
                    return performReactionEvent<false>(event.rate, timeStep);
                };
                auto depending = [this](const TREvent &e1, const TREvent &e2) {
                    return eventsDependent(e1, e2);
                };
                auto eval = [&](const TREvent &event) {
                    auto &topology = topologies.at(event.topology_idx);
                    if (topology->isDeactivated()) {
                        throw std::logic_error(
                                fmt::format("deactivated topology with idx {} for {} event", event.topology_idx,
                                            event.spatial ? "spatial" : "structural"));
                    }
                    assert(!topology->isDeactivated());
                    if (!event.spatial) {
                        handleStructuralReaction(topologies, new_topologies, event, topology);
                    } else {
                        if (event.topology_idx2 >= 0) {
                            auto &top2 = topologies.at(static_cast<std::size_t>(event.topology_idx2));
                            if (topologyDeactivated(event.topology_idx2)) {
                                throw std::logic_error(
                                        fmt::format("encountered deactivated topology here, oh no (ix {}).",
                                                    event.topology_idx2));
                            }
                            handleTopologyTopologyReaction(topology, top2, event);
                        } else {
                            handleTopologyParticleReaction(topology, event);
                        }
                    }
                };

                auto postPerform = [&](const TREvent &event, std::size_t nDeactivated) {
                    for(auto it = events.begin(); it < events.end() - nDeactivated; ++it) {
                        if (eventsDependent(event, *it)) {
                            //log::critical("here ya go ye filth: {}", sss.str());
                            std::stringstream ss;
                            for(auto _it = events.begin(); _it < events.end() - nDeactivated; ++_it) {
                                ss << fmt::format("\tEvent: {}({}) + {}({})", _it->topology_idx, _it->idx1, _it->topology_idx2, _it->idx2) << "\n";
                            }
                            throw std::logic_error(fmt::format("events list contained event that shouldve been deactivated (t11={}, t12={}, t21={}, t22={}), (ix11={}, ix12={}, ix21={}, ix22={})\nlist:\n{}",
                                                               event.topology_idx, event.topology_idx2, it->topology_idx, it->topology_idx2, event.idx1, event.idx2, it->idx1, it->idx2, ss.str()));
                        }
                    }
                };
                // todo remove post perform
                algo::performEvents(events, shouldEval, depending, eval/*, postPerform*/);
            }

            /*std::size_t nDeactivated = 0;
            const std::size_t nEvents = events.size();
            while (nDeactivated < nEvents) {
                const auto cumulative_rate = (events.end() - nDeactivated - 1)->cumulativeRate;

                const auto x = readdy::model::rnd::uniform_real(c_::zero, cumulative_rate);

                const auto eventIt = std::lower_bound(
                        events.begin(), events.end() - nDeactivated, x, [](const TREvent &elem1, const rate_t elem2) {
                            return elem1.cumulativeRate < elem2;
                        }
                );

                if (eventIt != events.end()) {
                    const auto &event = *eventIt;

                    if (performReactionEvent<false>(event.rate, timeStep)) {
                        log::trace("picked event {} / {} with rate {}", std::distance(events.begin(), eventIt) + 1,
                                   events.size(), eventIt->rate);
                        // perform the event!
                        auto &topology = topologies.at(event.topology_idx);
                        if (topology->isDeactivated()) {
                            throw std::logic_error(fmt::format("deactivated topology with idx {} for {} event", event.topology_idx,
                                          event.spatial ? "spatial" : "structural"));
                        }
                        assert(!topology->isDeactivated());
                        if(!event.spatial) {
                            handleStructuralReaction(topologies, new_topologies, event, topology);
                        } else {
                            if(event.topology_idx2 >= 0) {
                                auto &top2 = topologies.at(static_cast<std::size_t>(event.topology_idx2));
                                if(topologyDeactivated(event.topology_idx2)) {
                                    auto end = events.end() - nDeactivated;
                                    if (eventIt < end) {
                                        if(end < events.end()) {
                                            for (auto it = end; it != events.end(); ++it) {
                                                if (eventsDependent(*it, event)) {
                                                    throw std::logic_error(
                                                            "encountered event with previously disabled topology, hmmm....");
                                                }
                                            }
                                        } else {
                                            if(end == events.end()) {
                                                throw std::logic_error("the end is already @ events.end, we should have aborted?!");
                                            } else {
                                                throw std::logic_error(fmt::format("end goes beyond events.end by {}", std::distance(events.end(), end)));
                                            }
                                        }
                                        throw std::logic_error(fmt::format("encountered deactivated topology here, oh no (ix {}).", event.topology_idx2));
                                    } else {
                                        if(eventIt >= events.end()) {
                                            throw std::logic_error(fmt::format("now this is some bs, we are beyond events.end (by {})", std::distance(eventIt, events.end())));
                                        } else {
                                            throw std::logic_error(fmt::format("we are beyond end by {} and maybe beyond events.end by {}", std::distance(eventIt, end), std::distance(eventIt, events.end())));
                                        }
                                    }
                                }
                                handleTopologyTopologyReaction(topology, top2, event);
                            } else {
                                handleTopologyParticleReaction(topology, event);
                                // todo remove me
                                throw std::logic_error("we shouldnt reach this");
                            }
                        }

                        //log::warn("considering pair {}, {}", event.topology_idx, event.topology_idx2);

                        // loop over all other events, swap events considering this particular topology to the end


                      //deactivate events whose educts have disappeared (including the just handled one)
                        std::stringstream sss;
                        {
                            auto _it = events.begin();
                            scalar cumsum = 0.0;
                            const auto idx1 = event.idx1;
                            const auto idx2 = event.idx2;
                            while (_it < events.end() - nDeactivated) {
                                if (eventsDependent(*_it, event)) {
                                    ++nDeactivated;
                                    {
                                        auto mhm = events.end() - nDeactivated;
                                        sss << fmt::format("swapping t11={}, t12={} <-> t21={}, t22={}", _it->topology_idx, _it->topology_idx2, mhm->topology_idx, mhm->topology_idx2);
                                        sss << "\n";
                                    }
                                    std::iter_swap(_it, events.end() - nDeactivated);
                                } else {
                                    cumsum += _it->rate;
                                    _it->cumulativeRate = cumsum;
                                    ++_it;
                                }
                            }

                        }

                        // todo remove this, check if actually all events have been removed that involve these topologies
                        for(auto it = events.begin(); it < events.end() - nDeactivated; ++it) {
                            if (eventsDependent(event, *it)) {
                                log::critical("here ya go ye filth: {}", sss.str());
                                std::stringstream ss;
                                for(auto _it = events.begin(); _it < events.end() - nDeactivated; ++_it) {
                                    ss << fmt::format("\tEvent: {}({}) + {}({})", _it->topology_idx, _it->idx1, _it->topology_idx2, _it->idx2) << "\n";
                                }
                                throw std::logic_error(fmt::format("events list contained event that shouldve been deactivated (t11={}, t12={}, t21={}, t22={}), (ix11={}, ix12={}, ix21={}, ix22={})\nlist:\n{}",
                                                                   event.topology_idx, event.topology_idx2, it->topology_idx, it->topology_idx2, event.idx1, event.idx2, it->idx1, it->idx2, ss.str()));
                            }
                        }

                    } else {

                        // remove event from the list (ie shift it to the end)
                        ++nDeactivated;

                        // swap with last element that is not yet deactivated
                        std::iter_swap(eventIt, events.end() - nDeactivated);

                        // this element's cumulative rate gets initialized with its own rate
                        eventIt->cumulativeRate = eventIt->rate;
                        if (eventIt > events.begin()) {
                            // and then increased by the cumulative rate of its predecessor
                            eventIt->cumulativeRate += (eventIt - 1)->cumulativeRate;
                        }
                        // now update the cumulative rates of all following elements...
                        auto cumsum = (*eventIt).cumulativeRate;
                        for (auto _it = eventIt + 1; _it < events.end() - nDeactivated; ++_it) {
                            cumsum += (*_it).rate;
                            (*_it).cumulativeRate = cumsum;
                        }

                    }

                } else {
                    log::critical("this should not happen (event not found by drawn rate)");
                    throw std::logic_error("this should not happen (event not found by drawn rate)");
                }
            }
             */

            if (!new_topologies.empty()) {
                for (auto &&top : new_topologies) {
                    if (!top.isNormalParticle(*kernel)) {
                        // we have a new topology here, update data accordingly.
                        top.updateReactionRates(context.topology_registry().structuralReactionsOf(top.type()));
                        top.configure();
                        model.insert_topology(std::move(top));
                    } else {
                        // if we have a single particle that is not of flavor topology, remove from topology structure!
                        model.getParticleData()->entry_at(top.getParticles().front()).topology_index = -1;
                    }
                }
            }
        }
    }
}

void CPUEvaluateTopologyReactions::handleStructuralReaction(CPUStateModel::topologies_vec &topologies,
                                                            std::vector<CPUStateModel::topology> &new_topologies,
                                                            const CPUEvaluateTopologyReactions::TREvent &event,
                                                            CPUStateModel::topology_ref &topology) const {
    const auto &topology_type_registry = kernel->context().topology_registry();
    auto &reaction = topology_type_registry.structuralReactionsOf(topology->type()).at(static_cast<std::size_t>(event.reaction_idx));
    auto result = reaction.execute(*topology, kernel);
    if (!result.empty()) {
        // we had a topology fission, so we need to actually remove the current topology from the
        // data structure
        topologies.erase(topologies.begin() + event.topology_idx);
        //log::error("erased topology with index {}", event.topology_idx);
        assert(topology->isDeactivated());

        {/*
            // todo remove me
            for(auto &t1 : result) {
                if (t1.graph().vertices().size() != t1.getNParticles()) {
                    throw std::logic_error("some shit");
                }
                std::vector<int> counts(t1.graph().vertices().size(), 0);
                for (const auto &edge : t1.graph().edges()) {
                    const auto &v1 = std::get<0>(edge);
                    const auto &v2 = std::get<1>(edge);

                    counts.at(v1->particleIndex) += 1;
                    counts.at(v2->particleIndex) += 1;
                }

                int nEnds = 0;
                for (const auto c : counts) {
                    if (c == 1) {
                        ++nEnds;
                    }
                }

                if (nEnds != 2) {
                    std::stringstream ss;
                    for (const auto c : counts) {
                        ss << c << ", ";
                    }
                    throw std::logic_error(fmt::format(
                            "number of ends should always be 2 but was {} after structural reaction (counts: {})",
                            nEnds, ss.str()));
                }
            }
        */}
        for (auto &it : result) {
            if(!it.isNormalParticle(*kernel)) {
                new_topologies.push_back(std::move(it));
            }
        }
    } else {
        if (topology->isNormalParticle(*kernel)) {
            kernel->getCPUKernelStateModel().getParticleData()->entry_at(topology->getParticles().front()).topology_index = -1;
            topologies.erase(topologies.begin() + event.topology_idx);
            //log::error("erased topology with index {}", event.topology_idx);
            assert(topology->isDeactivated());
        }
    }
}

CPUEvaluateTopologyReactions::topology_reaction_events CPUEvaluateTopologyReactions::gatherEvents() {
    topology_reaction_events events;
    const auto &topology_types = kernel->context().topology_registry();
    {
        rate_t current_cumulative_rate = 0;
        std::size_t topology_idx = 0;
        for (auto &top : kernel->getCPUKernelStateModel().topologies()) {
            if (!top->isDeactivated()) {
                std::size_t reaction_idx = 0;
                for (const auto &reaction : topology_types.structuralReactionsOf(top->type())) {
                    TREvent event{};
                    event.rate = top->rates().at(reaction_idx);
                    event.cumulativeRate = event.rate + current_cumulative_rate;
                    current_cumulative_rate = event.cumulativeRate;
                    event.topology_idx = topology_idx;
                    event.reaction_idx = reaction_idx;

                    events.push_back(event);
                    ++reaction_idx;
                }
            }
            ++topology_idx;
        }

        const auto &context = kernel->context();

        static const CPUStateModel::topology_ref EMPTY_TOP {};

        if (!context.topology_registry().spatialReactionRegistry().empty()) {
            const auto &model = kernel->getCPUKernelStateModel();
            const auto &top_registry = context.topology_registry();
            const auto &d2 = context.distSquaredFun();
            const auto &data = *kernel->getCPUKernelStateModel().getParticleData();
            const auto &nl = *kernel->getCPUKernelStateModel().getNeighborList();
            const auto &topologies = kernel->getCPUKernelStateModel().topologies();

            for (std::size_t cell = 0; cell < nl.nCells(); ++cell) {
                for(auto itParticle = nl.particlesBegin(cell); itParticle != nl.particlesEnd(cell); ++itParticle) {
                    const auto &entry = data.entry_at(*itParticle);
                    if (!entry.deactivated && top_registry.isSpatialReactionType(entry.type)) {
                        const auto entryTopologyDeactivated = topologyDeactivated(entry.topology_index);
                        const auto hasEntryTop = entry.topology_index >= 0 && !entryTopologyDeactivated;

                        nl.forEachNeighbor(*itParticle, cell, [&](std::size_t neighborIndex) {
                            const auto &neighbor = data.entry_at(neighborIndex);
                            const auto neighborTopDeactivated = topologyDeactivated(neighbor.topology_index);
                            const auto hasNeighborTop = neighbor.topology_index >= 0 && !neighborTopDeactivated;
                            if ((!hasEntryTop && !hasNeighborTop) || (hasNeighborTop && *itParticle > neighborIndex)) {
                                // use symmetry or skip entirely
                                return;
                            }
                            topology_type_type tt1 = hasEntryTop ? topologies.at(
                                    static_cast<std::size_t>(entry.topology_index))->type()
                                                                 : static_cast<topology_type_type>(-1);
                            topology_type_type tt2 = hasNeighborTop ? topologies.at(
                                    static_cast<std::size_t>(neighbor.topology_index))->type()
                                                                    : static_cast<topology_type_type>(-1);

                            const auto distSquared = d2(entry.pos, neighbor.pos);
                            std::size_t reaction_index = 0;
                            const auto &otherTop = hasNeighborTop ? model.topologies().at(
                                    static_cast<std::size_t>(neighbor.topology_index)
                            ) : EMPTY_TOP;
                            const auto &reactions = top_registry.spatialReactionsByType(entry.type, tt1,
                                                                                        neighbor.type, tt2);
                            for (const auto &reaction : reactions) {
                                if (!reaction.allow_self_connection() &&
                                    entry.topology_index == neighbor.topology_index) {
                                    ++reaction_index;
                                    continue;
                                }
                                if (distSquared < reaction.radius() * reaction.radius()) {
                                    TREvent event{};
                                    event.rate = reaction.rate();
                                    event.cumulativeRate = event.rate + current_cumulative_rate;
                                    current_cumulative_rate = event.cumulativeRate;
                                    if (hasEntryTop && !hasNeighborTop) {
                                        // entry is a topology, neighbor an ordinary particle
                                        event.topology_idx = static_cast<std::size_t>(entry.topology_index);
                                        event.t1 = entry.type;
                                        event.t2 = neighbor.type;
                                        event.idx1 = *itParticle;
                                        event.idx2 = neighborIndex;
                                    } else if (!hasEntryTop && hasNeighborTop) {
                                        // neighbor is a topology, entry an ordinary particle
                                        event.topology_idx = static_cast<std::size_t>(neighbor.topology_index);
                                        event.t1 = neighbor.type;
                                        event.t2 = entry.type;
                                        event.idx1 = neighborIndex;
                                        event.idx2 = *itParticle;
                                    } else if (hasEntryTop && hasNeighborTop) {
                                        // this is a topology-topology fusion
                                        event.topology_idx = static_cast<std::size_t>(entry.topology_index);
                                        event.topology_idx2 = static_cast<std::size_t>(neighbor.topology_index);
                                        // todo remove
                                        {/*
                                            if (topologyDeactivated(event.topology_idx)) {
                                                throw std::logic_error("hier sollte man aber nicht hinkommen 1");
                                            }
                                            if (topologyDeactivated(event.topology_idx2)) {
                                                throw std::logic_error("hier sollte man aber nicht hinkommen 1");
                                            }
                                        */}
                                        event.t1 = entry.type;
                                        event.t2 = neighbor.type;
                                        event.idx1 = *itParticle;
                                        event.idx2 = neighborIndex;
                                    } else {
                                        log::critical("got no topology for topology-fusion");
                                    }
                                    event.reaction_idx = reaction_index;
                                    event.spatial = true;

                                    events.push_back(event);
                                }
                                ++reaction_index;
                            }
                        });
                    }
                }
            }
        }
    }
    return events;
}

void CPUEvaluateTopologyReactions::handleTopologyParticleReaction(CPUStateModel::topology_ref &topology,
                                                                  const CPUEvaluateTopologyReactions::TREvent &event) {
    const auto& context = kernel->context();
    const auto& top_registry = context.topology_registry();
    const auto& reaction = top_registry.spatialReactionsByType(event.t1, topology->type(), event.t2,
                                                               topology_type_empty).at(event.reaction_idx);

    auto& model = kernel->getCPUKernelStateModel();
    auto& data = *model.getParticleData();

    auto& entry1 = data.entry_at(event.idx1);
    auto& entry2 = data.entry_at(event.idx2);
    auto& entry1Type = entry1.type;
    auto& entry2Type = entry2.type;
    if(entry1Type == reaction.type1()) {
        entry1Type = reaction.type_to1();
        entry2Type = reaction.type_to2();
    } else {
        entry1Type = reaction.type_to2();
        entry2Type = reaction.type_to1();
    }
    if(event.topology_idx2 < 0) {
        entry1.topology_index = event.topology_idx;
        entry2.topology_index = event.topology_idx;

        if(reaction.is_fusion()) {
            topology->appendParticle(event.idx2, entry2Type, event.idx1, entry1Type);
        } else {
            topology->vertexForParticle(event.idx1)->setParticleType(entry1Type);
        }
    } else {
        throw std::logic_error("this branch should never be reached as topology-topology reactions are "
                                       "handeled in a different method");
    }
    if(topology->type() == reaction.top_type1()) {
        topology->type() = reaction.top_type_to1();
    } else {
        topology->type() = reaction.top_type_to2();
    }
    topology->updateReactionRates(context.topology_registry().structuralReactionsOf(topology->type()));
    topology->configure();
}

void CPUEvaluateTopologyReactions::handleTopologyTopologyReaction(CPUStateModel::topology_ref &t1,
                                                                  CPUStateModel::topology_ref &t2,
                                                                  const TREvent &event) {
    const auto& context = kernel->context();
    const auto& top_registry = context.topology_registry();
    const auto& reaction = top_registry.spatialReactionsByType(event.t1, t1->type(), event.t2, t2->type()).at(event.reaction_idx);

    auto& model = kernel->getCPUKernelStateModel();
    auto& data = *model.getParticleData();

    auto &entry1 = data.entry_at(event.idx1);
    auto &entry2 = data.entry_at(event.idx2);
    auto &entry1Type = entry1.type;
    auto &entry2Type = entry2.type;

    // todo remove
    {/*
        if (context.particle_types().nameOf(entry1Type) != "Head") {
            throw std::logic_error(fmt::format("entry 1 was no head for reaction {} but was {}",
                                               top_registry.generateSpatialReactionRepresentation(reaction),
                                               context.particle_types().nameOf(entry1Type)));
        }
        if (context.particle_types().nameOf(entry2Type) != "Head") {
            throw std::logic_error(fmt::format("entry 2 was no head for reaction {} but was {}",
                                               top_registry.generateSpatialReactionRepresentation(reaction),
                                               context.particle_types().nameOf(entry2Type)));
        }
    */}
    auto top_type_to1 = reaction.top_type_to1();
    auto top_type_to2 = reaction.top_type_to2();
    if(entry1Type == reaction.type1() && t1->type() == reaction.top_type1()) {
        entry1Type = reaction.type_to1();
        entry2Type = reaction.type_to2();
    } else {
        std::swap(top_type_to1, top_type_to2);
        entry1Type = reaction.type_to2();
        entry2Type = reaction.type_to1();
    }

    // topology - topology fusion
    if(reaction.is_fusion()) {
        if(top_type_to1 == -1) {
            throw std::logic_error("this shouldnt happen in a fusion");
        }
        //topology->appendTopology(otherTopology, ...)
        if(event.topology_idx == event.topology_idx2) {
            // introduce edge if not already present
            auto v1 = t1->vertexForParticle(event.idx1);
            auto v2 = t1->vertexForParticle(event.idx2);
            if(!t1->graph().containsEdge(v1, v2)) {
                t1->graph().addEdge(v1, v2);
            }
        } else {
            // merge topologies
            for(auto pidx : t2->getParticles()) {
                data.entry_at(pidx).topology_index = event.topology_idx;
            }
            auto &topologies = kernel->getCPUKernelStateModel().topologies();
            t1->appendTopology(*t2, event.idx2, entry2Type, event.idx1, entry1Type, top_type_to1);
            topologies.erase(topologies.begin() + event.topology_idx2);
        }
    } else {
        t1->vertexForParticle(event.idx1)->setParticleType(entry1Type);
        t2->vertexForParticle(event.idx2)->setParticleType(entry2Type);
        t1->type() = top_type_to1;
        t2->type() = top_type_to2;

        t2->updateReactionRates(context.topology_registry().structuralReactionsOf(t2->type()));
        t2->configure();
    }
    t1->updateReactionRates(context.topology_registry().structuralReactionsOf(t1->type()));
    t1->configure();

    // todo remove
    {/*
        if (t1->graph().vertices().size() != t1->getNParticles()) {
            throw std::logic_error("some shit");
        }
        std::vector<int> counts(t1->graph().vertices().size(), 0);
        for (const auto edge : t1->graph().edges()) {
            const auto &v1 = std::get<0>(edge);
            const auto &v2 = std::get<1>(edge);

            counts.at(v1->particleIndex) += 1;
            counts.at(v2->particleIndex) += 1;
        }

        int nEnds = 0;
        for (const auto c : counts) {
            if (c == 1) {
                ++nEnds;
            }
        }

        if (nEnds != 2) {
            std::stringstream ss;
            for (const auto c : counts) {
                ss << c << ", ";
            }
            throw std::logic_error(
                    fmt::format("number of ends should always be 2 but was {} after spatial reaction (counts: {})",
                                nEnds, ss.str()));
        }
    */}
}

bool CPUEvaluateTopologyReactions::eventsDependent(const CPUEvaluateTopologyReactions::TREvent &evt1,
                                                   const CPUEvaluateTopologyReactions::TREvent &evt2) const {
    if(evt1.topology_idx == evt2.topology_idx || evt1.topology_idx == evt2.topology_idx2) {
        return true;
    }
    return evt1.topology_idx2 >= 0 && (evt1.topology_idx2 == evt2.topology_idx || evt1.topology_idx2 == evt2.topology_idx2);
}

bool CPUEvaluateTopologyReactions::topologyDeactivated(std::ptrdiff_t index) const {
    return index < 0 || kernel->getCPUKernelStateModel().topologies().at(
            static_cast<std::size_t>(index))->isDeactivated();
}

}
}
}
}
}