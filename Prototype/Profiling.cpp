// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 14.09.2015
// -------------------------------------------------------------------------------------------------

#include "Profiling.hpp"

#include <SDL.h>

// -------------------------------------------------------------------------------------------------
Profiling::Profiling()
{
    m_ticksPerUs = double(SDL_GetPerformanceFrequency()) / (1000.0 * 1000.0);
    m_ticksAtStartup = SDL_GetPerformanceCounter();
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
Profiling::Section::Section(const std::string &name, const glm::fvec3 &color) :
    m_name(name), m_color(color)
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
    u64 ticksEnter = m_profiling->ticksSinceStartup();

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
    u64 ticksExit = m_profiling->ticksSinceStartup();

    SectionSample &sample = m_frames.back();
    sample.ticksExit = m_profiling->ticksSinceStartup();
    sample.timeInUs = m_profiling->ticksDiffToTimeInUs(sample.ticksEnter, sample.ticksExit);
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
u64 Profiling::ticksSinceStartup() const
{
    u64 ticksPassed = SDL_GetPerformanceCounter() - m_ticksAtStartup;
    return ticksPassed;
}

// -------------------------------------------------------------------------------------------------
u64 Profiling::ticksDiffToTimeInUs(u64 ticksA, u64 ticksB) const
{
    u64 ticksPassed = ticksB - ticksA;
    return u64(glm::round(double(ticksPassed) / m_ticksPerUs));
}
