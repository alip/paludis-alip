/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2008, 2009, 2010 Ciaran McCreesh
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

#include <paludis/repositories/unavailable/unavailable_package_id.hh>
#include <paludis/repositories/unavailable/unavailable_repository.hh>
#include <paludis/util/private_implementation_pattern-impl.hh>
#include <paludis/util/stringify.hh>
#include <paludis/util/simple_visitor_cast.hh>
#include <paludis/util/set.hh>
#include <paludis/util/make_shared_ptr.hh>
#include <paludis/util/hashes.hh>
#include <paludis/util/wrapped_forward_iterator.hh>
#include <paludis/name.hh>
#include <paludis/version_spec.hh>
#include <paludis/metadata_key.hh>
#include <paludis/literal_metadata_key.hh>
#include <paludis/action.hh>
#include <paludis/unchoices_key.hh>
#include <paludis/user_dep_spec.hh>

using namespace paludis;
using namespace paludis::unavailable_repository;

namespace paludis
{
    template <>
    struct Implementation<UnavailablePackageID>
    {
        const Environment * const env;
        const QualifiedPackageName name;
        const VersionSpec version;
        const UnavailableRepository * const repo;

        const std::tr1::shared_ptr<const MetadataValueKey<SlotName> > slot_key;
        const std::tr1::shared_ptr<const MetadataValueKey<std::string> > description_key;
        const std::tr1::shared_ptr<const MetadataValueKey<std::string> > repository_homepage_key, repository_description_key;
        const std::tr1::shared_ptr<const MetadataCollectionKey<Set<std::string> > > from_repositories_key;
        const std::tr1::shared_ptr<const MetadataValueKey<std::tr1::shared_ptr<const Choices> > > choices_key;
        const std::tr1::shared_ptr<const Mask> mask;

        Implementation(
                const UnavailablePackageIDParams & e) :
            env(e.environment()),
            name(e.name()),
            version(e.version()),
            repo(e.repository()),
            slot_key(new LiteralMetadataValueKey<SlotName>("SLOT", "Slot", mkt_internal, e.slot())),
            description_key(e.description()),
            repository_homepage_key(e.repository_homepage()),
            repository_description_key(e.repository_description()),
            from_repositories_key(e.from_repositories()),
            choices_key(unchoices_key()),
            mask(e.mask())
        {
        }
    };
}

UnavailablePackageID::UnavailablePackageID(const UnavailablePackageIDParams & entry) :
    PrivateImplementationPattern<UnavailablePackageID>(new Implementation<UnavailablePackageID>(entry)),
    _imp(PrivateImplementationPattern<UnavailablePackageID>::_imp)
{
    add_metadata_key(_imp->slot_key);
    add_metadata_key(_imp->description_key);
    add_metadata_key(_imp->from_repositories_key);
    if (_imp->repository_homepage_key)
        add_metadata_key(_imp->repository_homepage_key);
    if (_imp->repository_description_key)
        add_metadata_key(_imp->repository_description_key);
    add_mask(_imp->mask);
}

UnavailablePackageID::~UnavailablePackageID()
{
}

void
UnavailablePackageID::need_keys_added() const
{
}

void
UnavailablePackageID::need_masks_added() const
{
}

const std::string
UnavailablePackageID::canonical_form(const PackageIDCanonicalForm f) const
{
    switch (f)
    {
        case idcf_full:
            return stringify(_imp->name) + "-" + stringify(_imp->version) +
                ":" + stringify(_imp->slot_key->value()) + "::" + stringify(_imp->repo->name()) +
                " (in ::" + *_imp->from_repositories_key->value()->begin() + ")";

        case idcf_no_version:
            return stringify(_imp->name) + ":" + stringify(_imp->slot_key->value()) +
                "::" + stringify(_imp->repo->name()) +
                " (in ::" + *_imp->from_repositories_key->value()->begin() + ")";

        case idcf_version:
            return stringify(_imp->version) +
                " (in ::" + *_imp->from_repositories_key->value()->begin() + ")";

        case idcf_no_name:
            return stringify(_imp->version) +
                ":" + stringify(_imp->slot_key->value()) + "::" + stringify(_imp->repo->name()) +
                " (in ::" + *_imp->from_repositories_key->value()->begin() + ")";

        case last_idcf:
            break;
    }

    throw InternalError(PALUDIS_HERE, "Bad PackageIDCanonicalForm");
}

PackageDepSpec
UnavailablePackageID::uniquely_identifying_spec() const
{
    return parse_user_package_dep_spec("=" + stringify(name()) + "-" + stringify(version()) +
            (slot_key() ? ":" + stringify(slot_key()->value()) : "") + "::" + stringify(repository()->name()) +
            "[." + _imp->from_repositories_key->raw_name() + "=" + *_imp->from_repositories_key->value()->begin() + "]",
            _imp->env, UserPackageDepSpecOptions());
}

const QualifiedPackageName
UnavailablePackageID::name() const
{
    return _imp->name;
}

const VersionSpec
UnavailablePackageID::version() const
{
    return _imp->version;
}

const std::tr1::shared_ptr<const Repository>
UnavailablePackageID::repository() const
{
    return _imp->repo->shared_from_this();
}

bool
UnavailablePackageID::supports_action(const SupportsActionTestBase & a) const
{
    return simple_visitor_cast<const SupportsActionTest<InstallAction> >(a);
}

void
UnavailablePackageID::perform_action(Action & a) const
{
    throw ActionFailedError("Unsupported action: " + a.simple_name());
}

std::tr1::shared_ptr<const Set<std::string> >
UnavailablePackageID::breaks_portage() const
{
    return make_shared_ptr(new Set<std::string>);
}

bool
UnavailablePackageID::arbitrary_less_than_comparison(const PackageID & other) const
{
    if (! other.slot_key())
        return false;

    if (slot_key()->value() < other.slot_key()->value())
        return true;
    if (slot_key()->value() > other.slot_key()->value())
        return false;

    std::tr1::shared_ptr<const MetadataCollectionKey<Set<std::string > > > k(other.from_repositories_key());
    if (! k)
        throw InternalError(PALUDIS_HERE, "other has no from_repositories_key()");
    if (1 != k->value()->size())
        throw InternalError(PALUDIS_HERE, "other has bad from_repositories_key");

    return *_imp->from_repositories_key->value()->begin() < *k->value()->begin();
}

std::size_t
UnavailablePackageID::extra_hash_value() const
{
    return Hash<std::pair<SlotName, std::string> >()(std::make_pair(
                slot_key()->value(), *_imp->from_repositories_key->value()->begin()));
}

const std::tr1::shared_ptr<const MetadataCollectionKey<PackageIDSequence> >
UnavailablePackageID::contains_key() const
{
    return std::tr1::shared_ptr<const MetadataCollectionKey<PackageIDSequence> >();
}

const std::tr1::shared_ptr<const MetadataValueKey<std::tr1::shared_ptr<const PackageID> > >
UnavailablePackageID::contained_in_key() const
{
    return std::tr1::shared_ptr<const MetadataValueKey<std::tr1::shared_ptr<const PackageID> > >();
}

const std::tr1::shared_ptr<const MetadataValueKey<FSEntry> >
UnavailablePackageID::fs_location_key() const
{
    return std::tr1::shared_ptr<const MetadataValueKey<FSEntry> >();
}

const std::tr1::shared_ptr<const MetadataCollectionKey<Set<std::string> > >
UnavailablePackageID::behaviours_key() const
{
    return make_null_shared_ptr();
}

const std::tr1::shared_ptr<const MetadataValueKey<std::tr1::shared_ptr<const PackageID> > >
UnavailablePackageID::virtual_for_key() const
{
    return std::tr1::shared_ptr<const MetadataValueKey<std::tr1::shared_ptr<const PackageID> > >();
}

const std::tr1::shared_ptr<const MetadataCollectionKey<KeywordNameSet> >
UnavailablePackageID::keywords_key() const
{
    return std::tr1::shared_ptr<const MetadataCollectionKey<KeywordNameSet> >();
}

const std::tr1::shared_ptr<const MetadataSpecTreeKey<ProvideSpecTree> >
UnavailablePackageID::provide_key() const
{
    return std::tr1::shared_ptr<const MetadataSpecTreeKey<ProvideSpecTree> >();
}

const std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >
UnavailablePackageID::dependencies_key() const
{
    return std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >();
}

const std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >
UnavailablePackageID::build_dependencies_key() const
{
    return std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >();
}

const std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >
UnavailablePackageID::run_dependencies_key() const
{
    return std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >();
}

const std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >
UnavailablePackageID::post_dependencies_key() const
{
    return std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >();
}

const std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >
UnavailablePackageID::suggested_dependencies_key() const
{
    return std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >();
}

const std::tr1::shared_ptr<const MetadataValueKey<std::string> >
UnavailablePackageID::short_description_key() const
{
    return _imp->description_key;
}

const std::tr1::shared_ptr<const MetadataValueKey<std::string> >
UnavailablePackageID::long_description_key() const
{
    return std::tr1::shared_ptr<const MetadataValueKey<std::string> >();
}

const std::tr1::shared_ptr<const MetadataSpecTreeKey<FetchableURISpecTree> >
UnavailablePackageID::fetches_key() const
{
    return std::tr1::shared_ptr<const MetadataSpecTreeKey<FetchableURISpecTree> >();
}

const std::tr1::shared_ptr<const MetadataSpecTreeKey<SimpleURISpecTree> >
UnavailablePackageID::homepage_key() const
{
    return std::tr1::shared_ptr<const MetadataSpecTreeKey<SimpleURISpecTree> >();
}

const std::tr1::shared_ptr<const MetadataValueKey<std::tr1::shared_ptr<const Contents> > >
UnavailablePackageID::contents_key() const
{
    return std::tr1::shared_ptr<const MetadataValueKey<std::tr1::shared_ptr<const Contents> > >();
}

const std::tr1::shared_ptr<const MetadataTimeKey>
UnavailablePackageID::installed_time_key() const
{
    return std::tr1::shared_ptr<const MetadataTimeKey>();
}

const std::tr1::shared_ptr<const MetadataCollectionKey<Set<std::string> > >
UnavailablePackageID::from_repositories_key() const
{
    return _imp->from_repositories_key;
}

const std::tr1::shared_ptr<const MetadataValueKey<std::tr1::shared_ptr<const Choices> > >
UnavailablePackageID::choices_key() const
{
    return _imp->choices_key;
}

const std::tr1::shared_ptr<const MetadataValueKey<SlotName> >
UnavailablePackageID::slot_key() const
{
    return _imp->slot_key;
}

template class PrivateImplementationPattern<UnavailablePackageID>;

