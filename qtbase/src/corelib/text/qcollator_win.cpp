/****************************************************************************
**
** Copyright (C) 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcollator_p.h"
#include "qlocale_p.h"
#include "qstringlist.h"
#include "qstring.h"
#include <private/qsystemlibrary_p.h>

#include <QDebug>
#include <QOperatingSystemVersion.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

//NOTE: SORT_DIGITSASNUMBERS is available since win7
#ifndef SORT_DIGITSASNUMBERS
#define SORT_DIGITSASNUMBERS 8
#endif

// implemented in qlocale_win.cpp
extern LCID qt_inIsoNametoLCID(const char *name);

typedef int (*fnCompareStringEx)(const WCHAR*, DWORD, const WCHAR*, int, const WCHAR*, int, LPNLSVERSIONINFO, void*, LPARAM);
typedef int (*fnLCMapStringEx)(const WCHAR*, DWORD, const WCHAR*, int, WCHAR*, int, LPNLSVERSIONINFO, void*, LPARAM);

static fnCompareStringEx pCompareStringEx;
static fnLCMapStringEx pLCMapStringEx;
static bool resolved = false;

static void resolve()
{
    QSystemLibrary library(QLatin1String("kernel32"));
    pCompareStringEx = (fnCompareStringEx) library.resolve("CompareStringEx");
    pLCMapStringEx = (fnLCMapStringEx) library.resolve("LCMapStringEx");
    resolved = true;
}

static int compareStringEx(const QCollatorPrivate *p, const WCHAR *d1, int s1, const WCHAR *d2, int s2)
{
    return pCompareStringEx(LPCWSTR(p->localeName.utf16()), p->collator,
        d1, s1, d2, s2, nullptr, nullptr, 0);
}

static int compareString(const QCollatorPrivate *p, const WCHAR *d1, int s1, const WCHAR *d2, int s2)
{
    return CompareStringW(p->localeID, p->collator, d1, s1, d2, s2);
}

static int mapStringEx(const QCollatorPrivate *p, const WCHAR *in, int inSize, WCHAR *out, int outSize)
{
    return pLCMapStringEx(LPCWSTR(p->localeName.utf16()), LCMAP_SORTKEY | p->collator,
                          in, inSize, out, outSize, nullptr, nullptr, 0);
}

static int mapString(const QCollatorPrivate *p, const WCHAR *in, int inSize, WCHAR *out, int outSize)
{
    return LCMapStringW(p->localeID, LCMAP_SORTKEY | p->collator, in, inSize, out, outSize);
}

void QCollatorPrivate::init()
{
    collator = 0;
    if (isC())
        return;

    if (!resolved)
        resolve();
    if (pCompareStringEx && pLCMapStringEx)
    {
        localeName = locale.bcp47Name();
        pCompare = compareStringEx;
        pMapString = mapStringEx;
    } else
    {
        localeID = qt_inIsoNametoLCID(QLocalePrivate::get(locale)->bcp47Name().constData());
        pCompare = compareString;
        pMapString = mapString;
    }

    if (caseSensitivity == Qt::CaseInsensitive)
        collator |= NORM_IGNORECASE;

    // WINE does not support SORT_DIGITSASNUMBERS :-(
    // (and its std::sort() crashes on bad comparisons, QTBUG-74209)
    if (numericMode)
    {
        if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows7)
            collator |= SORT_DIGITSASNUMBERS;
        else
            qWarning() << "Numeric sorting unsupported on Windows versions older than Windows 7.";
    }

    if (ignorePunctuation)
        collator |= NORM_IGNORESYMBOLS;

    dirty = false;
}

void QCollatorPrivate::cleanup()
{
}

int QCollator::compare(QStringView s1, QStringView s2) const
{
    if (!s1.size())
        return s2.size() ? -1 : 0;
    if (!s2.size())
        return +1;

    if (d->isC())
        return s1.compare(s2, d->caseSensitivity);

    if (d->dirty)
        d->init();

    //* from Windows documentation *
    // Returns one of the following values if successful. To maintain the C
    // runtime convention of comparing strings, the value 2 can be subtracted
    // from a nonzero return value. Then, the meaning of <0, ==0, and >0 is
    // consistent with the C runtime.
    // [...] The function returns 0 if it does not succeed.
    // https://docs.microsoft.com/en-us/windows/desktop/api/stringapiset/nf-stringapiset-comparestringex#return-value

    const int ret = d->pCompare(d,
     reinterpret_cast<const WCHAR*>(s1.data()), s1.size(),
     reinterpret_cast<const WCHAR*>(s2.data()), s2.size());

    if (Q_LIKELY(ret))
        return ret - 2;

    switch (DWORD error = GetLastError())
    {
    case ERROR_INVALID_FLAGS:
        qWarning("Unsupported flags (%d) used in QCollator", int(d->collator));
        break;
    case ERROR_INVALID_PARAMETER:
        qWarning("Invalid parameter for QCollator::compare()");
        break;
    default:
        qWarning("Failed (%ld) comparison in QCollator::compare()", long(error));
        break;
    }
    // We have no idea what to return, so pretend we think they're equal.
    // At least that way we'll be consistent if we get the same values swapped ...
    return 0;
}

QCollatorSortKey QCollator::sortKey(const QString &string) const
{
    if (d->dirty)
        d->init();
    if (d->isC())
        return QCollatorSortKey(new QCollatorSortKeyPrivate(string));

    int size = d->pMapString(d, reinterpret_cast<const WCHAR*>(string.constData()), string.size(), nullptr, 0);

    QString ret(size, Qt::Uninitialized);
    int finalSize = d->pMapString(d, 
     reinterpret_cast<const WCHAR*>(string.constData()), string.size(),
     reinterpret_cast<WCHAR*>(ret.data()), ret.size());

    if (finalSize == 0)
        qWarning()
            << "there were problems when generating the ::sortKey by LCMapStringW with error:"
            << GetLastError();

    return QCollatorSortKey(new QCollatorSortKeyPrivate(std::move(ret)));
}

int QCollatorSortKey::compare(const QCollatorSortKey &otherKey) const
{
    return d->m_key.compare(otherKey.d->m_key);
}

QT_END_NAMESPACE
