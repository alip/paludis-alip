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

#ifndef PALUDIS_GUARD_PALUDIS_REPOSITORIES_UNWRITTEN_UNWRITTEN_ID_HH
#define PALUDIS_GUARD_PALUDIS_REPOSITORIES_UNWRITTEN_UNWRITTEN_ID_HH 1

#include <paludis/util/named_value.hh>
#include <paludis/package_id.hh>
#include <paludis/name.hh>
#include <paludis/version_spec.hh>

namespace paludis
{
    namespace n
    {
        typedef Name<struct added_by_name> added_by;
        typedef Name<struct bug_ids_name> bug_ids;
        typedef Name<struct comment_name> comment;
        typedef Name<struct description_name> description;
        typedef Name<struct environment_name> environment;
        typedef Name<struct homepage_name> homepage;
        typedef Name<struct mask_name> mask;
        typedef Name<struct name_name> name;
        typedef Name<struct remote_ids_name> remote_ids;
        typedef Name<struct repository_name> repository;
        typedef Name<struct slot_name> slot;
        typedef Name<struct version_name> version;
    }

    namespace unwritten_repository
    {
        struct UnwrittenRepository;

        struct UnwrittenIDParams
        {
            NamedValue<n::added_by, std::tr1::shared_ptr<const MetadataValueKey<std::string> > > added_by;
            NamedValue<n::bug_ids, std::tr1::shared_ptr<const MetadataCollectionKey<Sequence<std::string> > > > bug_ids;
            NamedValue<n::comment, std::tr1::shared_ptr<const MetadataValueKey<std::string> > > comment;
            NamedValue<n::description, std::tr1::shared_ptr<const MetadataValueKey<std::string> > > description;
            NamedValue<n::environment, const Environment *> environment;
            NamedValue<n::homepage, std::tr1::shared_ptr<const MetadataSpecTreeKey<SimpleURISpecTree> > > homepage;
            NamedValue<n::mask, std::tr1::shared_ptr<const Mask> > mask;
            NamedValue<n::name, QualifiedPackageName> name;
            NamedValue<n::remote_ids, std::tr1::shared_ptr<const MetadataCollectionKey<Sequence<std::string> > > > remote_ids;
            NamedValue<n::repository, const UnwrittenRepository *> repository;
            NamedValue<n::slot, std::tr1::shared_ptr<const MetadataValueKey<SlotName> > > slot;
            NamedValue<n::version, VersionSpec> version;
        };

        class PALUDIS_VISIBLE UnwrittenID :
            public PackageID,
            private PrivateImplementationPattern<UnwrittenID>
        {
            private:
                PrivateImplementationPattern<UnwrittenID>::ImpPtr & _imp;

            protected:
                void need_keys_added() const;
                void need_masks_added() const;

            public:
                UnwrittenID(const UnwrittenIDParams &);
                ~UnwrittenID();

                 const std::string canonical_form(const PackageIDCanonicalForm) const;
                 const QualifiedPackageName name() const;
                 const VersionSpec version() const;
                 const std::tr1::shared_ptr<const Repository> repository() const;
                 virtual PackageDepSpec uniquely_identifying_spec() const;

                 const std::tr1::shared_ptr<const MetadataValueKey<SlotName> > slot_key() const;
                 const std::tr1::shared_ptr<const MetadataValueKey<std::tr1::shared_ptr<const PackageID> > > virtual_for_key() const;
                 const std::tr1::shared_ptr<const MetadataCollectionKey<KeywordNameSet> > keywords_key() const;
                 const std::tr1::shared_ptr<const MetadataSpecTreeKey<ProvideSpecTree> > provide_key() const;
                 const std::tr1::shared_ptr<const MetadataCollectionKey<PackageIDSequence> > contains_key() const;
                 const std::tr1::shared_ptr<const MetadataValueKey<std::tr1::shared_ptr<const PackageID> > >
                     contained_in_key() const;
                 const std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >
                     dependencies_key() const;
                 const std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >
                     build_dependencies_key() const;
                 const std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >
                     run_dependencies_key() const;
                 const std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >
                     post_dependencies_key() const;
                 const std::tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> >
                     suggested_dependencies_key() const;
                 const std::tr1::shared_ptr<const MetadataSpecTreeKey<FetchableURISpecTree> > fetches_key() const;
                 const std::tr1::shared_ptr<const MetadataSpecTreeKey<SimpleURISpecTree> > homepage_key() const;
                 const std::tr1::shared_ptr<const MetadataValueKey<std::string> > short_description_key() const;
                 const std::tr1::shared_ptr<const MetadataValueKey<std::string> > long_description_key() const;
                 const std::tr1::shared_ptr<const MetadataValueKey<std::tr1::shared_ptr<const Contents> > >
                     contents_key() const;
                 const std::tr1::shared_ptr<const MetadataTimeKey> installed_time_key() const;
                 const std::tr1::shared_ptr<const MetadataValueKey<FSEntry> > fs_location_key() const;
                 const std::tr1::shared_ptr<const MetadataCollectionKey<Set<std::string> > > behaviours_key() const;
                 const std::tr1::shared_ptr<const MetadataCollectionKey<Set<std::string> > > from_repositories_key() const;
                 const std::tr1::shared_ptr<const MetadataValueKey<std::tr1::shared_ptr<const Choices> > > choices_key() const;

                 bool supports_action(const SupportsActionTestBase &) const
                     PALUDIS_ATTRIBUTE((warn_unused_result));
                 void perform_action(Action &) const PALUDIS_ATTRIBUTE((noreturn));

                 std::tr1::shared_ptr<const Set<std::string> > breaks_portage() const
                     PALUDIS_ATTRIBUTE((warn_unused_result));

                 bool arbitrary_less_than_comparison(const PackageID &) const
                    PALUDIS_ATTRIBUTE((warn_unused_result));
                 std::size_t extra_hash_value() const
                    PALUDIS_ATTRIBUTE((warn_unused_result));
        };
    }
}

#endif
