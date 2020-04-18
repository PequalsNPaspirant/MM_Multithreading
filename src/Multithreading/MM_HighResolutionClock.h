#pragma once

#ifdef _WIN32
#include <windows.h>
#include <chrono>
#include <iostream>

namespace mm
{
	class MM_HighResolutionClock
	{
	public:
		typedef long long                                       rep;
		typedef std::nano                                       period;
		typedef std::chrono::duration<rep, period>              duration;
		typedef std::chrono::time_point<MM_HighResolutionClock> time_point;
		static const bool is_steady = true;

		static time_point now()
		{
			static LARGE_INTEGER frequency = getQueryPerformanceFrequency();
			
			LARGE_INTEGER count;
			QueryPerformanceCounter(&count);
			return time_point(
				duration(count.QuadPart * static_cast<long long>(std::nano::den) / frequency.QuadPart)
				);
		}

	private:
		static LARGE_INTEGER getQueryPerformanceFrequency()
		{
			LARGE_INTEGER frequency;
			if (!QueryPerformanceFrequency(&frequency))
			{
				throw "QueryPerformanceFrequency failed";
			}

			return frequency;
		}
	};
}

#endif