// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 14.09.2015
// -------------------------------------------------------------------------------------------------

#ifndef PROFILING_HPP
#define PROFILING_HPP

#include "Common.hpp"

#include <string>
#include <vector>
#include <map>

#include <glm/glm.hpp>

// -------------------------------------------------------------------------------------------------
/// @brief Profiling module
struct Profiling
{
    Profiling();
    virtual ~Profiling();

    static Profiling *instance();

public:
    struct Section;
    struct SectionSample
    {
        u64 timeInUs;
        u64 ticksEnter = 0;
        u64 ticksExit = 0;
        const Section *section;
        u32 callDepth = 0;
    };

    struct Thread;
    struct Section
    {
        Section(const std::string &name, const glm::fvec3 &color);

        void enter();
        void exit();

    private:
        Profiling *m_profiling = nullptr;
        Thread *m_thread = nullptr;
        std::string m_name = "unknown";
        glm::fvec3 m_color;

        std::vector< SectionSample > m_frames;
    };

    struct SectionGuard
    {
        SectionGuard(Section &section);
        ~SectionGuard();

    private:
        COMMON_DISABLE_COPY(SectionGuard);

        Section &m_section;
    };

    struct Thread
    {
        u64 id = 0;
        std::string name = "unknown";
        u32 callDepth = 0;
        std::vector< SectionSample > samples;
    };

private:
    std::map< u64, Thread > m_threads;

    double m_ticksPerUs = 0;
    u64 m_ticksAtStartup = 0;

    u64 ticksSinceStartup() const;
    u64 ticksDiffToTimeInUs(u64 ticksA, u64 ticksB) const;

private:
    COMMON_DISABLE_COPY(Profiling);
};

#define PROFILING_SECTION(name, color) \
    static Profiling::Section __section_##name(#name, color); \
    Profiling::SectionGuard __section_guard_##name(__section_##name);

#endif
