/*
    This file is part of the KDE libraries

    SPDX-FileCopyrightText: 1999 Matthias Kalle Dalheimer <kalle@kde.org>
    SPDX-FileCopyrightText: 2000 Charles Samuels <charles@kde.org>
    SPDX-FileCopyrightText: 2005 Joseph Wenninger <kde@jowenn.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KRANDOM_H
#define KRANDOM_H

#include <kcoreaddons_export.h>

#include <QRandomGenerator>
#include <QString>

#include <limits>

/*!
 * \namespace KRandom
 * \inmodule KCoreAddons
 *
 * \brief Helper to create random data.
 *
 * This namespace provides methods which generate random data.
 * KRandom is not recommended for serious random-number generation needs,
 * like cryptography.
 */
namespace KRandom
{
/*!
 * Generates a random string.  It operates in the range [A-Za-z0-9]
 *
 * \a length Generate a string of this length.
 *
 * Returns the random string
 */
KCOREADDONS_EXPORT QString randomString(int length);

/*!
 * Reorders the elements of the given container randomly using the given random number generator.
 *
 * The container needs to implement size() and T &operator[]
 *
 * \since 5.73
 */
template<typename T>
void shuffle(T &container, QRandomGenerator *generator)
{
    Q_ASSERT(container.size() <= std::numeric_limits<int>::max());
    // Fisher-Yates algorithm
    for (int index = container.size() - 1; index > 0; --index) {
        const int swapIndex = generator->bounded(index + 1);
        qSwap(container[index], container[swapIndex]);
    }
}

/*!
 * Reorders the elements of the given container randomly.
 *
 * The container needs to implement size() and T &operator[]
 *
 * \since 5.73
 */
template<typename T>
void shuffle(T &container)
{
    shuffle(container, QRandomGenerator::global());
}

}

#endif
