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
    struct Section;

    struct SectionSample
    {
        u64 ticksEnter = 0;
        u64 ticksExit = 0;
        const Section *section = nullptr;
        u32 callDepth = 0;
    };

    struct Thread
    {
        u64 id = 0;
        std::string name = "unknown";
        u32 callDepth = 0;
        std::vector< SectionSample > samples;
    };

public:
    Profiling();
    virtual ~Profiling();

    static Profiling *instance();

    Thread *mainThreadPrevFrame();
    void frameReset();

    double ticksToMs(u64 ticks) const;

public:
    struct Section
    {
        Section(const std::string &name, const glm::fvec3 &color);

        std::string name = "unknown";
        glm::fvec3 color;

        void enter();
        void exit();

    private:
        Thread *m_thread = nullptr;
        Profiling *m_profiling = nullptr;
        std::vector< SectionSample > m_frames;
    };

    struct SectionGuard
    {
        SectionGuard(Section &section);
        ~SectionGuard();

    private:
        COMMON_DISABLE_COPY(SectionGuard)

        Section &m_section;
    };

private:
    std::map< u64, Thread > m_threads;
    std::map< u64, Thread > m_threadsPrevFrame;

    u64 m_ticksPerS = 0;
    double m_ticksPerMs = 0.0;
    u64 m_ticksFrameStart = 0;

    u64 ticksSinceFrameStart() const;

private:
    COMMON_DISABLE_COPY(Profiling)
};

#define PROFILING_SECTION(name, color) \
    static Profiling::Section __section_##name(#name, color); \
    Profiling::SectionGuard __section_guard_##name(__section_##name);

#endif
