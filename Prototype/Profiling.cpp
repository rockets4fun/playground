// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 14.09.2015
// -------------------------------------------------------------------------------------------------

#include "Profiling.hpp"

#include <SDL.h>

// -------------------------------------------------------------------------------------------------
Profiling::Profiling()
{
    m_ticksPerS = SDL_GetPerformanceFrequency();
    m_ticksPerMs = double(m_ticksPerS) / 1000.0;
}

// -------------------------------------------------------------------------------------------------
Profiling::~Profiling()
{
}

// -------------------------------------------------------------------------------------------------
Profiling *Profiling::instance()
{
    static Profiling profiling;
    return &profiling;
}

// -------------------------------------------------------------------------------------------------
Profiling::Thread *Profiling::mainThreadPrevFrame()
{
    if (m_threadsPrevFrame.empty())
    {
        return nullptr;
    }
    return &m_threadsPrevFrame.begin()->second;
}

// -------------------------------------------------------------------------------------------------
void Profiling::frameReset()
{
    m_threadsPrevFrame = m_threads;
    for (auto &threadMapIter : m_threads)
    {
        COMMON_ASSERT(threadMapIter.second.callDepth == 0);
        threadMapIter.second.samples.clear();
    }
    m_ticksFrameStart = SDL_GetPerformanceCounter();
}

// -------------------------------------------------------------------------------------------------
double Profiling::ticksToMs(u64 ticks) const
{
    u64 seconds = ticks / m_ticksPerS;
    u64 ticksRemaining = ticks % m_ticksPerS;
    double milliseconds = double(seconds) * 1000.0 + double(ticksRemaining) / m_ticksPerMs;
    return milliseconds;
}

// -------------------------------------------------------------------------------------------------
#ifdef PROFILING_ENABLE_BROFILER
u32 Profiling::toBrofilerColor(const glm::fvec3 &color)
{
    return 0xFF000000
        | u8(glm::round(glm::clamp(color.r, 0.0f, 1.0f) * 255.0)) << 16
        | u8(glm::round(glm::clamp(color.g, 0.0f, 1.0f) * 255.0)) <<  8
        | u8(glm::round(glm::clamp(color.b, 0.0f, 1.0f) * 255.0));
}
#endif

// -------------------------------------------------------------------------------------------------
Profiling::Section::Section(const std::string &nameInit, const glm::fvec3 &colorInit) :
    name(nameInit), color(colorInit)
{
    m_profiling = Profiling::instance();
    u64 currentThreadId = u64(SDL_ThreadID());
    m_thread = &m_profiling->m_threads[currentThreadId];
    if (!m_thread->id)
    {
        m_thread->id = currentThreadId;
    }
    COMMON_ASSERT(m_thread->id == currentThreadId);
}

// -------------------------------------------------------------------------------------------------
void Profiling::Section::enter()
{
    u64 ticksEnter = m_profiling->ticksSinceFrameStart();

    ++m_thread->callDepth;
    m_frames.push_back(SectionSample());

    SectionSample &sample = m_frames.back();
    sample.section = this;
    sample.callDepth = m_thread->callDepth;
    sample.ticksEnter = ticksEnter;
}

// -------------------------------------------------------------------------------------------------
void Profiling::Section::exit()
{
    u64 ticksExit = m_profiling->ticksSinceFrameStart();

    SectionSample &sample = m_frames.back();
    sample.ticksExit = ticksExit;
    m_thread->samples.push_back(sample);

    COMMON_ASSERT(sample.callDepth == m_thread->callDepth);

    m_frames.pop_back();
    --m_thread->callDepth;
}

// -------------------------------------------------------------------------------------------------
Profiling::SectionGuard::SectionGuard(Section &section) : m_section(section)
{
    m_section.enter();
}

// -------------------------------------------------------------------------------------------------
Profiling::SectionGuard::~SectionGuard()
{
    m_section.exit();
}

// -------------------------------------------------------------------------------------------------
u64 Profiling::ticksSinceFrameStart() const
{
    u64 ticksPassed = SDL_GetPerformanceCounter() - m_ticksFrameStart;
    return ticksPassed;
}
