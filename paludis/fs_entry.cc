/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006 Ciaran McCreesh <ciaranm@gentoo.org>
 * Copyright (c) 2006 Mark Loeser <halcy0n@gentoo.org>
 *
 * This file is part of the Paludis package manager. Paludis is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "fs_entry.hh"
#include "exception.hh"
#include "stringify.hh"

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <cstring>
#include <cstdlib>

/** \file
 * Implementation of paludis::FSEntry.
 *
 * \ingroup Filesystem
 */

using namespace paludis;

FSError::FSError(const std::string & message) throw () :
    Exception(message)
{
}

FSEntry::FSEntry(const std::string & path) :
    ComparisonPolicyType(&FSEntry::_path),
    _path(path),
    _stat_info(new struct stat),
    _checked(false)
{
    _normalise();
}

FSEntry::FSEntry(const FSEntry & other) :
    ComparisonPolicyType(&FSEntry::_path),
    _path(other._path),
    _stat_info(other._stat_info),
    _exists(other._exists),
    _checked(other._checked)
{
}

FSEntry::~FSEntry()
{
}

const FSEntry &
FSEntry::operator= (const FSEntry & other)
{
    _path = other._path;
    return *this;
}

FSEntry::operator std::string() const
{
    return _path;
}

FSEntry
FSEntry::operator/ (const FSEntry & rhs) const
{
    return FSEntry(_path + "/" + rhs._path);
}

FSEntry
FSEntry::operator/ (const std::string & rhs) const
{
    return operator/ (FSEntry(rhs));
}

const FSEntry &
FSEntry::operator/= (const FSEntry & rhs)
{
    _path.append("/");
    _path.append(rhs._path);
    _normalise();
    _checked = false;

    return *this;
}

bool
FSEntry::exists() const
{
    _stat();

    return _exists;
}

bool
FSEntry::is_directory() const
{
    _stat();

    if(_exists)
        return S_ISDIR((*_stat_info).st_mode);

    return false;
}

bool
FSEntry::is_regular_file() const
{
    _stat();

    if(_exists)
        return S_ISREG((*_stat_info).st_mode);

    return false;
}

bool
FSEntry::owner_has_read() const
{
    _stat();

    if(_exists)
        return (*_stat_info).st_mode & S_IRUSR;

    return false;
}

bool
FSEntry::owner_has_write() const
{
    _stat();

    if(_exists)
        return (*_stat_info).st_mode & S_IWUSR;

    return false;
}

bool
FSEntry::owner_has_execute() const
{
    _stat();

    if(_exists)
        return (*_stat_info).st_mode & S_IXUSR;

    return false;
}

bool
FSEntry::group_has_read() const
{
    _stat();

    if(_exists)
        return (*_stat_info).st_mode & S_IRGRP;

    return false;
}

bool
FSEntry::group_has_write() const
{
    _stat();

    if(_exists)
        return (*_stat_info).st_mode & S_IWGRP;

    return false;
}

bool
FSEntry::group_has_execute() const
{
    _stat();

    if(_exists)
        return (*_stat_info).st_mode & S_IXGRP;

    return false;
}

bool
FSEntry::others_has_read() const
{
    _stat();

    if(_exists)
        return (*_stat_info).st_mode & S_IROTH;

    return false;
}

bool
FSEntry::others_has_write() const
{
    _stat();

    if(_exists)
        return (*_stat_info).st_mode & S_IWOTH;

    return false;
}

bool
FSEntry::others_has_execute() const
{
    _stat();

    if(_exists)
        return (*_stat_info).st_mode & S_IXOTH;

    return false;
}

void
FSEntry::_normalise()
{
    try
    {
        std::string new_path;
        std::string::size_type p(0);
        while (p < _path.length())
        {
            if ('/' == _path[p])
            {
                new_path += '/';
                while (++p < _path.length())
                    if ('/' != _path[p])
                        break;
            }
            else
                new_path += _path[p++];
        }
        _path = new_path;

        if (! _path.empty())
            if ('/' == _path.at(_path.length() - 1))
                _path.erase(_path.length() - 1);
        if (_path.empty())
            _path = "/";
    }
    catch (const std::exception & e)
    {
        Context c("When normalising FSEntry path '" + _path + "':");
        throw InternalError(PALUDIS_HERE,
                "caught std::exception '" + stringify(e.what()) + "'");
    }
}

void
FSEntry::_stat() const
{
    if(_checked == true)
        return;

    if (0 != stat(_path.c_str(), _stat_info.raw_pointer()))
    {
        if (errno != ENOENT)
            throw FSError("Error running stat() on '" + stringify(_path) + "': "
                    + strerror(errno));

        _exists = false;
    }
    else
        _exists = true;

    _checked = true;
}

std::string
FSEntry::basename() const
{
    return _path.substr(_path.rfind('/') + 1);
}

FSEntry
FSEntry::realpath() const
{
    char r[PATH_MAX + 1];
    std::memset(r, 0, PATH_MAX + 1);
    if (! ::realpath(_path.c_str(), r))
        throw FSError("Could not resolve path '" + _path + "'");
    return FSEntry(r);
}

std::ostream &
paludis::operator<< (std::ostream & s, const FSEntry & f)
{
    s << std::string(f);
    return s;
}


