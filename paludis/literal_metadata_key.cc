/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2007 Ciaran McCreesh
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

#include <paludis/literal_metadata_key.hh>
#include <paludis/util/private_implementation_pattern-impl.hh>
#include <paludis/util/fs_entry.hh>
#include <paludis/util/sequence.hh>
#include <paludis/util/wrapped_forward_iterator.hh>
#include <paludis/util/join.hh>
#include <paludis/util/tr1_functional.hh>
#include <paludis/util/visitor-impl.hh>
#include <paludis/formatter.hh>
#include <paludis/package_id.hh>

using namespace paludis;

namespace paludis
{
    template <>
    struct Implementation<LiteralMetadataStringKey>
    {
        const std::string value;

        Implementation(const std::string & v) :
            value(v)
        {
        }
    };

    template <>
    struct Implementation<LiteralMetadataFSEntryKey>
    {
        const FSEntry value;

        Implementation(const FSEntry & v) :
            value(v)
        {
        }
    };

    template <>
    struct Implementation<LiteralMetadataPackageIDKey>
    {
        const tr1::shared_ptr<const PackageID> value;

        Implementation(const tr1::shared_ptr<const PackageID> & v) :
            value(v)
        {
        }
    };

    template <>
    struct Implementation<LiteralMetadataFSEntrySequenceKey>
    {
        const tr1::shared_ptr<const FSEntrySequence> value;

        Implementation(const tr1::shared_ptr<const FSEntrySequence> & v) :
            value(v)
        {
        }
    };

    template <>
    struct Implementation<LiteralMetadataStringSetKey>
    {
        const tr1::shared_ptr<const Set<std::string> > value;

        Implementation(const tr1::shared_ptr<const Set<std::string> > & v) :
            value(v)
        {
        }
    };
}

LiteralMetadataStringKey::LiteralMetadataStringKey(const std::string & h, const std::string & r,
        const MetadataKeyType t, const std::string & v) :
    MetadataStringKey(h, r, t),
    PrivateImplementationPattern<LiteralMetadataStringKey>(new Implementation<LiteralMetadataStringKey>(v)),
    _imp(PrivateImplementationPattern<LiteralMetadataStringKey>::_imp)
{
}

LiteralMetadataStringKey::~LiteralMetadataStringKey()
{
}

const std::string
LiteralMetadataStringKey::value() const
{
    return _imp->value;
}

LiteralMetadataFSEntryKey::LiteralMetadataFSEntryKey(const std::string & h, const std::string & r,
        const MetadataKeyType t, const FSEntry & v) :
    MetadataFSEntryKey(h, r, t),
    PrivateImplementationPattern<LiteralMetadataFSEntryKey>(new Implementation<LiteralMetadataFSEntryKey>(v)),
    _imp(PrivateImplementationPattern<LiteralMetadataFSEntryKey>::_imp)
{
}

LiteralMetadataFSEntryKey::~LiteralMetadataFSEntryKey()
{
}

const FSEntry
LiteralMetadataFSEntryKey::value() const
{
    return _imp->value;
}

LiteralMetadataPackageIDKey::LiteralMetadataPackageIDKey(const std::string & h, const std::string & r,
        const MetadataKeyType t, const tr1::shared_ptr<const PackageID> & v) :
    MetadataPackageIDKey(h, r, t),
    PrivateImplementationPattern<LiteralMetadataPackageIDKey>(new Implementation<LiteralMetadataPackageIDKey>(v)),
    _imp(PrivateImplementationPattern<LiteralMetadataPackageIDKey>::_imp)
{
}

LiteralMetadataPackageIDKey::~LiteralMetadataPackageIDKey()
{
}

const tr1::shared_ptr<const PackageID>
LiteralMetadataPackageIDKey::value() const
{
    return _imp->value;
}

LiteralMetadataFSEntrySequenceKey::LiteralMetadataFSEntrySequenceKey(const std::string & h, const std::string & r,
        const MetadataKeyType t, const tr1::shared_ptr<const FSEntrySequence> & v) :
    MetadataCollectionKey<FSEntrySequence>(h, r, t),
    PrivateImplementationPattern<LiteralMetadataFSEntrySequenceKey>(new Implementation<LiteralMetadataFSEntrySequenceKey>(v)),
    _imp(PrivateImplementationPattern<LiteralMetadataFSEntrySequenceKey>::_imp)
{
}

LiteralMetadataFSEntrySequenceKey::~LiteralMetadataFSEntrySequenceKey()
{
}

const tr1::shared_ptr<const FSEntrySequence>
LiteralMetadataFSEntrySequenceKey::value() const
{
    return _imp->value;
}

namespace
{
    std::string format_fsentry(const FSEntry & i, const Formatter<FSEntry> & f)
    {
        return f.format(i, format::Plain());
    }
}

std::string
LiteralMetadataFSEntrySequenceKey::pretty_print_flat(const Formatter<FSEntry> & f) const
{
    using namespace tr1::placeholders;
    return join(value()->begin(), value()->end(), " ", tr1::bind(&format_fsentry, _1, f));
}

LiteralMetadataStringSetKey::LiteralMetadataStringSetKey(const std::string & h, const std::string & r,
        const MetadataKeyType t, const tr1::shared_ptr<const Set<std::string> > & v) :
    MetadataCollectionKey<Set<std::string> >(h, r, t),
    PrivateImplementationPattern<LiteralMetadataStringSetKey>(new Implementation<LiteralMetadataStringSetKey>(v)),
    _imp(PrivateImplementationPattern<LiteralMetadataStringSetKey>::_imp)
{
}

LiteralMetadataStringSetKey::~LiteralMetadataStringSetKey()
{
}

const tr1::shared_ptr<const Set<std::string> >
LiteralMetadataStringSetKey::value() const
{
    return _imp->value;
}

namespace
{
    std::string format_string(const std::string & i, const Formatter<std::string> & f)
    {
        return f.format(i, format::Plain());
    }
}

std::string
LiteralMetadataStringSetKey::pretty_print_flat(const Formatter<std::string> & f) const
{
    using namespace tr1::placeholders;
    return join(value()->begin(), value()->end(), " ", tr1::bind(&format_string, _1, f));
}
