// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 14.09.2015
// -------------------------------------------------------------------------------------------------

#ifndef PROFILER_HPP
#define PROFILER_HPP

#include "Common.hpp"

#include <string>
#include <vector>
#include <map>

#include <glm/glm.hpp>

#define PROFILER_ENABLE_REMOTERY

#ifdef PROFILER_ENABLE_REMOTERY
#   include <Remotery.h>
#endif

#ifdef COMMON_WINDOWS
//#   define PROFILER_ENABLE_BROFILER
#endif

#ifdef PROFILER_ENABLE_BROFILER
#   include <Brofiler.h>
#endif

// -------------------------------------------------------------------------------------------------
/// @brief Profiler module
struct Profiler
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
    Profiler();
    virtual ~Profiler();

    static Profiler *instance();

    Thread *mainThreadPrevFrame();
    void frameReset();

    double ticksToMs(u64 ticks) const;

#ifdef PROFILER_ENABLE_BROFILER
    static u32 toBrofilerColor(const glm::fvec3 &color);
#endif

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
        Profiler *m_profiling = nullptr;
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

#ifdef PROFILER_ENABLE_REMOTERY
    Remotery *m_remotery = nullptr;
#endif

    u64 ticksSinceFrameStart() const;

private:
    COMMON_DISABLE_COPY(Profiler)
};

// -------------------------------------------------------------------------------------------------

#ifdef PROFILER_ENABLE_REMOTERY
#   define PROFILER_REMOTERY_SECTION(name) \
        rmt_ScopedCPUSample(name, 0);
#else
#   define PROFILER_REMOTERY_SECTION(name, color)
#endif

#ifdef PROFILER_ENABLE_REMOTERY
#   define PROFILER_REMOTERY_THREAD(name) \
        rmt_SetCurrentThreadName(#name);
#else
#   define PROFILER_REMOTERY_THREAD(name, color)
#endif

// -------------------------------------------------------------------------------------------------

#ifdef PROFILER_ENABLE_BROFILER
#   define PROFILER_BROFILER_SECTION(name, color) \
        BROFILER_CATEGORY(#name, Profiler::toBrofilerColor(color))
#else
#   define PROFILER_BROFILER_SECTION(name, color)
#endif

#ifdef PROFILER_ENABLE_BROFILER
#   define PROFILER_BROFILER_THREAD(name, color) \
        BROFILER_FRAME(#name)
#else
#   define PROFILER_BROFILER_THREAD(name, color)
#endif

// -------------------------------------------------------------------------------------------------

#define PROFILER_SECTION(name, color) \
    static Profiler::Section __section_##name(#name, color); \
    Profiler::SectionGuard __section_guard_##name(__section_##name); \
    PROFILER_REMOTERY_SECTION(name) \
    PROFILER_BROFILER_SECTION(name, color)

#define PROFILER_THREAD(name, color) \
    Profiler::instance()->frameReset(); \
    static Profiler::Section __section_##name(#name, color); \
    Profiler::SectionGuard __section_guard_##name(__section_##name); \
    PROFILER_REMOTERY_THREAD(name) \
    PROFILER_BROFILER_THREAD(name, color)

#endif
