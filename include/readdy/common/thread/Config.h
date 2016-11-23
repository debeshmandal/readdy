/**
 * << detailed description >>
 *
 * @file Config.h
 * @brief << brief description >>
 * @author clonker
 * @date 05.09.16
 */

#ifndef READDY_MAIN_CONFIG_H
#define READDY_MAIN_CONFIG_H

namespace readdy {
namespace util {
namespace thread {

struct Config {
    using n_threads_t = decltype(std::thread::hardware_concurrency());

    Config();
    n_threads_t nThreads() const;
    void setNThreads(const n_threads_t);
private:
    n_threads_t m_nThreads;
};

}
}
}
#endif //READDY_MAIN_CONFIG_H