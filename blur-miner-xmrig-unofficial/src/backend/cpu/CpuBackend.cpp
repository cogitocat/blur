/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <mutex>


#include "backend/common/Hashrate.h"
#include "backend/common/interfaces/IWorker.h"
#include "backend/common/Tags.h"
#include "backend/common/Workers.h"
#include "backend/cpu/Cpu.h"
#include "backend/cpu/CpuBackend.h"
#include "base/io/log/Log.h"
#include "base/net/stratum/Job.h"
#include "base/tools/Chrono.h"
#include "base/tools/String.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "crypto/common/VirtualMemory.h"
#include "rapidjson/document.h"


#ifdef XMRIG_FEATURE_API
#   include "base/api/interfaces/IApiRequest.h"
#endif


namespace xmrig {


extern template class Threads<CpuThreads>;


static const char *tag      = CYAN_BG_BOLD(WHITE_BOLD_S " cpu ");
static const String kType   = "cpu";
static std::mutex mutex;


struct CpuLaunchStatus
{
public:
    inline size_t hugePages() const     { return m_hugePages; }
    inline size_t memory() const        { return m_ways * m_memory; }
    inline size_t pages() const         { return m_pages; }
    inline size_t threads() const       { return m_threads; }
    inline size_t ways() const          { return m_ways; }

    inline void start(const std::vector<CpuLaunchData> &threads, size_t memory)
    {
        m_hugePages = 0;
        m_memory    = memory;
        m_pages     = 0;
        m_started   = 0;
        m_errors    = 0;
        m_threads   = threads.size();
        m_ways      = 0;
        m_ts        = Chrono::steadyMSecs();
    }

    inline bool started(IWorker *worker, bool ready)
    {
        if (ready) {
            auto hugePages = worker->memory()->hugePages();

            m_started++;
            m_hugePages += hugePages.first;
            m_pages     += hugePages.second;
            m_ways      += worker->intensity();
        }
        else {
            m_errors++;
        }

        return (m_started + m_errors) == m_threads;
    }

    inline void print() const
    {
        if (m_started == 0) {
            LOG_ERR("%s " RED_BOLD("disabled") YELLOW(" (failed to start threads)"), tag);

            return;
        }

        LOG_INFO("%s" GREEN_BOLD(" READY") " threads %s%zu/%zu (%zu)" CLEAR " huge pages %s%1.0f%% %zu/%zu" CLEAR " memory " CYAN_BOLD("%zu KB") BLACK_BOLD(" (%" PRIu64 " ms)"),
                 tag,
                 m_errors == 0 ? CYAN_BOLD_S : YELLOW_BOLD_S,
                 m_started, m_threads, m_ways,
                 (m_hugePages == m_pages ? GREEN_BOLD_S : (m_hugePages == 0 ? RED_BOLD_S : YELLOW_BOLD_S)),
                 m_hugePages == 0 ? 0.0 : static_cast<double>(m_hugePages) / m_pages * 100.0,
                 m_hugePages, m_pages,
                 memory() / 1024,
                 Chrono::steadyMSecs() - m_ts
                 );
    }

private:
    size_t m_errors       = 0;
    size_t m_hugePages    = 0;
    size_t m_memory       = 0;
    size_t m_pages        = 0;
    size_t m_started      = 0;
    size_t m_threads      = 0;
    size_t m_ways         = 0;
    uint64_t m_ts         = 0;
};


class CpuBackendPrivate
{
public:
    inline CpuBackendPrivate(Controller *controller) :
        controller(controller)
    {
    }


    inline void start()
    {
        LOG_INFO("%s use profile " BLUE_BG(WHITE_BOLD_S " cn/blur ") WHITE_BOLD_S " (" CYAN_BOLD("%zu") WHITE_BOLD(" thread%s)") " scratchpad " CYAN_BOLD("%zu KB"),
                 tag,
                 threads.size(),
                 threads.size() > 1 ? "s" : "",
                 CnAlgo::CN_MEMORY / 1024
                 );

        status.start(threads, CnAlgo::CN_MEMORY);
        workers.start(threads);
    }


    size_t ways()
    {
        std::lock_guard<std::mutex> lock(mutex);

        return status.ways();
    }


    rapidjson::Value hugePages(int version, rapidjson::Document &doc)
    {
        std::pair<unsigned, unsigned> pages(0, 0);

        mutex.lock();

        pages.first  += status.hugePages();
        pages.second += status.pages();

        mutex.unlock();

        rapidjson::Value hugepages;

        if (version > 1) {
            hugepages.SetArray();
            hugepages.PushBack(pages.first, doc.GetAllocator());
            hugepages.PushBack(pages.second, doc.GetAllocator());
        }
        else {
            hugepages = pages.first == pages.second;
        }

        return hugepages;
    }

    Controller *controller;
    CpuLaunchStatus status;
    std::vector<CpuLaunchData> threads;
    Workers<CpuLaunchData> workers;
};


} // namespace xmrig


const char *xmrig::backend_tag(uint32_t backend)
{
#   ifdef XMRIG_FEATURE_OPENCL
    if (backend == Nonce::OPENCL) {
        return ocl_tag();
    }
#   endif

#   ifdef XMRIG_FEATURE_CUDA
    if (backend == Nonce::CUDA) {
        return cuda_tag();
    }
#   endif

    return tag;
}


const char *xmrig::cpu_tag()
{
    return tag;
}


xmrig::CpuBackend::CpuBackend(Controller *controller) :
    d_ptr(new CpuBackendPrivate(controller))
{
    d_ptr->workers.setBackend(this);
}


xmrig::CpuBackend::~CpuBackend()
{
    delete d_ptr;
}


bool xmrig::CpuBackend::isEnabled() const
{
    return d_ptr->controller->config()->cpu().isEnabled();
}


bool xmrig::CpuBackend::isEnabled(const Algorithm &algorithm) const
{
    return !d_ptr->controller->config()->cpu().threads().get().isEmpty();
}


const xmrig::Hashrate *xmrig::CpuBackend::hashrate() const
{
    return d_ptr->workers.hashrate();
}


const xmrig::String &xmrig::CpuBackend::type() const
{
    return kType;
}


void xmrig::CpuBackend::prepare(const Job &nextJob)
{

}


void xmrig::CpuBackend::printHashrate(bool details)
{
    if (!details || !hashrate()) {
        return;
    }

    char num[8 * 3] = { 0 };

    Log::print(WHITE_BOLD_S "|    CPU # | AFFINITY | 10s H/s | 60s H/s | 15m H/s |");

    size_t i = 0;
    for (const CpuLaunchData &data : d_ptr->threads) {
         Log::print("| %8zu | %8" PRId64 " | %7s | %7s | %7s |",
                    i,
                    data.affinity,
                    Hashrate::format(hashrate()->calc(i, Hashrate::ShortInterval),  num,         sizeof num / 3),
                    Hashrate::format(hashrate()->calc(i, Hashrate::MediumInterval), num + 8,     sizeof num / 3),
                    Hashrate::format(hashrate()->calc(i, Hashrate::LargeInterval),  num + 8 * 2, sizeof num / 3)
                    );

         i++;
    }

#   ifdef XMRIG_FEATURE_OPENCL
    Log::print(WHITE_BOLD_S "|        - |        - | %7s | %7s | %7s |",
               Hashrate::format(hashrate()->calc(Hashrate::ShortInterval),  num,         sizeof num / 3),
               Hashrate::format(hashrate()->calc(Hashrate::MediumInterval), num + 8,     sizeof num / 3),
               Hashrate::format(hashrate()->calc(Hashrate::LargeInterval),  num + 8 * 2, sizeof num / 3)
               );
#   endif
}


void xmrig::CpuBackend::setJob(const Job &job)
{
    if (!isEnabled()) {
        return stop();
    }

    const CpuConfig &cpu = d_ptr->controller->config()->cpu();

    std::vector<CpuLaunchData> threads = cpu.get(d_ptr->controller->miner());
    if (!d_ptr->threads.empty() && d_ptr->threads.size() == threads.size() && std::equal(d_ptr->threads.begin(), d_ptr->threads.end(), threads.begin())) {
        return;
    }

    if (threads.empty()) {
        LOG_WARN("%s " RED_BOLD("disabled") YELLOW(" (no suitable configuration found)"), tag);

        return stop();
    }

    stop();

    d_ptr->threads = std::move(threads);
    d_ptr->start();
}


void xmrig::CpuBackend::start(IWorker *worker, bool ready)
{
    mutex.lock();

    if (d_ptr->status.started(worker, ready)) {
        d_ptr->status.print();
    }

    mutex.unlock();

    if (ready) {
        worker->start();
    }
}


void xmrig::CpuBackend::stop()
{
    if (d_ptr->threads.empty()) {
        return;
    }

    const uint64_t ts = Chrono::steadyMSecs();

    d_ptr->workers.stop();
    d_ptr->threads.clear();

    LOG_INFO("%s" YELLOW(" stopped") BLACK_BOLD(" (%" PRIu64 " ms)"), tag, Chrono::steadyMSecs() - ts);
}


void xmrig::CpuBackend::tick(uint64_t ticks)
{
    d_ptr->workers.tick(ticks);
}


#ifdef XMRIG_FEATURE_API
rapidjson::Value xmrig::CpuBackend::toJSON(rapidjson::Document &doc) const
{
    using namespace rapidjson;
    auto &allocator         = doc.GetAllocator();
    const CpuConfig &cpu    = d_ptr->controller->config()->cpu();

    Value out(kObjectType);
    out.AddMember("type",       type().toJSON(), allocator);
    out.AddMember("enabled",    isEnabled(), allocator);
    out.AddMember("hw-aes",     cpu.isHwAES(), allocator);
    out.AddMember("priority",   cpu.priority(), allocator);

    out.AddMember("hugepages", d_ptr->hugePages(2, doc), allocator);
    out.AddMember("memory",    static_cast<uint64_t>(d_ptr->ways() * CnAlgo::CN_MEMORY), allocator);

    if (d_ptr->threads.empty() || !hashrate()) {
        return out;
    }

    out.AddMember("hashrate", hashrate()->toJSON(doc), allocator);

    Value threads(kArrayType);

    size_t i = 0;
    for (const CpuLaunchData &data : d_ptr->threads) {
        Value thread(kObjectType);
        thread.AddMember("intensity",   data.intensity, allocator);
        thread.AddMember("affinity",    data.affinity, allocator);
        thread.AddMember("av",          data.av(), allocator);
        thread.AddMember("hashrate",    hashrate()->toJSON(i, doc), allocator);

        i++;
        threads.PushBack(thread, allocator);
    }

    out.AddMember("threads", threads, allocator);

    return out;
}


void xmrig::CpuBackend::handleRequest(IApiRequest &request)
{
    if (request.type() == IApiRequest::REQ_SUMMARY) {
        request.reply().AddMember("hugepages", d_ptr->hugePages(request.version(), request.doc()), request.doc().GetAllocator());
    }
}
#endif
