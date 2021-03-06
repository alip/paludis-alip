/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006, 2007, 2008 Ciaran McCreesh
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

#ifndef PALUDIS_GUARD_PALUDIS_DEP_TAG_FWD_HH
#define PALUDIS_GUARD_PALUDIS_DEP_TAG_FWD_HH 1

#include <paludis/util/set-fwd.hh>
#include <paludis/util/attributes.hh>

/** \file
 * Forward declarations for paludis/dep_tag.hh .
 *
 * \ingroup g_dep_spec
 */

namespace paludis
{
    class DepTagCategory;
    class DepTagCategoryFactory;

    class DepTag;
    class GLSADepTag;
    class GeneralSetDepTag;
    class DependencyDepTag;
    class TargetDepTag;

    class DepTagEntry;
    class DepTagEntryComparator;

    /**
     * Tags attached to a DepListEntry.
     *
     * \ingroup g_dep_spec
     */
    typedef Set<DepTagEntry, DepTagEntryComparator> DepListEntryTags;
}

#endif
