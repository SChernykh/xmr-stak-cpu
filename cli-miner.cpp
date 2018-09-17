 /*
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  * Additional permission under GNU GPL version 3 section 7
  *
  * If you modify this Program, or any covered work, by linking or combining
  * it with OpenSSL (or a modified version of that library), containing parts
  * covered by the terms of OpenSSL License and SSLeay License, the licensors
  * of this Program grant you additional permission to convey the resulting work.
  *
  */

#include "executor.h"
#include "minethd.h"
#include "jconf.h"
#include "console.h"
#include "donate-level.h"
#ifndef CONF_NO_HWLOC
#   include "autoAdjustHwloc.hpp"
#else
#   include "autoAdjust.hpp"
#endif
#include "version.h"

#ifndef CONF_NO_HTTPD
#	include "httpd.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>

#ifndef CONF_NO_TLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

//Do a press any key for the windows folk. *insert any key joke here*
#ifdef _WIN32
void win_exit()
{
	printer::inst()->print_str("Press any key to exit.");
	get_key();
	return;
}

#define strcasecmp _stricmp

#else
void win_exit() { return; }
#endif // _WIN32

void do_benchmark();

int main(int argc, char *argv[])
{
	if(!jconf::inst()->parse_config("config.txt"))
	{
		win_exit();
		return 0;
	}

#ifdef PGO_BUILD
	if ((argc > 1) && (strcmp(argv[1], "/instrument") == 0))
	{
		return minethd::pgo_instrument();
	}
#endif

	minethd::self_test();
	do_benchmark();
#ifndef PERFORMANCE_TUNING
	win_exit();
#endif
	return 0;
}

#ifdef PERFORMANCE_TUNING
extern uint64_t min_cycles;
#endif

void do_benchmark()
{
	int benchmark_time = 60;

#ifdef PERFORMANCE_TUNING
	LARGE_INTEGER f, t1, t2;
	QueryPerformanceFrequency(&f);
	QueryPerformanceCounter(&t1);
	uint64_t tsc1 = __rdtsc();
	uint64_t tsc2;
	do
	{
		QueryPerformanceCounter(&t2);
		tsc2 = __rdtsc();
	} while (t2.QuadPart - t1.QuadPart < f.QuadPart);
	double rdtsc_speed = static_cast<double>(tsc2 - tsc1) / 1e9 / (t2.QuadPart - t1.QuadPart) * f.QuadPart;
	printer::inst()->print_msg(L0, "rdtsc speed: %.3f GHz", rdtsc_speed);
	benchmark_time = 10;
#endif

	using namespace std::chrono;
	std::vector<minethd*>* pvThreads;

	printer::inst()->print_msg(L0, "Running a %d second benchmark...", benchmark_time);

	uint8_t work[76] = {0};
	minethd::miner_work oWork = minethd::miner_work("", work, sizeof(work), 0, 0, false, 0);
	pvThreads = minethd::thread_starter(oWork);

	uint64_t iStartStamp = time_point_cast<milliseconds>(high_resolution_clock::now()).time_since_epoch().count();

	std::this_thread::sleep_for(std::chrono::seconds(benchmark_time));

	oWork = minethd::miner_work();
	minethd::switch_work(oWork);

	double fTotalHps = 0.0;
	for (uint32_t i = 0; i < pvThreads->size(); i++)
	{
		double fHps = pvThreads->at(i)->iHashCount;
		fHps /= (pvThreads->at(i)->iTimestamp - iStartStamp) / 1000.0;

		printer::inst()->print_msg(L0, "Thread %u: %.1f H/S", i, fHps);
		fTotalHps += fHps;
	}

	printer::inst()->print_msg(L0, "Total: %.1f H/S", fTotalHps);
#ifdef PERFORMANCE_TUNING
	printer::inst()->print_msg(L0, "%.2f ns per iteration", min_cycles / 524288.0 / rdtsc_speed);
#endif
}
