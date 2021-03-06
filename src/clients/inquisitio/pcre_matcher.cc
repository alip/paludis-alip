/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2007, 2009 Ciaran McCreesh
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

#include "pcre_matcher.hh"
#include <pcrecpp.h>
#include <paludis/util/private_implementation_pattern-impl.hh>

using namespace paludis;
using namespace inquisitio;

namespace paludis
{
    template<>
    struct Implementation<PCREMatcher>
    {
        const pcrecpp::RE pattern;

        Implementation(const std::string & s) :
            pattern(s, pcrecpp::RE_Options().set_caseless(true))
        {
            if (! pattern.error().empty())
                throw BadPCREPatternError(s, pattern.error());
        }
    };
}

PCREMatcher::PCREMatcher(const std::string & s) :
    PrivateImplementationPattern<PCREMatcher>(new Implementation<PCREMatcher>(s))
{
}

bool
PCREMatcher::operator() (const std::string & s) const
{
    return _imp->pattern.PartialMatch(s);
}

PCREMatcher::~PCREMatcher()
{
}

BadPCREPatternError::BadPCREPatternError(const std::string & p, const std::string & m) throw () :
    Exception("Bad PCRE pattern '" + p + "': " + m)
{
}

