/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006 Ciaran McCreesh <ciaranm@gentoo.org>
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

#ifndef PALUDIS_GUARD_PALUDIS_LOG_HH
#define PALUDIS_GUARD_PALUDIS_LOG_HH 1

#include <paludis/instantiation_policy.hh>
#include <paludis/private_implementation_pattern.hh>
#include <iosfwd>

namespace paludis
{
    /**
     * Specifies the level of a log message.
     *
     * Keep this in order. When deciding whether to display a message, Log
     * uses message log level >= current log level, so it's important that
     * least critical levels have lower numeric values.
     */
    enum LogLevel
    {
        ll_debug,             ///< Debug message
        ll_qa,                ///< QA messages
        ll_warning,           ///< Warning message
        last_ll,              ///< Number of items
        initial_ll = ll_debug ///< Initial value
    };

    class Log :
        public InstantiationPolicy<Log, instantiation_method::SingletonAtStartupTag>,
        private PrivateImplementationPattern<Log>
    {
        friend class InstantiationPolicy<Log, instantiation_method::SingletonAtStartupTag>;

        private:
            Log();

        public:
            ~Log();

            void set_log_level(const LogLevel);

            void message(const LogLevel, const std::string &);

            void set_log_stream(std::ostream * const);
    };
}

#endif
