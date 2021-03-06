/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018      Lee Clagett <https://github.com/vtnerd>
 * Copyright 2018      SChernykh   <https://github.com/SChernykh>
 * Copyright 2019      Howard Chu  <https://github.com/hyc>
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


#include <cassert>
#include <cstring>


#include "base/net/stratum/Job.h"
#include "base/tools/Buffer.h"

xmrig::Job::Job(const String &clientId) :
    m_clientId(clientId)
{
}

bool xmrig::Job::isEqual(const Job &other) const
{
    return m_id == other.m_id && m_clientId == other.m_clientId && memcmp(m_blob, other.m_blob, sizeof(m_blob)) == 0;
}


bool xmrig::Job::setBlob(const char *blob)
{
    if (!blob) {
        return false;
    }

    m_size = strlen(blob);
    if (m_size % 2 != 0) {
        return false;
    }

    m_size /= 2;
    if (m_size < 76 || m_size >= sizeof(m_blob)) {
        return false;
    }

    if (!Buffer::fromHex(blob, m_size * 2, m_blob)) {
        return false;
    }

    return true;
}

bool xmrig::Job::setTarget(const char *target)
{
    if (!target) {
        return false;
    }

    const size_t len = strlen(target);

    if (len <= 8) {
        uint32_t tmp = 0;
        char str[8];
        memcpy(str, target, len);

        if (!Buffer::fromHex(str, 8, reinterpret_cast<uint8_t *>(&tmp)) || tmp == 0) {
            return false;
        }

        m_target = 0xFFFFFFFFFFFFFFFFULL / (0xFFFFFFFFULL / static_cast<uint64_t>(tmp));
    }
    else if (len <= 16) {
        m_target = 0;
        char str[16];
        memcpy(str, target, len);

        if (!Buffer::fromHex(str, 16, reinterpret_cast<uint8_t *>(&m_target)) || m_target == 0) {
            return false;
        }
    }
    else {
        return false;
    }

    m_diff = toDiff(m_target);
    return true;
}


void xmrig::Job::setDiff(uint64_t diff)
{
    m_diff   = diff;
    m_target = toDiff(diff);
}


void xmrig::Job::copy(const Job &other)
{
    m_size       = other.m_size;
    m_clientId   = other.m_clientId;
    m_id         = other.m_id;
    m_backend    = other.m_backend;
    m_diff       = other.m_diff;
    m_height     = other.m_height;
    m_target     = other.m_target;
    m_index      = other.m_index;
    m_seed       = other.m_seed;
    m_extraNonce = other.m_extraNonce;
    m_poolWallet = other.m_poolWallet;
    m_extraIters = other.m_extraIters;

    memcpy(m_blob, other.m_blob, sizeof(m_blob));
}


void xmrig::Job::move(Job &&other)
{
    m_size       = other.m_size;
    m_clientId   = std::move(other.m_clientId);
    m_id         = std::move(other.m_id);
    m_backend    = other.m_backend;
    m_diff       = other.m_diff;
    m_height     = other.m_height;
    m_target     = other.m_target;
    m_index      = other.m_index;
    m_seed       = std::move(other.m_seed);
    m_extraNonce = std::move(other.m_extraNonce);
    m_poolWallet = std::move(other.m_poolWallet);
    m_extraIters = std::move(other.m_extraIters);

    memcpy(m_blob, other.m_blob, sizeof(m_blob));

    other.m_size        = 0;
    other.m_diff        = 0;
}
