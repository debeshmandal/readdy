/********************************************************************
 * Copyright © 2018 Computational Molecular Biology Group,          *
 *                  Freie Universität Berlin (GER)                  *
 *                                                                  *
 * Redistribution and use in source and binary forms, with or       *
 * without modification, are permitted provided that the            *
 * following conditions are met:                                    *
 *  1. Redistributions of source code must retain the above         *
 *     copyright notice, this list of conditions and the            *
 *     following disclaimer.                                        *
 *  2. Redistributions in binary form must reproduce the above      *
 *     copyright notice, this list of conditions and the following  *
 *     disclaimer in the documentation and/or other materials       *
 *     provided with the distribution.                              *
 *  3. Neither the name of the copyright holder nor the names of    *
 *     its contributors may be used to endorse or promote products  *
 *     derived from this software without specific                  *
 *     prior written permission.                                    *
 *                                                                  *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND           *
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,      *
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF         *
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE         *
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR            *
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,         *
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; *
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER *
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,      *
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    *
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF      *
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                       *
 ********************************************************************/


/**
 * << detailed description >>
 *
 * @file Utils.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 19.04.17
 * @copyright BSD-3
 */

#include <readdy/model/topologies/Utils.h>
#include <sstream>

namespace readdy::model::top::util {

std::string to_gexf(graph::Graph& graph) {
    std::ostringstream ss;
    ss << R"(<?xml version="1.0" encoding="UTF-8"?>)";
    ss << R"(<gexf xmlns="http://www.gexf.net/1.2draft" version="1.2">)";
    ss << R"(<graph mode="static" defaultedgetype="undirected">)";
    {
        ss << "<nodes>";
        std::size_t id = 0;
        for (auto &v : graph.vertices()) {
            ss << "<node id=\"" << v.particleIndex << "\"/>";
            ++id;
        }
        ss << "</nodes>";
    }
    {
        ss << "<edges>";
        std::size_t id = 0;
        for(const auto& [v1, v2] : graph.edges()) {
            ss << "<edge id=\"" << id << "\" " "source=\"" << v1->particleIndex
               << "\" " "target=\"" << v2->particleIndex << "\" />";
            ++id;
        }
        ss << "</edges>";
    }
    ss << "</graph>";
    ss << "</gexf>";
    return ss.str();
}

}
