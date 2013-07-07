#ifndef FORMATSIZE_H
#define FORMATSIZE_H

#include <QString>

static inline QString PrettySize(unsigned long nbytes)
{
    if (nbytes < 1024) {
        return QString("%1 B").arg(nbytes);
    } else if ((nbytes >> 10) < 1024) {
        return QString("%1 KB").arg(nbytes/1024.f, 0, 'f', 2);
    } else if ((nbytes >> 20) < 1024) {
        return QString("%1 MB").arg(nbytes/(1024*1024.f), 0, 'f', 2);
    } else {
        return QString("%1 GB").arg(nbytes/(1024*1024*1024.f), 0, 'f', 2);
    }
}

static inline QString PrettySize(int pages, int shift)
{
    unsigned long nbytes = (unsigned long) pages << shift;
    return PrettySize(nbytes);
}

#endif
