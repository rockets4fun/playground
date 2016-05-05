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

#define PROFILING_ENABLE_REMOTERY

#ifdef PROFILING_ENABLE_REMOTERY
#   include <Remotery.h>
#endif

#ifdef COMMON_WINDOWS
//#   define PROFILING_ENABLE_BROFILER
#endif

#ifdef PROFILING_ENABLE_BROFILER
#   include <Brofiler.h>
#endif

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

#ifdef PROFILING_ENABLE_BROFILER
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

#ifdef PROFILING_ENABLE_REMOTERY
    Remotery *m_remotery = nullptr;
#endif

    u64 ticksSinceFrameStart() const;

private:
    COMMON_DISABLE_COPY(Profiling)
};

// -------------------------------------------------------------------------------------------------

#ifdef PROFILING_ENABLE_REMOTERY
#   define PROFILING_REMOTERY_SECTION(name) \
        rmt_ScopedCPUSample(name, 0);
#else
#   define PROFILING_REMOTERY_SECTION(name, color)
#endif

#ifdef PROFILING_ENABLE_REMOTERY
#   define PROFILING_REMOTERY_THREAD(name) \
        rmt_SetCurrentThreadName(#name);
#else
#   define PROFILING_REMOTERY_THREAD(name, color)
#endif

// -------------------------------------------------------------------------------------------------

#ifdef PROFILING_ENABLE_BROFILER
#   define PROFILING_BROFILER_SECTION(name, color) \
        BROFILER_CATEGORY(#name, Profiling::toBrofilerColor(color))
#else
#   define PROFILING_BROFILER_SECTION(name, color)
#endif

#ifdef PROFILING_ENABLE_BROFILER
#   define PROFILING_BROFILER_THREAD(name, color) \
        BROFILER_FRAME(#name)
#else
#   define PROFILING_BROFILER_THREAD(name, color)
#endif

// -------------------------------------------------------------------------------------------------

#define PROFILING_SECTION(name, color) \
    static Profiling::Section __section_##name(#name, color); \
    Profiling::SectionGuard __section_guard_##name(__section_##name); \
    PROFILING_REMOTERY_SECTION(name) \
    PROFILING_BROFILER_SECTION(name, color)

#define PROFILING_THREAD(name, color) \
    Profiling::instance()->frameReset(); \
    static Profiling::Section __section_##name(#name, color); \
    Profiling::SectionGuard __section_guard_##name(__section_##name); \
    PROFILING_REMOTERY_THREAD(name) \
    PROFILING_BROFILER_THREAD(name, color)

#endif
