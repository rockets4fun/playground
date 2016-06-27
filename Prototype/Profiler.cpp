// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 14.09.2015
// -------------------------------------------------------------------------------------------------

#include "Profiler.hpp"

#include <SDL.h>

// -------------------------------------------------------------------------------------------------
Profiler::Profiler()
{
    m_ticksPerS = SDL_GetPerformanceFrequency();
    m_ticksPerMs = double(m_ticksPerS) / 1000.0;

#ifdef PROFILER_ENABLE_REMOTERY
    rmt_CreateGlobalInstance(&m_remotery);
#endif
}

// -------------------------------------------------------------------------------------------------
Profiler::~Profiler()
{
#ifdef PROFILER_ENABLE_REMOTERY
    rmt_DestroyGlobalInstance(m_remotery);
#endif
}

// -------------------------------------------------------------------------------------------------
Profiler *Profiler::instance()
{
    static Profiler profiling;
    return &profiling;
}

// -------------------------------------------------------------------------------------------------
Profiler::Thread *Profiler::mainThreadPrevFrame()
{
    if (m_threadsPrevFrame.empty())
    {
        return nullptr;
    }
    return &m_threadsPrevFrame.begin()->second;
}

// -------------------------------------------------------------------------------------------------
void Profiler::frameReset()
{
    /*
    // FIXME(martinmo): Swap to make sure raw pointers into samples vectors still valid
    m_threadsPrevFrame.swap(m_threads);
    m_threads = m_threadsPrevFrame;
    */

    m_threadsPrevFrame = m_threads;
    for (auto &threadMapIter : m_threads)
    {
        COMMON_ASSERT(threadMapIter.second.callDepth == 0);
        threadMapIter.second.samples.clear();
        threadMapIter.second.samplesStack.clear();
    }
    m_ticksFrameStart = SDL_GetPerformanceCounter();
}

// -------------------------------------------------------------------------------------------------
double Profiler::ticksToMs(u64 ticks) const
{
    u64 seconds = ticks / m_ticksPerS;
    u64 ticksRemaining = ticks % m_ticksPerS;
    double milliseconds = double(seconds) * 1000.0 + double(ticksRemaining) / m_ticksPerMs;
    return milliseconds;
}

// -------------------------------------------------------------------------------------------------
#ifdef PROFILER_ENABLE_BROFILER
u32 Profiler::toBrofilerColor(const glm::fvec3 &color)
{
    return 0xFF000000
        | u8(glm::round(glm::clamp(color.r, 0.0f, 1.0f) * 255.0)) << 16
        | u8(glm::round(glm::clamp(color.g, 0.0f, 1.0f) * 255.0)) <<  8
        | u8(glm::round(glm::clamp(color.b, 0.0f, 1.0f) * 255.0));
}
#endif

// -------------------------------------------------------------------------------------------------
Profiler::Section::Section(const std::string &nameInit, const glm::fvec3 &colorInit) :
    name(nameInit), color(colorInit)
{
    m_profiling = Profiler::instance();
    u64 currentThreadId = u64(SDL_ThreadID());
    m_thread = &m_profiling->m_threads[currentThreadId];
    if (!m_thread->id)
    {
        COMMON_ASSERT(currentThreadId);
        m_thread->id = currentThreadId;
        m_thread->samples.reserve(Thread::MAX_SAMPLE_COUNT);
    }
    COMMON_ASSERT(m_thread->id == currentThreadId);
}

// -------------------------------------------------------------------------------------------------
void Profiler::Section::enter()
{
    u64 ticksEnter = m_profiling->ticksSinceFrameStart();

    // We have to make sure that we are not re-allocating 'Thread::samples' because we
    // are storing pointers into that vector in 'Thread::samplesStack' and 'SectionSample::parent'
    COMMON_ASSERT(m_thread->samples.size() < Thread::MAX_SAMPLE_COUNT);
    m_thread->samples.push_back(SectionSample());
    SectionSample *sample = &m_thread->samples.back();

    sample->section = this;
    sample->callDepth = m_thread->callDepth;
    sample->ticksEnter = ticksEnter;
    /*
    if (!m_thread->samplesStack.empty())
    {
        sample->parent = m_thread->samplesStack.back();
    }
    */

    ++m_thread->callDepth;
    m_thread->samplesStack.push_back(sample);
}

// -------------------------------------------------------------------------------------------------
void Profiler::Section::exit()
{
    SectionSample *sample = m_thread->samplesStack.back();

    m_thread->samplesStack.pop_back();
    --m_thread->callDepth;

    COMMON_ASSERT(m_thread->callDepth == sample->callDepth);

    sample->ticksExit = m_profiling->ticksSinceFrameStart();
}

// -------------------------------------------------------------------------------------------------
Profiler::SectionGuard::SectionGuard(Section &section) : m_section(section)
{
    m_section.enter();
}

// -------------------------------------------------------------------------------------------------
Profiler::SectionGuard::~SectionGuard()
{
    m_section.exit();
}

// -------------------------------------------------------------------------------------------------
u64 Profiler::ticksSinceFrameStart() const
{
    u64 ticksPassed = SDL_GetPerformanceCounter() - m_ticksFrameStart;
    return ticksPassed;
}
