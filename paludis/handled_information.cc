/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2007, 2008 Ciaran McCreesh
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

#include <paludis/handled_information.hh>
#include <paludis/util/private_implementation_pattern-impl.hh>
#include <paludis/dep_spec.hh>

using namespace paludis;

DepListEntryHandled::~DepListEntryHandled()
{
}

namespace paludis
{
    template <>
    struct Implementation<DepListEntryHandledSkippedUnsatisfied>
    {
        const PackageDepSpec spec;

        Implementation(const PackageDepSpec & s) :
            spec(s)
        {
        }
    };
}

DepListEntryHandledSkippedUnsatisfied::DepListEntryHandledSkippedUnsatisfied(const PackageDepSpec & s) :
    PrivateImplementationPattern<DepListEntryHandledSkippedUnsatisfied>(new Implementation<DepListEntryHandledSkippedUnsatisfied>(s))
{
}

DepListEntryHandledSkippedUnsatisfied::~DepListEntryHandledSkippedUnsatisfied()
{
}

const PackageDepSpec
DepListEntryHandledSkippedUnsatisfied::spec() const
{
    return _imp->spec;
}

namespace paludis
{
    template <>
    struct Implementation<DepListEntryHandledSkippedDependent>
    {
        const std::tr1::shared_ptr<const PackageID> id;

        Implementation(const std::tr1::shared_ptr<const PackageID> & i) :
            id(i)
        {
        }
    };
}

DepListEntryHandledSkippedDependent::DepListEntryHandledSkippedDependent(const std::tr1::shared_ptr<const PackageID> & i) :
    PrivateImplementationPattern<DepListEntryHandledSkippedDependent>(new Implementation<DepListEntryHandledSkippedDependent>(i))
{
}

DepListEntryHandledSkippedDependent::~DepListEntryHandledSkippedDependent()
{
}

const std::tr1::shared_ptr<const PackageID>
DepListEntryHandledSkippedDependent::id() const
{
    return _imp->id;
}


