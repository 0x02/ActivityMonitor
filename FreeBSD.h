#ifndef FREEBSD_H
#define FREEBSD_H

// for GetSysctl()
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/sysctl.h>

// for KVM
#include <unistd.h>
#include <paths.h>
#include <fcntl.h>
#include <kvm.h>
#include <sys/sysctl.h>
#include <sys/user.h>

#include <algorithm>
#include <string>
#include <vector>

namespace FreeBSD {

static inline void GetSysctl(const char* name, void* ptr, size_t len)
{
    size_t nlen = len;
    if (sysctlbyname(name, ptr, &nlen, nullptr, 0) == -1) {
        fprintf(stderr, "GetSysctl(): sysctl(%s...) failed: %s\n", name, strerror(errno));
        return;
    }    
    if (nlen != len) {
        fprintf(stderr, "GetSysctl(): sysctl(%s...) expected %lu, got %lu\n",
            name, (unsigned long)len, (unsigned long)nlen);
        return;
    }
}

template <typename V>
static inline void GetSysctl(const std::string& name, V& var)
{
    GetSysctl(name.c_str(), &var, sizeof(V));
}

struct PageInfo
{
    PageInfo()
    {
        int _size = getpagesize();
        size = _size;
        while (_size > 1) { 
            shift++;
            _size >>= 1;
        }   
    }

    int size = 0;
    int shift = 0;
};

struct VMStats
{
    void Update()
    {
        // pages
        GetSysctl("vfs.bufspace", buf);
        GetSysctl("vm.stats.vm.v_free_count", free);
        GetSysctl("vm.stats.vm.v_inactive_count", inactive);
        GetSysctl("vm.stats.vm.v_active_count", active);
        GetSysctl("vm.stats.vm.v_wire_count", wire);
        GetSysctl("vm.stats.vm.v_cache_count", cache);
        GetSysctl("vm.stats.vm.v_swappgsin", swapin);
        GetSysctl("vm.stats.vm.v_swappgsout", swapout);
    }

    long buf = 0;
    int active = 0;
    int inactive = 0;
    int wire = 0;
    int cache = 0;
    int free = 0;
    int swapin = 0;
    int swapout = 0 ;
};

struct ZFSStats
{
    bool HasArc() const
    {
        size_t size = sizeof(arc_size);
        return (sysctlbyname("kstat.zfs.misc.arcstats.size", 
                         (void*)&arc_size, &size,nullptr, 0) == 0 && arc_size != 0);
    }
    void Update()
    {
        // bytes
        GetSysctl("kstat.zfs.misc.arcstats.size", arc_size);
        GetSysctl("vfs.zfs.mfu_size", mfu);
        GetSysctl("vfs.zfs.mru_size", mru);
        GetSysctl("vfs.zfs.anon_size", anon);
        GetSysctl("kstat.zfs.misc.arcstats.hdr_size", arc_hdr);
        GetSysctl("kstat.zfs.misc.arcstats.l2_hdr_size", arc_l2hdr);
        GetSysctl("kstat.zfs.misc.arcstats.other_size", arc_other);
    }

    uint64_t arc_size = 0;
    uint64_t mfu = 0;
    uint64_t mru = 0;
    uint64_t anon = 0;
    uint64_t arc_hdr = 0;
    uint64_t arc_l2hdr = 0;
    uint64_t arc_other = 0;
};

class KVM
{
public:
    KVM() = default;
    KVM(const KVM&) = delete;
    KVM& operator=(const KVM&) = delete;

    ~KVM() {
        Close();
    }

    bool Open()
    {
        m_kd = kvm_open(nullptr, _PATH_DEVNULL, nullptr, O_RDONLY, "KVM::Open()");
        return (m_kd != nullptr);
    }

    void Close()
    {
        if (m_kd != nullptr) {
            m_swaps.clear();
            m_procs.clear();
            kvm_close(m_kd);
            m_kd = nullptr;
        }
    }

    // swap info { dev0, dev1, ..., total }
    bool HasSwap() const
    {
        int nswapdev = 0;
        GetSysctl("vm.nswapdev", nswapdev);
        return nswapdev > 0;
    }

    const std::vector<struct kvm_swap>& SwapInfo() const
    {
        return m_swaps;
    }
    
    void UpdateSwapInfo()
    {
        int nswapdev = 0;
        GetSysctl("vm.nswapdev", nswapdev);
        if (nswapdev == 0) {
            m_swaps.clear();
            return;
        }

        m_swaps.resize(nswapdev+1);
        int n = kvm_getswapinfo(m_kd, m_swaps.data(), nswapdev+1, 0);
        if (n < 0) {
            m_swaps.clear();
        }
    }

    // process filter
    std::vector<std::string> ProcessFilter() const {
        return m_procfilter;
    }

    void SetProcessFilter(const std::vector<std::string>& filter)
    {
        m_procfilter = filter;
    }

    void ClearProcessFilter()
    {
        m_procfilter.clear();
    }

    // process info
    const std::vector<const struct kinfo_proc*>& ProcessInfo() const
    {
        return m_procs;
    }

    void UpdateProcessInfo(bool all = false)
    {
        if (m_kd == nullptr) {
            return;
        }

        m_procs.clear();
        int nproc = 0;
        auto pbase = kvm_getprocs(m_kd, all ? KERN_PROC_ALL : KERN_PROC_PROC, 0, &nproc);
        if (pbase == nullptr) {
            return;
        }
        m_procs.reserve(nproc);
        for (int iproc = 0; iproc < nproc; ++iproc) {
            const auto info = pbase + iproc;
            if (!m_procfilter.empty()) {
                std::string name(info->ki_comm);
                bool found = false;
                for (const auto& filter: m_procfilter) {
                    if (name.find(filter) != std::string::npos) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    continue;
                }
            }
            m_procs.push_back(info);
        }

        typedef const struct kinfo_proc* procitem_t;

        auto cmp_pid = [](procitem_t a, procitem_t b) -> bool { 
            return a->ki_pid < b->ki_pid;
        };

        auto cmp_name = [](procitem_t a, procitem_t b) -> bool { 
            int ret = strcmp(a->ki_comm, b->ki_comm);
            if (ret != 0)
                return ret < 0;
            else
                return a->ki_pid < b->ki_pid;
        };

        std::sort(m_procs.begin(), m_procs.end(), cmp_name);
    }

private:
    kvm_t* m_kd = nullptr;
    std::vector<struct kvm_swap> m_swaps;
    std::vector<const struct kinfo_proc*> m_procs;

    std::vector<std::string> m_procfilter;
};

}

#endif
