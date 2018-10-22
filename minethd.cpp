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

#include <assert.h>
#include <cmath>
#include <chrono>
#include <cstring>
#include <thread>
#include <bitset>
#include <fstream>
#include "console.h"

#ifdef _WIN32
#include <windows.h>

void thd_setaffinity(std::thread::native_handle_type h, uint64_t cpu_id)
{
	SetThreadAffinityMask(h, 1ULL << cpu_id);
}
#else
#include <pthread.h>

#if defined(__APPLE__)
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"
#elif defined(__FreeBSD__)
#include <pthread_np.h>
#endif


void thd_setaffinity(std::thread::native_handle_type h, uint64_t cpu_id)
{
#if defined(__APPLE__)
	thread_port_t mach_thread;
	thread_affinity_policy_data_t policy = { static_cast<integer_t>(cpu_id) };
	mach_thread = pthread_mach_thread_np(h);
	thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, 1);
#elif defined(__FreeBSD__)
	cpuset_t mn;
	CPU_ZERO(&mn);
	CPU_SET(cpu_id, &mn);
	pthread_setaffinity_np(h, sizeof(cpuset_t), &mn);
#else
	cpu_set_t mn;
	CPU_ZERO(&mn);
	CPU_SET(cpu_id, &mn);
	pthread_setaffinity_np(h, sizeof(cpu_set_t), &mn);
#endif
}
#endif // _WIN32

#include "minethd.h"
#include "jconf.h"
#include "crypto/cryptonight_aesni.h"
#include "hwlocMemory.hpp"

telemetry::telemetry(size_t iThd)
{
	ppHashCounts = new uint64_t*[iThd];
	ppTimestamps = new uint64_t*[iThd];
	iBucketTop = new uint32_t[iThd];

	for (size_t i = 0; i < iThd; i++)
	{
		ppHashCounts[i] = new uint64_t[iBucketSize];
		ppTimestamps[i] = new uint64_t[iBucketSize];
		iBucketTop[i] = 0;
		memset(ppHashCounts[0], 0, sizeof(uint64_t) * iBucketSize);
		memset(ppTimestamps[0], 0, sizeof(uint64_t) * iBucketSize);
	}
}

double telemetry::calc_telemetry_data(size_t iLastMilisec, size_t iThread)
{
	using namespace std::chrono;
	uint64_t iTimeNow = time_point_cast<milliseconds>(high_resolution_clock::now()).time_since_epoch().count();

	uint64_t iEarliestHashCnt = 0;
	uint64_t iEarliestStamp = 0;
	uint64_t iLastestStamp = 0;
	uint64_t iLastestHashCnt = 0;
	bool bHaveFullSet = false;

	//Start at 1, buckettop points to next empty
	for (size_t i = 1; i < iBucketSize; i++)
	{
		size_t idx = (iBucketTop[iThread] - i) & iBucketMask; //overflow expected here

		if (ppTimestamps[iThread][idx] == 0)
			break; //That means we don't have the data yet

		if (iLastestStamp == 0)
		{
			iLastestStamp = ppTimestamps[iThread][idx];
			iLastestHashCnt = ppHashCounts[iThread][idx];
		}

		if (iTimeNow - ppTimestamps[iThread][idx] > iLastMilisec)
		{
			bHaveFullSet = true;
			break; //We are out of the requested time period
		}

		iEarliestStamp = ppTimestamps[iThread][idx];
		iEarliestHashCnt = ppHashCounts[iThread][idx];
	}

	if (!bHaveFullSet || iEarliestStamp == 0 || iLastestStamp == 0)
		return nan("");

	//Don't think that can happen, but just in case
	if (iLastestStamp - iEarliestStamp == 0)
		return nan("");

	double fHashes, fTime;
	fHashes = iLastestHashCnt - iEarliestHashCnt;
	fTime = iLastestStamp - iEarliestStamp;
	fTime /= 1000.0;

	return fHashes / fTime;
}

void telemetry::push_perf_value(size_t iThd, uint64_t iHashCount, uint64_t iTimestamp)
{
	size_t iTop = iBucketTop[iThd];
	ppHashCounts[iThd][iTop] = iHashCount;
	ppTimestamps[iThd][iTop] = iTimestamp;

	iBucketTop[iThd] = (iTop + 1) & iBucketMask;
}

minethd::minethd(miner_work& pWork, size_t iNo, bool double_work, int variant, int asm_version, int64_t affinity)
{
	oWork = pWork;
	bQuit = 0;
	iThreadNo = (uint8_t)iNo;
	iJobNo = 0;
	iHashCount = 0;
	iTimestamp = 0;
	iVariant = variant;
	iAsmVersion = asm_version;
	this->affinity = affinity;
	thdHandle = 0;

	if(double_work)
		oWorkThd = std::thread(&minethd::double_work_main, this);
	else
		oWorkThd = std::thread(&minethd::work_main, this);

	thdHandle = oWorkThd.native_handle();
	if (affinity >= 0) //-1 means no affinity
		pin_thd_affinity();
}

std::atomic<uint64_t> minethd::iGlobalJobNo;
std::atomic<uint64_t> minethd::iConsumeCnt; //Threads get jobs as they are initialized
minethd::miner_work minethd::oGlobalWork;
uint64_t minethd::iThreadCount = 0;

cryptonight_ctx* minethd_alloc_ctx()
{
	cryptonight_ctx* ctx;
	alloc_msg msg = { 0 };

	switch (jconf::inst()->GetSlowMemSetting())
	{
	case jconf::never_use:
		ctx = cryptonight_alloc_ctx(1, 1, &msg);
		if (ctx == NULL)
			printer::inst()->print_msg(L0, "MEMORY ALLOC FAILED: %s", msg.warning);
		return ctx;

	case jconf::no_mlck:
		ctx = cryptonight_alloc_ctx(1, 0, &msg);
		if (ctx == NULL)
			printer::inst()->print_msg(L0, "MEMORY ALLOC FAILED: %s", msg.warning);
		return ctx;

	case jconf::print_warning:
		ctx = cryptonight_alloc_ctx(1, 1, &msg);
		if (msg.warning != NULL)
			printer::inst()->print_msg(L0, "MEMORY ALLOC FAILED: %s", msg.warning);
		if (ctx == NULL)
			ctx = cryptonight_alloc_ctx(0, 0, NULL);
		return ctx;

	case jconf::always_use:
		return cryptonight_alloc_ctx(0, 0, NULL);

	case jconf::unknown_value:
		return NULL; //Shut up compiler
	}

	return nullptr; //Should never happen
}

static void print_hash(const char* input, const char* hash)
{
	printf("HASH(\"%s\") = ", input);
	for (int k = 0; k < 32; ++k)
	{
		printf("%02x", static_cast<uint8_t>(hash[k]));
	}
	printf("\n");
}

bool minethd::self_test()
{
	for (int i = 0; i < 256; ++i)
	{
		const uint64_t index = (((i >> 3) & 6) | (i & 1)) << 1;
		variant1_table[i] = i ^ ((0x75310 >> index) & 0x30);
	}

	alloc_msg msg = { 0 };
	size_t res;
	bool fatal = false;

	switch (jconf::inst()->GetSlowMemSetting())
	{
	case jconf::never_use:
		res = cryptonight_init(1, 1, &msg);
		fatal = true;
		break;

	case jconf::no_mlck:
		res = cryptonight_init(1, 0, &msg);
		fatal = true;
		break;

	case jconf::print_warning:
		res = cryptonight_init(1, 1, &msg);
		break;

	case jconf::always_use:
		res = cryptonight_init(0, 0, &msg);
		break;

	case jconf::unknown_value:
	default:
		return false; //Shut up compiler
	}

	if(msg.warning != nullptr)
		printer::inst()->print_msg(L0, "MEMORY INIT ERROR: %s", msg.warning);

	if(res == 0 && fatal)
		return false;

	std::ifstream f("tests.txt");
	if (!f.is_open())
	{
		printer::inst()->print_msg(L0, "Cryptonight hash self-test failed: tests.txt not found.");
		return false;
	}

	cryptonight_ctx *ctx0, *ctx1;
	if((ctx0 = minethd_alloc_ctx()) == nullptr)
		return false;
	if ((ctx1 = minethd_alloc_ctx()) == nullptr)
		return false;

	std::string prev_input;
	std::string input;
	enum { HASH_SIZE = 32 };
	char reference_hash[3][HASH_SIZE];
	char reference_hash_dbl[3][HASH_SIZE * 2];
	while (!f.eof())
	{
		prev_input = input;
		std::getline(f, input);
		if (input.empty())
		{
			continue;
		}

		for (int i = 0; i < 3; ++i)
		{
			char hash[HASH_SIZE];
			char hash_dbl[HASH_SIZE * 2];
			cn_hash_fun hash_fun;
			cn_hash_fun_dbl hash_fun_dbl;
			hash_fun = func_selector(jconf::inst()->HaveHardwareAes(), i, 0);
			hash_fun_dbl = func_dbl_selector(jconf::inst()->HaveHardwareAes(), i, 0);

			std::string output;
			std::getline(f, output);
			if (output.length() != HASH_SIZE * 2)
			{
				printer::inst()->print_msg(L0, "Cryptonight hash self-test (variant %d) failed.", i);
				return false;
			}

			hash_fun(input.c_str(), input.length(), hash, ctx0);
			if (!prev_input.empty())
			{
				hash_fun_dbl(prev_input.c_str(), prev_input.length(), hash_dbl, input.c_str(), input.length(), hash_dbl + HASH_SIZE, ctx0, ctx1);
			}

			memcpy(reference_hash_dbl[i], reference_hash[i], HASH_SIZE);
			for (int j = 0; j < HASH_SIZE; ++j)
			{
				reference_hash[i][j] = static_cast<char>(std::stoul(output.substr(j * 2, 2), 0, 16));
			}
			memcpy(reference_hash_dbl[i] + HASH_SIZE, reference_hash[i], HASH_SIZE);

			if (memcmp(hash, reference_hash[i], HASH_SIZE) != 0)
			{
				print_hash(input.c_str(), hash);
				printer::inst()->print_msg(L0, "Cryptonight hash self-test (variant %d) failed.", i);
				return false;
			}

			if (!prev_input.empty())
			{
				if (memcmp(hash_dbl, reference_hash_dbl[i], HASH_SIZE * 2) != 0)
				{
					print_hash(prev_input.c_str(), hash_dbl);
					print_hash(input.c_str(), hash_dbl + HASH_SIZE);
					printer::inst()->print_msg(L0, "Cryptonight double hash self-test (variant %d) failed.", i);
					return false;
				}
			}

			if (i > 0)
			{
				for (int j = 1; j <= 4; ++j)
				{
					char hash[32];
					cn_hash_fun hash_fun = func_selector(jconf::inst()->HaveHardwareAes(), i, j);
					cn_hash_fun_dbl hash_fun_dbl = func_dbl_selector(jconf::inst()->HaveHardwareAes(), i, j);

					hash_fun(input.c_str(), input.length(), hash, ctx0);
					if (!prev_input.empty())
					{
						hash_fun_dbl(prev_input.c_str(), prev_input.length(), hash_dbl, input.c_str(), input.length(), hash_dbl + HASH_SIZE, ctx0, ctx1);
					}

					if (memcmp(hash, reference_hash[i], HASH_SIZE) != 0)
					{
						print_hash(input.c_str(), hash);
						printer::inst()->print_msg(L0, "Cryptonight hash self-test (variant %d, asm version %d) failed.", i, j);
						return false;
					}
					if (!prev_input.empty())
					{
						if (memcmp(hash_dbl, reference_hash_dbl[i], HASH_SIZE * 2) != 0)
						{
							print_hash(prev_input.c_str(), hash_dbl);
							print_hash(input.c_str(), hash_dbl + HASH_SIZE);
							printer::inst()->print_msg(L0, "Cryptonight double hash self-test (variant %d, asm version %d) failed.", i, j);
							return false;
						}
					}
				}
			}
		}
	}

	cryptonight_free_ctx(ctx0);

	printer::inst()->print_msg(L0, "Cryptonight hash self-test passed.");
	return true;
}

#ifdef PGO_BUILD
int minethd::pgo_instrument()
{
	printer::inst()->print_msg(L0, "Started instrumenting cryptonight_hash()");

	cryptonight_ctx *ctx0 = minethd_alloc_ctx();
	cryptonight_ctx *ctx1 = minethd_alloc_ctx();
	if (!ctx0 || !ctx1)
	{
		printer::inst()->print_msg(L0, "Failed to allocate memory");
		return 1;
	}

	char input[64] = {};
	cn_hash_fun hash_fun;
	cn_hash_fun_dbl hash_fun_dbl;
	char hash[64];
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j <= 1; ++j)
		{
			hash_fun = func_selector(j != 0, i, 0);
			hash_fun_dbl = func_dbl_selector(j != 0, i, 0);
			hash_fun(input, sizeof(input), hash, ctx0);
			hash_fun_dbl(input, sizeof(input), hash, input, sizeof(input), hash + 32, ctx0, ctx1);
		}
	}

	for (int variant = 0; variant <= 2; ++variant)
	{
		for (int i = 1; i <= 2; ++i)
		{
			hash_fun = func_selector(true, variant, i);
			hash_fun_dbl = func_dbl_selector(true, variant, i);
			hash_fun(input, sizeof(input), hash, ctx0);
			hash_fun_dbl(input, sizeof(input), hash, input, sizeof(input), hash + 32, ctx0, ctx1);
		}
	}

	cryptonight_free_ctx(ctx0);
	cryptonight_free_ctx(ctx1);

	printer::inst()->print_msg(L0, "Finished instrumenting cryptonight_hash()");
	return 0;
}
#endif

std::vector<minethd*>* minethd::thread_starter(miner_work& pWork)
{
	iGlobalJobNo = 0;
	iConsumeCnt = 0;
	std::vector<minethd*>* pvThreads = new std::vector<minethd*>;

	//Launch the requested number of single and double threads, to distribute
	//load evenly we need to alternate single and double threads
	size_t i, n = jconf::inst()->GetThreadCount();
	pvThreads->reserve(n);

	jconf::thd_cfg cfg;
	for (i = 0; i < n; i++)
	{
		jconf::inst()->GetThreadConfig(i, cfg);

		minethd* thd = new minethd(pWork, i, cfg.bDoubleMode, cfg.iVariant, cfg.iAsmVersion, cfg.iCpuAff);
		pvThreads->push_back(thd);

		if(cfg.iCpuAff >= 0)
			printer::inst()->print_msg(L1, "Starting %s thread, affinity: %d.", cfg.bDoubleMode ? "double" : "single", (int)cfg.iCpuAff);
		else
			printer::inst()->print_msg(L1, "Starting %s thread, no affinity.", cfg.bDoubleMode ? "double" : "single");
	}

	iThreadCount = n;
	return pvThreads;
}

void minethd::switch_work(miner_work& pWork)
{
	// iConsumeCnt is a basic lock-like polling mechanism just in case we happen to push work
	// faster than threads can consume them. This should never happen in real life.
	// Pool cant physically send jobs faster than every 250ms or so due to net latency.

	while (iConsumeCnt.load(std::memory_order_seq_cst) < iThreadCount)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	oGlobalWork = pWork;
	iConsumeCnt.store(0, std::memory_order_seq_cst);
	iGlobalJobNo++;
}

void minethd::consume_work()
{
	memcpy(&oWork, &oGlobalWork, sizeof(miner_work));
	iJobNo++;
	iConsumeCnt++;
}

extern "C"
{
	void cnv1_mainloop_sandybridge_asm(cryptonight_ctx* ctx0);
	void cnv2_mainloop_ivybridge_asm(cryptonight_ctx* ctx0);
	void cnv2_mainloop_ryzen_asm(cryptonight_ctx* ctx0);
	void cnv2_mainloop_bulldozer_asm(cryptonight_ctx* ctx0, const uint32_t* sqrt_lut);
	void cnv2_double_mainloop_sandybridge_asm(cryptonight_ctx* ctx0, cryptonight_ctx* ctx1);
	void cnv1_mainloop_soft_aes_sandybridge_asm(cryptonight_ctx* ctx0);
	void cnv2_mainloop_soft_aes_sandybridge_asm(cryptonight_ctx* ctx0);
}

#ifdef PERFORMANCE_TUNING
uint64_t t1, t2;
uint64_t min_cycles = uint64_t(-1);
#endif

ALIGN(64) uint8_t variant1_table[256];

void cryptonight_hash_v1_asm(const void* input, size_t len, void* output, cryptonight_ctx* ctx0)
{
	keccak((const uint8_t *)input, len, ctx0->hash_state, 200);
	cn_explode_scratchpad<MEMORY, false>((__m128i*)ctx0->hash_state, (__m128i*)ctx0->long_state);

#ifdef PERFORMANCE_TUNING
	t1 = __rdtsc();
#endif
	ctx0->input = input;
	ctx0->variant1_table = variant1_table;
	cnv1_mainloop_sandybridge_asm(ctx0);
#ifdef PERFORMANCE_TUNING
	t2 = __rdtsc();
#endif

	cn_implode_scratchpad<MEMORY, false>((__m128i*)ctx0->long_state, (__m128i*)ctx0->hash_state);
	keccakf((uint64_t*)ctx0->hash_state, 24);
	extra_hashes[ctx0->hash_state[0] & 3](ctx0->hash_state, 200, (char*)output);
}

void cryptonight_hash_v1_soft_aes_asm(const void* input, size_t len, void* output, cryptonight_ctx* ctx0)
{
	keccak((const uint8_t *)input, len, ctx0->hash_state, 200);
	cn_explode_scratchpad<MEMORY, true>((__m128i*)ctx0->hash_state, (__m128i*)ctx0->long_state);

#ifdef PERFORMANCE_TUNING
	t1 = __rdtsc();
#endif
	ctx0->input = input;
	ctx0->variant1_table = variant1_table;
	ctx0->t_fn = (const uint32_t*)t_fn;
	cnv1_mainloop_soft_aes_sandybridge_asm(ctx0);
#ifdef PERFORMANCE_TUNING
	t2 = __rdtsc();
#endif

	cn_implode_scratchpad<MEMORY, true>((__m128i*)ctx0->long_state, (__m128i*)ctx0->hash_state);
	keccakf((uint64_t*)ctx0->hash_state, 24);
	extra_hashes[ctx0->hash_state[0] & 3](ctx0->hash_state, 200, (char*)output);
}

template<int asm_version>
void cryptonight_hash_v2_asm(const void* input, size_t len, void* output, cryptonight_ctx* ctx0)
{
	keccak((const uint8_t *)input, len, ctx0->hash_state, 200);
	cn_explode_scratchpad<MEMORY, false>((__m128i*)ctx0->hash_state, (__m128i*)ctx0->long_state);

#ifdef PERFORMANCE_TUNING
	t1 = __rdtsc();
#endif
	if (asm_version == 1)
		cnv2_mainloop_ivybridge_asm(ctx0);
	else if (asm_version == 2)
		cnv2_mainloop_ryzen_asm(ctx0);
	else
		cnv2_mainloop_bulldozer_asm(ctx0, SqrtV2Table);
#ifdef PERFORMANCE_TUNING
	t2 = __rdtsc();
#endif

	cn_implode_scratchpad<MEMORY, false>((__m128i*)ctx0->long_state, (__m128i*)ctx0->hash_state);
	keccakf((uint64_t*)ctx0->hash_state, 24);
	extra_hashes[ctx0->hash_state[0] & 3](ctx0->hash_state, 200, (char*)output);
}

void cryptonight_hash_v2_soft_aes_asm(const void* input, size_t len, void* output, cryptonight_ctx* ctx0)
{
	keccak((const uint8_t *)input, len, ctx0->hash_state, 200);
	cn_explode_scratchpad<MEMORY, true>((__m128i*)ctx0->hash_state, (__m128i*)ctx0->long_state);

#ifdef PERFORMANCE_TUNING
	t1 = __rdtsc();
#endif
	ctx0->input = input;
	ctx0->t_fn = (const uint32_t*)t_fn;
	cnv2_mainloop_soft_aes_sandybridge_asm(ctx0);
#ifdef PERFORMANCE_TUNING
	t2 = __rdtsc();
#endif

	cn_implode_scratchpad<MEMORY, true>((__m128i*)ctx0->long_state, (__m128i*)ctx0->hash_state);
	keccakf((uint64_t*)ctx0->hash_state, 24);
	extra_hashes[ctx0->hash_state[0] & 3](ctx0->hash_state, 200, (char*)output);
}

void cryptonight_double_hash_v2_asm(const void* input1, size_t len1, void* output1, const void* input2, size_t len2, void* output2, cryptonight_ctx* __restrict ctx0, cryptonight_ctx* __restrict ctx1)
{
	keccak((const uint8_t *)input1, len1, ctx0->hash_state, 200);
	keccak((const uint8_t *)input2, len2, ctx1->hash_state, 200);

	// Optim - 99% time boundary
	cn_explode_scratchpad<MEMORY, false>((__m128i*)ctx0->hash_state, (__m128i*)ctx0->long_state);
	cn_explode_scratchpad<MEMORY, false>((__m128i*)ctx1->hash_state, (__m128i*)ctx1->long_state);

#ifdef PERFORMANCE_TUNING
	t1 = __rdtsc();
#endif
	cnv2_double_mainloop_sandybridge_asm(ctx0, ctx1);
#ifdef PERFORMANCE_TUNING
	t2 = __rdtsc();
#endif

	// Optim - 90% time boundary
	cn_implode_scratchpad<MEMORY, false>((__m128i*)ctx0->long_state, (__m128i*)ctx0->hash_state);
	cn_implode_scratchpad<MEMORY, false>((__m128i*)ctx1->long_state, (__m128i*)ctx1->hash_state);

	// Optim - 99% time boundary

	keccakf((uint64_t*)ctx0->hash_state, 24);
	extra_hashes[ctx0->hash_state[0] & 3](ctx0->hash_state, 200, (char*)output1);
	keccakf((uint64_t*)ctx1->hash_state, 24);
	extra_hashes[ctx1->hash_state[0] & 3](ctx1->hash_state, 200, (char*)output2);
}

minethd::cn_hash_fun minethd::func_selector(bool bHaveAes, int variant, int asm_version)
{
	// We have two independent flag bits in the functions
	// therefore we will build a binary digit and select the
	// function as a two digit binary
	// Digit order SOFT_AES, NO_PREFETCH, SHUFFLE, INT_MATH

	if (asm_version > 0)
	{
		if (!bHaveAes)
		{
			if (variant == 1)
				return cryptonight_hash_v1_soft_aes_asm;
			if (variant == 2)
				return cryptonight_hash_v2_soft_aes_asm;
		}
		else
		{
			if (variant == 1)
			{
				return cryptonight_hash_v1_asm;
			}
			else if (variant == 2)
			{
				// Intel Ivy Bridge (Xeon v2, Core i7/i5/i3 3xxx, Pentium G2xxx, Celeron G1xxx)
				if (asm_version == 1)
					return cryptonight_hash_v2_asm<1>;

				// AMD Ryzen (1xxx and 2xxx series)
				if (asm_version == 2)
					return cryptonight_hash_v2_asm<2>;

				// AMD Bulldozer
				if (asm_version == 3)
					return cryptonight_hash_v2_asm<3>;
			}
		}
	}

	static const cn_hash_fun func_table[8] = {
		cryptonight_hash<0x80000, MEMORY, false, 0>,
		cryptonight_hash<0x80000, MEMORY, false, 1>,
		cryptonight_hash<0x80000, MEMORY, false, 2>,
		cryptonight_hash<0x80000, MEMORY, false, 3>,

		cryptonight_hash<0x80000, MEMORY, true, 0>,
		cryptonight_hash<0x80000, MEMORY, true, 1>,
		cryptonight_hash<0x80000, MEMORY, true, 2>,
		cryptonight_hash<0x80000, MEMORY, true, 3>,
	};

	return func_table[variant + (bHaveAes ? 0 : 4)];
}

void minethd::pin_thd_affinity()
{
	// pin memory to NUMA node
	bindMemoryToNUMANode(affinity);

#if defined(__APPLE__)
	printer::inst()->print_msg(L1, "WARNING on MacOS thread affinity is only advisory.");
#endif
	while (thdHandle.load() == 0) {}
	thd_setaffinity(thdHandle.load(), affinity);
}

void minethd::work_main()
{
	if(affinity >= 0) //-1 means no affinity
		pin_thd_affinity();

	cn_hash_fun hash_fun;
	cryptonight_ctx* ctx;
	uint64_t iCount = 0;
	uint32_t* piNonce;
	uint8_t bHashOut[32];

	hash_fun = func_selector(jconf::inst()->HaveHardwareAes(), iVariant, iAsmVersion);
	ctx = minethd_alloc_ctx();

	piNonce = (uint32_t*)(oWork.bWorkBlob + 39);
	iConsumeCnt++;

	while (bQuit == 0)
	{
		if (oWork.bStall)
		{
			/*  We are stalled here because the executor didn't find a job for us yet,
			    either because of network latency, or a socket problem. Since we are
			    raison d'etre of this software it us sensible to just wait until we have something*/

			while (iGlobalJobNo.load(std::memory_order_relaxed) == iJobNo)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

			consume_work();
			continue;
		}

		while(iGlobalJobNo.load(std::memory_order_relaxed) == iJobNo)
		{
			if ((iCount & 0xF) == 0) //Store stats every 16 hashes
			{
				using namespace std::chrono;
				uint64_t iStamp = time_point_cast<milliseconds>(high_resolution_clock::now()).time_since_epoch().count();
				iHashCount.store(iCount, std::memory_order_relaxed);
				iTimestamp.store(iStamp, std::memory_order_relaxed);
			}
			iCount++;

			hash_fun(oWork.bWorkBlob, oWork.iWorkSize, bHashOut, ctx);
#ifdef PERFORMANCE_TUNING
			if (t2 - t1 < min_cycles)
			{
				min_cycles = t2 - t1;
			}
#endif
		}

		consume_work();
	}

	cryptonight_free_ctx(ctx);
}

minethd::cn_hash_fun_dbl minethd::func_dbl_selector(bool bHaveAes, int variant, int asm_version)
{
	// We have two independent flag bits in the functions
	// therefore we will build a binary digit and select the
	// function as a two digit binary
	// Digit order SOFT_AES, NO_PREFETCH, SHUFFLE, INT_MATH

	if (bHaveAes && (variant == 2) && (asm_version > 0))
	{
		return cryptonight_double_hash_v2_asm;
	}

	static const cn_hash_fun_dbl func_table[8] = {
		cryptonight_double_hash<0x80000, MEMORY, false, 0>,
		cryptonight_double_hash<0x80000, MEMORY, false, 1>,
		cryptonight_double_hash<0x80000, MEMORY, false, 2>,
		cryptonight_double_hash<0x80000, MEMORY, false, 3>,

		cryptonight_double_hash<0x80000, MEMORY, true, 0>,
		cryptonight_double_hash<0x80000, MEMORY, true, 1>,
		cryptonight_double_hash<0x80000, MEMORY, true, 2>,
		cryptonight_double_hash<0x80000, MEMORY, true, 3>,
	};

	return func_table[variant + (bHaveAes ? 0 : 4)];
}

void minethd::double_work_main()
{
	if(affinity >= 0) //-1 means no affinity
		pin_thd_affinity();

	cn_hash_fun_dbl hash_fun;
	cryptonight_ctx* ctx0;
	cryptonight_ctx* ctx1;
	uint64_t iCount = 0;
	uint64_t *piHashVal0, *piHashVal1;
	uint32_t *piNonce0, *piNonce1;
	uint8_t bDoubleHashOut[64];
	uint8_t	bDoubleWorkBlob[sizeof(miner_work::bWorkBlob) * 2];
	uint32_t iNonce;

	hash_fun = func_dbl_selector(jconf::inst()->HaveHardwareAes(), iVariant, iAsmVersion);
	ctx0 = minethd_alloc_ctx();
	ctx1 = minethd_alloc_ctx();

	piHashVal0 = (uint64_t*)(bDoubleHashOut + 24);
	piHashVal1 = (uint64_t*)(bDoubleHashOut + 32 + 24);
	piNonce0 = (uint32_t*)(bDoubleWorkBlob + 39);
	piNonce1 = (uint32_t*)(bDoubleWorkBlob + oWork.iWorkSize + 39);

	iConsumeCnt++;

	while (bQuit == 0)
	{
		if (oWork.bStall)
		{
			/*	We are stalled here because the executor didn't find a job for us yet,
			either because of network latency, or a socket problem. Since we are
			raison d'etre of this software it us sensible to just wait until we have something*/

			while (iGlobalJobNo.load(std::memory_order_relaxed) == iJobNo)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

			consume_work();
			memcpy(bDoubleWorkBlob, oWork.bWorkBlob, oWork.iWorkSize);
			memcpy(bDoubleWorkBlob + oWork.iWorkSize, oWork.bWorkBlob, oWork.iWorkSize);
			piNonce1 = (uint32_t*)(bDoubleWorkBlob + oWork.iWorkSize + 39);
			continue;
		}

		if(oWork.bNiceHash)
			iNonce = calc_nicehash_nonce(*piNonce0, oWork.iResumeCnt);
		else
			iNonce = calc_start_nonce(oWork.iResumeCnt);

		while (iGlobalJobNo.load(std::memory_order_relaxed) == iJobNo)
		{
			if ((iCount & 0x7) == 0) //Store stats every 16 hashes
			{
				using namespace std::chrono;
				uint64_t iStamp = time_point_cast<milliseconds>(high_resolution_clock::now()).time_since_epoch().count();
				iHashCount.store(iCount, std::memory_order_relaxed);
				iTimestamp.store(iStamp, std::memory_order_relaxed);
			}

			iCount += 2;

			*piNonce0 = ++iNonce;
			*piNonce1 = ++iNonce;

			hash_fun(bDoubleWorkBlob, oWork.iWorkSize, bDoubleHashOut, bDoubleWorkBlob + oWork.iWorkSize, oWork.iWorkSize, bDoubleHashOut + 32, ctx0, ctx1);
#ifdef PERFORMANCE_TUNING
			if (t2 - t1 < min_cycles)
			{
				min_cycles = t2 - t1;
			}
#endif
		}

		consume_work();
		memcpy(bDoubleWorkBlob, oWork.bWorkBlob, oWork.iWorkSize);
		memcpy(bDoubleWorkBlob + oWork.iWorkSize, oWork.bWorkBlob, oWork.iWorkSize);
		piNonce1 = (uint32_t*)(bDoubleWorkBlob + oWork.iWorkSize + 39);
	}

	cryptonight_free_ctx(ctx0);
	cryptonight_free_ctx(ctx1);
}
