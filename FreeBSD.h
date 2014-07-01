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
    if (sysctlbyname(name, ptr, &nlen, NULL, 0) == -1) {
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
                         (void*)&arc_size, &size,NULL, 0) == 0 && arc_size != 0);
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
    KVM():
        m_kd(NULL)
    {
    }

    bool Open()
    {
        m_kd = kvm_open(NULL, _PATH_DEVNULL, NULL, O_RDONLY, "KVM::Open()");
        return (m_kd != NULL);
    }

    void Close()
    {
        if (m_kd != NULL) {
            kvm_close(m_kd);
            m_kd = NULL;
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

    // process info
    const std::vector<struct kinfo_proc>& ProcessInfo() const
    {
        return m_procs;
    }

    void SetProcessFilter(const std::vector<std::string>& filter)
    {
        m_procfilter = filter;
    }

    void ClearProcessFilter()
    {
        m_procfilter.clear();
    }

    void UpdateProcessInfo(bool all = false)
    {
        int nproc = 0;
        struct kinfo_proc* pbase = 
            kvm_getprocs(m_kd, all ? KERN_PROC_ALL : KERN_PROC_PROC, 0, &nproc);

        m_procs.clear();
        m_procs.reserve(nproc);
        for (int i = 0; i < nproc; ++i) {
            const struct kinfo_proc& info = pbase[i];
            if (!m_procfilter.empty()) {
                std::string name(info.ki_comm);
                bool found = false;
                for (size_t i = 0; i < m_procfilter.size(); ++i) {
                    if (name.find(m_procfilter[i]) != std::string::npos) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    continue;
            }
            m_procs.push_back(info);
        }

        auto cmp_pid = [](const struct kinfo_proc& a, const struct kinfo_proc& b) -> bool { 
            return a.ki_pid < b.ki_pid;
        };

        auto cmp_name = [](const struct kinfo_proc& a, const struct kinfo_proc& b) -> bool { 
            int ret = strcmp(a.ki_comm, b.ki_comm);
            if (ret != 0)
                return ret < 0;
            else
                return a.ki_pid < b.ki_pid;
        };

        std::sort(m_procs.begin(), m_procs.end(), cmp_name);
    }

private:
    kvm_t* m_kd = nullptr;

    std::vector<struct kvm_swap> m_swaps;

    std::vector<std::string> m_procfilter;
    std::vector<struct kinfo_proc> m_procs;
};

}

#endif
