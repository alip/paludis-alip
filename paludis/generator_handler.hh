/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2008 Ciaran McCreesh
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

#ifndef PALUDIS_GUARD_PALUDIS_GENERATOR_HANDLER_HH
#define PALUDIS_GUARD_PALUDIS_GENERATOR_HANDLER_HH 1

#include <paludis/generator_handler-fwd.hh>
#include <paludis/name-fwd.hh>
#include <paludis/environment-fwd.hh>
#include <paludis/package_id-fwd.hh>
#include <paludis/util/attributes.hh>
#include <tr1/memory>

namespace paludis
{
    class PALUDIS_VISIBLE GeneratorHandler
    {
        protected:
            virtual ~GeneratorHandler() = 0;

        public:
            virtual std::tr1::shared_ptr<const RepositoryNameSet> repositories(
                    const Environment * const) const
                PALUDIS_ATTRIBUTE((warn_unused_result)) = 0;

            virtual std::tr1::shared_ptr<const CategoryNamePartSet> categories(
                    const Environment * const,
                    const std::tr1::shared_ptr<const RepositoryNameSet> &) const
                PALUDIS_ATTRIBUTE((warn_unused_result)) = 0;

            virtual std::tr1::shared_ptr<const QualifiedPackageNameSet> packages(
                    const Environment * const,
                    const std::tr1::shared_ptr<const RepositoryNameSet> &,
                    const std::tr1::shared_ptr<const CategoryNamePartSet> &) const
                PALUDIS_ATTRIBUTE((warn_unused_result)) = 0;

            virtual std::tr1::shared_ptr<const PackageIDSet> ids(
                    const Environment * const,
                    const std::tr1::shared_ptr<const RepositoryNameSet> &,
                    const std::tr1::shared_ptr<const QualifiedPackageNameSet> &) const
                PALUDIS_ATTRIBUTE((warn_unused_result)) = 0;

            virtual std::string as_string() const = 0;
    };

    class PALUDIS_VISIBLE AllGeneratorHandlerBase :
        public GeneratorHandler
    {
        public:
            virtual std::tr1::shared_ptr<const RepositoryNameSet> repositories(
                    const Environment * const env) const;

            virtual std::tr1::shared_ptr<const CategoryNamePartSet> categories(
                    const Environment * const env,
                    const std::tr1::shared_ptr<const RepositoryNameSet> & repos) const;

            virtual std::tr1::shared_ptr<const QualifiedPackageNameSet> packages(
                    const Environment * const env,
                    const std::tr1::shared_ptr<const RepositoryNameSet> & repos,
                    const std::tr1::shared_ptr<const CategoryNamePartSet> & cats) const;

            virtual std::tr1::shared_ptr<const PackageIDSet> ids(
                    const Environment * const env,
                    const std::tr1::shared_ptr<const RepositoryNameSet> & repos,
                    const std::tr1::shared_ptr<const QualifiedPackageNameSet> & qpns) const;
    };
}

#endif
