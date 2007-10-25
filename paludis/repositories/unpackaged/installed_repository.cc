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

#include <paludis/repositories/unpackaged/installed_repository.hh>
#include <paludis/repositories/unpackaged/installed_id.hh>
#include <paludis/repositories/unpackaged/ndbam.hh>
#include <paludis/repositories/unpackaged/ndbam_merger.hh>
#include <paludis/util/private_implementation_pattern-impl.hh>
#include <paludis/util/sequence.hh>
#include <paludis/util/iterator.hh>
#include <paludis/util/make_shared_ptr.hh>
#include <paludis/util/visitor-impl.hh>
#include <paludis/util/stringify.hh>
#include <paludis/util/set.hh>
#include <paludis/util/dir_iterator.hh>
#include <paludis/util/system.hh>
#include <paludis/action.hh>
#include <paludis/environment.hh>
#include <paludis/dep_tag.hh>
#include <paludis/metadata_key.hh>
#include <libwrapiter/libwrapiter_forward_iterator.hh>
#include <fstream>
#include <sstream>
#include <sys/time.h>

using namespace paludis;
using namespace paludis::unpackaged_repositories;

#include <paludis/repositories/unpackaged/installed_repository-sr.cc>

namespace
{
    bool supported_installed_unpackaged(const std::string & s)
    {
        return s == "installed_unpackaged-1";
    }
}

namespace paludis
{
    template <>
    struct Implementation<InstalledUnpackagedRepository>
    {
        const InstalledUnpackagedRepositoryParams params;
        mutable NDBAM ndbam;

        Implementation(const InstalledUnpackagedRepositoryParams & p) :
            params(p),
            ndbam(p.location, &supported_installed_unpackaged, "installed_unpackaged-1")
        {
        }
    };
}

InstalledUnpackagedRepository::InstalledUnpackagedRepository(
        const RepositoryName & n, const InstalledUnpackagedRepositoryParams & p) :
    PrivateImplementationPattern<InstalledUnpackagedRepository>(new Implementation<InstalledUnpackagedRepository>(p)),
    Repository(n, RepositoryCapabilities::create()
            .installed_interface(this)
            .sets_interface(this)
            .syncable_interface(0)
            .use_interface(0)
            .world_interface(0)
            .mirrors_interface(0)
            .environment_variable_interface(0)
            .provides_interface(0)
            .virtuals_interface(0)
            .make_virtuals_interface(0)
            .destination_interface(this)
            .licenses_interface(0)
            .e_interface(0)
            .hook_interface(0)
            .qa_interface(0)
            .manifest_interface(0),
            "installed-unpackaged")
{
}

InstalledUnpackagedRepository::~InstalledUnpackagedRepository()
{
}

tr1::shared_ptr<const PackageIDSequence>
InstalledUnpackagedRepository::do_package_ids(const QualifiedPackageName & q) const
{
    tr1::shared_ptr<NDBAMEntrySequence> entries(_imp->ndbam.entries(q));
    tr1::shared_ptr<PackageIDSequence> result(new PackageIDSequence);

    for (IndirectIterator<NDBAMEntrySequence::ConstIterator> e(entries->begin()), e_end(entries->end()) ;
            e != e_end ; ++e)
    {
        Lock l(*e->mutex);
        if (! e->package_id)
            e->package_id.reset(new InstalledUnpackagedID(_imp->params.environment, e->name, e->version,
                        e->slot, name(), e->fs_location, e->magic, root(), &_imp->ndbam));
        result->push_back(e->package_id);
    }

    return result;
}

tr1::shared_ptr<const QualifiedPackageNameSet>
InstalledUnpackagedRepository::do_package_names(const CategoryNamePart & c) const
{
    return _imp->ndbam.package_names(c);
}

tr1::shared_ptr<const CategoryNamePartSet>
InstalledUnpackagedRepository::do_category_names() const
{
    return _imp->ndbam.category_names();
}

tr1::shared_ptr<const CategoryNamePartSet>
InstalledUnpackagedRepository::do_category_names_containing_package(
        const PackageNamePart & p) const
{
    return _imp->ndbam.category_names_containing_package(p);
}

bool
InstalledUnpackagedRepository::do_has_package_named(const QualifiedPackageName & q) const
{
    return _imp->ndbam.has_package_named(q);
}

bool
InstalledUnpackagedRepository::do_has_category_named(const CategoryNamePart & c) const
{
    return _imp->ndbam.has_category_named(c);
}

namespace
{
    struct SomeIDsMightSupportVisitor :
        ConstVisitor<SupportsActionTestVisitorTypes>
    {
        bool result;

        void visit(const SupportsActionTest<UninstallAction> &)
        {
            result = true;
        }

        void visit(const SupportsActionTest<InstalledAction> &)
        {
            result = true;
        }

        void visit(const SupportsActionTest<ConfigAction> &)
        {
           result = false;
        }

        void visit(const SupportsActionTest<InfoAction> &)
        {
            result = false;
        }

        void visit(const SupportsActionTest<PretendAction> &)
        {
            result = false;
        }

        void visit(const SupportsActionTest<FetchAction> &)
        {
            result = false;
        }

        void visit(const SupportsActionTest<InstallAction> &)
        {
            result = false;
        }
    };
}

bool
InstalledUnpackagedRepository::do_some_ids_might_support_action(const SupportsActionTestBase & test) const
{
    SomeIDsMightSupportVisitor v;
    test.accept(v);
    return v.result;
}

void
InstalledUnpackagedRepository::merge(const MergeOptions & m)
{
    Context context("When merging '" + stringify(*m.package_id) + "' at '" + stringify(m.image_dir)
            + "' to InstalledUnpackagedRepository repository '" + stringify(name()) + "':");

    if (! is_suitable_destination_for(*m.package_id))
        throw InstallActionError("Not a suitable destination for '" + stringify(*m.package_id) + "'");

    tr1::shared_ptr<const PackageID> if_overwritten_id, if_same_name_id;
    {
        tr1::shared_ptr<const PackageIDSequence> ids(package_ids(m.package_id->name()));
        for (PackageIDSequence::ConstIterator v(ids->begin()), v_end(ids->end()) ;
                v != v_end ; ++v)
        {
            if_same_name_id = *v;
            if ((*v)->version() == m.package_id->version() && (*v)->slot() == m.package_id->slot())
            {
                if_overwritten_id = *v;
                break;
            }
        }
    }

    FSEntry uid_dir(_imp->params.location);
    if (if_same_name_id)
        uid_dir = if_same_name_id->fs_location_key()->value().dirname();
    else
    {
        std::string uid(stringify(m.package_id->name().category) + "---" + stringify(m.package_id->name().package));
        uid_dir /= "data";
        uid_dir.mkdir();
        uid_dir /= uid;
        uid_dir.mkdir();
    }

    FSEntry target_ver_dir(uid_dir);
    {
        struct timeval t;
        gettimeofday(&t, 0);
        std::ostringstream magic;
        magic << std::hex << t.tv_usec << "x" << t.tv_sec;
        target_ver_dir /= (stringify(m.package_id->version()) + ":" + stringify(m.package_id->slot()) + ":" + magic.str());
    }

    if (target_ver_dir.exists())
        throw InstallActionError("Temporary merge directory '" + stringify(target_ver_dir) + "' already exists, probably "
                "due to a previous failed install. If it is safe to do so, please remove this directory and try again.");
    target_ver_dir.mkdir();

    {
        std::ofstream source_repository_file(stringify(target_ver_dir / "source_repository").c_str());
        source_repository_file << m.package_id->repository()->name() << std::endl;
        if (! source_repository_file)
            throw InstallActionError("Could not write to '" + stringify(target_ver_dir / "source_repository") + "'");
    }

    NDBAMMerger merger(
            NDBAMMergerOptions::create()
            .environment(_imp->params.environment)
            .image(m.image_dir)
            .root(root())
            .contents_file(target_ver_dir / "contents")
            .config_protect(getenv_with_default("CONFIG_PROTECT", ""))
            .config_protect_mask(getenv_with_default("CONFIG_PROTECT_MASK", ""))
            .package_id(m.package_id));

    if (! merger.check())
    {
        for (DirIterator d(target_ver_dir, false), d_end ; d != d_end ; ++d)
            FSEntry(*d).unlink();
        target_ver_dir.rmdir();
        throw InstallActionError("Not proceeding with install due to merge sanity check failing");
    }

    merger.merge();

    _imp->ndbam.index(m.package_id->name(), uid_dir.basename());

    if (if_overwritten_id)
    {
        tr1::static_pointer_cast<const InstalledUnpackagedID>(if_overwritten_id)->uninstall(UninstallActionOptions::create()
                .no_config_protect(false),
                true);
    }
}

bool
InstalledUnpackagedRepository::is_suitable_destination_for(const PackageID & e) const
{
    std::string f(e.repository()->format());
    return f == "unpackaged";
}

bool
InstalledUnpackagedRepository::is_default_destination() const
{
    return _imp->params.environment->root() == root();
}

bool
InstalledUnpackagedRepository::want_pre_post_phases() const
{
    return true;
}

FSEntry
InstalledUnpackagedRepository::root() const
{
    return _imp->params.root;
}

void
InstalledUnpackagedRepository::invalidate()
{
    _imp.reset(new Implementation<InstalledUnpackagedRepository>(_imp->params));
}

void
InstalledUnpackagedRepository::invalidate_masks()
{
}

void
InstalledUnpackagedRepository::deindex(const QualifiedPackageName & q) const
{
    _imp->ndbam.deindex(q);
}

tr1::shared_ptr<SetSpecTree::ConstItem>
InstalledUnpackagedRepository::do_package_set(const SetName & s) const
{
    using namespace tr1::placeholders;

    Context context("When fetching package set '" + stringify(s) + "' from '" +
            stringify(name()) + "':");

    if ("everything" == s.data() || "ununused" == s.data())
    {
        tr1::shared_ptr<ConstTreeSequence<SetSpecTree, AllDepSpec> > result(new ConstTreeSequence<SetSpecTree, AllDepSpec>(
                    make_shared_ptr(new AllDepSpec)));
        tr1::shared_ptr<GeneralSetDepTag> tag(new GeneralSetDepTag(s, stringify(name())));

        tr1::shared_ptr<const CategoryNamePartSet> cats(category_names());
        for (CategoryNamePartSet::ConstIterator c(cats->begin()), c_end(cats->end()) ;
                c != c_end ; ++c)
        {
            tr1::shared_ptr<const QualifiedPackageNameSet> pkgs(package_names(*c));
            for (QualifiedPackageNameSet::ConstIterator e(pkgs->begin()), e_end(pkgs->end()) ;
                    e != e_end ; ++e)
            {
                tr1::shared_ptr<PackageDepSpec> spec(new PackageDepSpec(make_shared_ptr(new QualifiedPackageName(*e))));
                spec->set_tag(tag);
                result->add(make_shared_ptr(new TreeLeaf<SetSpecTree, PackageDepSpec>(spec)));
            }
        }

        return result;
    }
    else
        return tr1::shared_ptr<SetSpecTree::ConstItem>();
}

tr1::shared_ptr<const SetNameSet>
InstalledUnpackagedRepository::sets_list() const
{
    Context context("While generating the list of sets:");

    tr1::shared_ptr<SetNameSet> result(new SetNameSet);
    result->insert(SetName("everything"));
    result->insert(SetName("ununused"));
    return result;
}
