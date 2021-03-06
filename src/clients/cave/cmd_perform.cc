/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009, 2010 Ciaran McCreesh
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

#include "cmd_perform.hh"
#include "cmd_resolve_cmdline.hh"
#include "exceptions.hh"
#include <paludis/args/args.hh>
#include <paludis/args/do_help.hh>
#include <paludis/name.hh>
#include <paludis/environment.hh>
#include <paludis/package_database.hh>
#include <paludis/repository.hh>
#include <paludis/user_dep_spec.hh>
#include <paludis/generator.hh>
#include <paludis/filtered_generator.hh>
#include <paludis/filter.hh>
#include <paludis/selection.hh>
#include <paludis/package_id.hh>
#include <paludis/action.hh>
#include <paludis/hook.hh>
#include <paludis/output_manager_from_environment.hh>
#include <paludis/output_manager.hh>
#include <paludis/ipc_output_manager.hh>
#include <paludis/util/set.hh>
#include <paludis/util/wrapped_forward_iterator.hh>
#include <paludis/util/make_shared_ptr.hh>
#include <paludis/util/make_named_values.hh>
#include <paludis/util/iterator_funcs.hh>
#include <paludis/util/return_literal_function.hh>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <set>

#include "command_command_line.hh"

using namespace paludis;
using namespace cave;
using std::cout;
using std::endl;

namespace
{
    struct PerformCommandLine :
        CaveCommandCommandLine
    {
        virtual std::string app_name() const
        {
            return "cave perform";
        }

        virtual std::string app_synopsis() const
        {
            return "Perform an action upon a package.";
        }

        virtual std::string app_description() const
        {
            return "Perform an action upon a package. Not suitable for direct use, although it "
                "may be useful in some more complex scripts.";
        }

        args::ArgsGroup g_general_options;
        args::SwitchArg a_if_supported;
        args::SwitchArg a_hooks;
        args::StringArg a_x_of_y;
        args::SwitchArg a_managed_output;

        args::ArgsGroup g_fetch_action_options;
        args::SwitchArg a_exclude_unmirrorable;
        args::SwitchArg a_fetch_unneeded;
        args::SwitchArg a_ignore_unfetched;
        args::SwitchArg a_regulars_only;

        args::ArgsGroup g_install_action_options;
        args::StringArg a_destination;
        args::StringSetArg a_replacing;
        args::StringSetArg a_skip_phase;
        args::StringSetArg a_abort_at_phase;
        args::StringSetArg a_skip_until_phase;

        args::ArgsGroup g_uninstall_action_options;
        args::StringArg a_config_protect;

        ResolveCommandLineImportOptions import_options;

        PerformCommandLine() :
            g_general_options(main_options_section(), "General Options",
                    "General options for all actions"),
            a_if_supported(&g_general_options, "if-supported", '\0',
                    "If the action is not supported, exit silently with success rather than erroring.", true),
            a_hooks(&g_general_options, "hooks", '\0',
                    "Also execute the appropriate hooks for the action.", true),
            a_x_of_y(&g_general_options, "x-of-y", '\0',
                    "Specify the value of the X_OF_Y variable that is passed to hooks."),
            a_managed_output(&g_general_options, "managed-output", '\0',
                    "Specify that our output is being managed by another process. Used by "
                    "'cave execute-resolution'; not for end user use.", false),

            g_fetch_action_options(main_options_section(), "Fetch Action Options",
                    "Options for if the action is 'fetch' or 'pretend-fetch'"),
            a_exclude_unmirrorable(&g_fetch_action_options, "exclude-unmirrorable", '\0',
                    "Do not include unmirrorable components", true),
            a_fetch_unneeded(&g_fetch_action_options, "fetch-unneeded", '\0',
                    "Also fetch components that aren't needed (e.g. because they are conditional upon "
                    "disabled options", true),
            a_ignore_unfetched(&g_fetch_action_options, "ignore-unfetched", '\0',
                    "Do not fetch any component that has not already been downloaded (but do verify "
                    "components that have already been downloaded", true),
            a_regulars_only(&g_fetch_action_options, "regulars-only", '\0',
                    "Only fetch regular components. If this option is not specified, the job cannot safely "
                    "be backgrounded or run in parallel with installs", true),

            g_install_action_options(main_options_section(), "Install Action Options",
                    "Options for if the action is 'install'"),
            a_destination(&g_install_action_options, "destination", '\0',
                    "The name of the repository to which the install should take place"),
            a_replacing(&g_install_action_options, "replacing", '\0',
                    "A spec that uniquely identifies an ID to be replaced as part of the install. May be "
                    "specified multiple times."),
            a_skip_phase(&g_install_action_options, "skip-phase", '\0',
                    "Skip the named phases"),
            a_abort_at_phase(&g_install_action_options, "abort-at-phase", '\0',
                    "Abort when a named phase is encounted"),
            a_skip_until_phase(&g_install_action_options, "skip-until-phase", '\0',
                    "Skip every phase until a named phase is encounted"),

            g_uninstall_action_options(main_options_section(), "Uninstall Action Options",
                    "Options for if the action is 'uninstall'"),
            a_config_protect(&g_uninstall_action_options, "config-protect", '\0',
                    "Specify additional items to include in the config protection list"),

            import_options(this)
        {
            add_usage_line("config spec");
            add_usage_line("fetch | pretend-fetch [ --exclude-unmirrorable ] [ --fetch-unneeded ]"
                    " [ --ignore-unfetched ] spec");
            add_usage_line("info spec");
            add_usage_line("install --destination repo [ --replacing spec ... ] spec");
            add_usage_line("pretend spec");
            add_usage_line("pretend-fetch spec");
            add_usage_line("uninstall [ --config-protect values ] spec");
        }
    };

    struct OutputManagerFromIPCOrEnvironment
    {
        std::tr1::shared_ptr<OutputManagerFromIPC> manager_if_ipc;
        std::tr1::shared_ptr<OutputManagerFromEnvironment> manager_if_env;

        OutputManagerFromIPCOrEnvironment(
                const Environment * const e,
                const PerformCommandLine & cmdline,
                const std::tr1::shared_ptr<const PackageID> & id)
        {
            if (cmdline.a_managed_output.specified())
                manager_if_ipc.reset(new OutputManagerFromIPC(e, id, oe_exclusive,
                            ClientOutputFeatures() + cof_summary_at_end));
            else
                manager_if_env.reset(new OutputManagerFromEnvironment(e, id, oe_exclusive,
                            ClientOutputFeatures() + cof_summary_at_end));
        }

        const std::tr1::shared_ptr<OutputManager> operator() (const Action & a)
        {
            if (manager_if_env)
                return (*manager_if_env)(a);
            else
                return (*manager_if_ipc)(a);
        }

        const std::tr1::shared_ptr<OutputManager> output_manager_if_constructed()
        {
            if (manager_if_env)
                return manager_if_env->output_manager_if_constructed();
            else
                return manager_if_ipc->output_manager_if_constructed();
        }

        void construct_standard_if_unconstructed()
        {
            if (manager_if_env)
                manager_if_env->construct_standard_if_unconstructed();
            else
                manager_if_ipc->construct_standard_if_unconstructed();
        }
    };

    void execute(
            const std::tr1::shared_ptr<Environment> & env,
            const PerformCommandLine & cmdline,
            const std::tr1::shared_ptr<const PackageID> & id,
            const std::string & action_name,
            Action & action)
    {
        if (cmdline.a_x_of_y.specified())
            std::cout << "\x1b]2;" << cmdline.a_x_of_y.argument() << " " << action_name << " "
                << stringify(*id) << "\x07" << std::flush;

        if (cmdline.a_hooks.specified())
            if (0 != env->perform_hook(Hook(action_name + "_pre")
                        ("TARGET", stringify(*id))
                        ("X_OF_Y", cmdline.a_x_of_y.argument())
                        ).max_exit_status())
                throw ActionAbortedError("Aborted by hook");

        id->perform_action(action);

        if (cmdline.a_hooks.specified())
            if (0 != env->perform_hook(Hook(action_name + "_post")
                        ("TARGET", stringify(*id))
                        ("X_OF_Y", cmdline.a_x_of_y.argument())
                        ).max_exit_status())
                throw ActionAbortedError("Aborted by hook");

        if (cmdline.a_x_of_y.specified())
            std::cout << "\x1b]2;Completed " << cmdline.a_x_of_y.argument() << " " << action_name << " "
                << stringify(*id) << "\x07" << std::flush;
    }

    bool ignore_nothing(const FSEntry &)
    {
        return false;
    }

    void perform_uninstall(
            const std::tr1::shared_ptr<Environment> & env,
            const PerformCommandLine & cmdline,
            const std::tr1::shared_ptr<const PackageID> & id,
            const std::string & action_name,
            const UninstallActionOptions & options)
    {
        UninstallAction uninstall_action(options);
        execute(env, cmdline, id, "clean", uninstall_action);

        if (cmdline.a_x_of_y.specified())
            std::cout << "\x1b]2;" << cmdline.a_x_of_y.argument() << " " << action_name << " "
                << stringify(*id) << "\x07" << std::flush;
    }

    struct WantInstallPhase
    {
        const PerformCommandLine & cmdline;
        OutputManagerFromIPCOrEnvironment & output_manager_holder;
        bool done_any;

        WantInstallPhase(const PerformCommandLine & c, OutputManagerFromIPCOrEnvironment & o) :
            cmdline(c),
            output_manager_holder(o),
            done_any(false)
        {
        }

        WantPhase operator() (const std::string & phase)
        {
            output_manager_holder.construct_standard_if_unconstructed();
            std::tr1::shared_ptr<OutputManager> output_manager(output_manager_holder.output_manager_if_constructed());

            if (cmdline.a_abort_at_phase.end_args() != std::find(
                        cmdline.a_abort_at_phase.begin_args(), cmdline.a_abort_at_phase.end_args(), phase))
            {
                output_manager->stdout_stream() << "+++ Aborting at phase '" + phase + "' as instructed" << endl;
                return wp_abort;
            }

            if (cmdline.a_skip_until_phase.end_args() != cmdline.a_skip_until_phase.begin_args())
                if (! done_any)
                    if (cmdline.a_skip_until_phase.end_args() == std::find(
                                cmdline.a_skip_until_phase.begin_args(), cmdline.a_skip_until_phase.end_args(), phase))
                    {
                        output_manager->stdout_stream() << "+++ Skipping phase '" << phase <<
                            "' as instructed since it is before a start phase" << endl;
                        return wp_skip;
                    }

            /* make --skip-until-phase foo --skip-phase foo work */
            done_any = true;

            if (cmdline.a_skip_phase.end_args() != std::find(
                        cmdline.a_skip_phase.begin_args(), cmdline.a_skip_phase.end_args(), phase))
            {
                output_manager->stdout_stream() << "+++ Skipping phase '" + phase + "' as instructed" << endl;
                return wp_skip;
            }

            if (cmdline.a_skip_phase.specified() || cmdline.a_skip_until_phase.specified() ||
                    cmdline.a_abort_at_phase.specified())
                output_manager->stdout_stream() << "+++ Executing phase '" + phase + "' as instructed" << endl;

            return wp_yes;
        }
    };

    struct OurPretendFetchAction :
        PretendFetchAction
    {
        std::set<FSEntry> already_downloaded;
        unsigned long size;
        bool overflow;

        OurPretendFetchAction(const FetchActionOptions & o) :
            PretendFetchAction(o),
            size(0),
            overflow(false)
        {
        }

        void will_fetch(const FSEntry & destination, const unsigned long size_in_bytes)
        {
            if (already_downloaded.end() != already_downloaded.find(destination))
                return;
            already_downloaded.insert(destination);

            unsigned long new_size(size + size_in_bytes);
            if (new_size < size)
                overflow = true;
            else
                size = new_size;
        }
    };
}

int
PerformCommand::run(
        const std::tr1::shared_ptr<Environment> & env,
        const std::tr1::shared_ptr<const Sequence<std::string > > & args
        )
{
    PerformCommandLine cmdline;
    cmdline.run(args, "CAVE", "CAVE_PERFORM_OPTIONS", "CAVE_PERFORM_CMDLINE");

    if (cmdline.a_help.specified())
    {
        cout << cmdline;
        return EXIT_SUCCESS;
    }

    if (2 != std::distance(cmdline.begin_parameters(), cmdline.end_parameters()))
        throw args::DoHelp("perform takes exactly two parameters");

    cmdline.import_options.apply(env);

    std::string action(*cmdline.begin_parameters());

    PackageDepSpec spec(parse_user_package_dep_spec(*next(cmdline.begin_parameters()), env.get(),
                UserPackageDepSpecOptions()));
    const std::tr1::shared_ptr<const PackageIDSequence> ids((*env)[selection::AllVersionsUnsorted(
                generator::Matches(spec, MatchPackageOptions()))]);
    if (ids->empty())
        throw NothingMatching(spec);
    else if (1 != std::distance(ids->begin(), ids->end()))
        throw BeMoreSpecific(spec, ids);
    const std::tr1::shared_ptr<const PackageID> id(*ids->begin());

    FetchParts parts;
    parts += fp_regulars;
    if (! cmdline.a_regulars_only.specified())
        parts += fp_extras;
    if (cmdline.a_fetch_unneeded.specified())
        parts += fp_unneeded;

    if (action == "config")
    {
        if (cmdline.a_if_supported.specified() && ! id->supports_action(SupportsActionTest<ConfigAction>()))
            return EXIT_SUCCESS;

        OutputManagerFromIPCOrEnvironment output_manager_holder(env.get(), cmdline, id);
        ConfigActionOptions options(make_named_values<ConfigActionOptions>(
                    n::make_output_manager() = std::tr1::ref(output_manager_holder)
                    ));
        ConfigAction config_action(options);
        execute(env, cmdline, id, action, config_action);
    }
    else if (action == "fetch")
    {
        if (cmdline.a_if_supported.specified() && ! id->supports_action(SupportsActionTest<FetchAction>()))
            return EXIT_SUCCESS;

        OutputManagerFromIPCOrEnvironment output_manager_holder(env.get(), cmdline, id);
        WantInstallPhase want_phase(cmdline, output_manager_holder);
        FetchActionOptions options(make_named_values<FetchActionOptions>(
                    n::errors() = make_shared_ptr(new Sequence<FetchActionFailure>),
                    n::exclude_unmirrorable() = cmdline.a_exclude_unmirrorable.specified(),
                    n::fetch_parts() = parts,
                    n::ignore_not_in_manifest() = false,
                    n::ignore_unfetched() = cmdline.a_ignore_unfetched.specified(),
                    n::make_output_manager() = std::tr1::ref(output_manager_holder),
                    n::safe_resume() = true,
                    n::want_phase() = want_phase
                    ));
        FetchAction fetch_action(options);
        execute(env, cmdline, id, action, fetch_action);
    }
    else if (action == "pretend-fetch")
    {
        if (cmdline.a_if_supported.specified() && ! id->supports_action(SupportsActionTest<PretendFetchAction>()))
            return EXIT_SUCCESS;

        OutputManagerFromIPCOrEnvironment output_manager_holder(env.get(), cmdline, id);
        FetchActionOptions options(make_named_values<FetchActionOptions>(
                    n::errors() = make_shared_ptr(new Sequence<FetchActionFailure>),
                    n::exclude_unmirrorable() = cmdline.a_exclude_unmirrorable.specified(),
                    n::fetch_parts() = parts,
                    n::ignore_not_in_manifest() = false,
                    n::ignore_unfetched() = cmdline.a_ignore_unfetched.specified(),
                    n::make_output_manager() = std::tr1::ref(output_manager_holder),
                    n::safe_resume() = true,
                    n::want_phase() = std::tr1::bind(return_literal_function(wp_yes))
                    ));
        OurPretendFetchAction pretend_fetch_action(options);
        execute(env, cmdline, id, action, pretend_fetch_action);

        if (! output_manager_holder.output_manager_if_constructed())
            output_manager_holder(pretend_fetch_action);

        if (pretend_fetch_action.overflow)
            output_manager_holder.output_manager_if_constructed()->stdout_stream()
                << "Total download size: more than " << pretend_fetch_action.size << " bytes" << endl;
        else
            output_manager_holder.output_manager_if_constructed()->stdout_stream()
                << "Total download size: " << pretend_fetch_action.size << " bytes" << endl;
    }
    else if (action == "info")
    {
        if (cmdline.a_if_supported.specified() && ! id->supports_action(SupportsActionTest<InfoAction>()))
            return EXIT_SUCCESS;

        OutputManagerFromIPCOrEnvironment output_manager_holder(env.get(), cmdline, id);
        InfoActionOptions options(make_named_values<InfoActionOptions>(
                    n::make_output_manager() = std::tr1::ref(output_manager_holder)
                    ));
        InfoAction info_action(options);
        execute(env, cmdline, id, action, info_action);
    }
    else if (action == "install")
    {
        if (cmdline.a_if_supported.specified() && ! id->supports_action(SupportsActionTest<InstallAction>()))
            return EXIT_SUCCESS;

        if (! cmdline.a_destination.specified())
            throw args::DoHelp("--destination must be specified for an install");

        const std::tr1::shared_ptr<Repository> destination(env->package_database()->fetch_repository(
                    RepositoryName(cmdline.a_destination.argument())));

        const std::tr1::shared_ptr<PackageIDSequence> replacing(new PackageIDSequence);
        for (args::StringSetArg::ConstIterator p(cmdline.a_replacing.begin_args()),
                p_end(cmdline.a_replacing.end_args()) ;
                p != p_end ; ++p)
        {
            PackageDepSpec rspec(parse_user_package_dep_spec(*p, env.get(), UserPackageDepSpecOptions()));
            const std::tr1::shared_ptr<const PackageIDSequence> rids((*env)[selection::AllVersionsUnsorted(
                        generator::Matches(rspec, MatchPackageOptions()))]);
            if (rids->empty())
                throw NothingMatching(rspec);
            else if (1 != std::distance(rids->begin(), rids->end()))
                throw BeMoreSpecific(rspec, rids);
            else
                replacing->push_back(*rids->begin());
        }

        OutputManagerFromIPCOrEnvironment output_manager_holder(env.get(), cmdline, id);
        WantInstallPhase want_phase(cmdline, output_manager_holder);
        InstallActionOptions options(make_named_values<InstallActionOptions>(
                    n::destination() = destination,
                    n::make_output_manager() = std::tr1::ref(output_manager_holder),
                    n::perform_uninstall() = std::tr1::bind(&perform_uninstall,
                            env, std::tr1::cref(cmdline), std::tr1::placeholders::_1,
                            action, std::tr1::placeholders::_2
                            ),
                    n::replacing() = replacing,
                    n::want_phase() = want_phase
                    ));
        InstallAction install_action(options);
        execute(env, cmdline, id, action, install_action);
    }
    else if (action == "pretend")
    {
        if (cmdline.a_if_supported.specified() && ! id->supports_action(SupportsActionTest<PretendAction>()))
            return EXIT_SUCCESS;

        OutputManagerFromIPCOrEnvironment output_manager_holder(env.get(), cmdline, id);
        PretendActionOptions options(make_named_values<PretendActionOptions>(
                    n::make_output_manager() = std::tr1::ref(output_manager_holder)
                    ));
        PretendAction pretend_action(options);
        execute(env, cmdline, id, action, pretend_action);
        if (pretend_action.failed())
            return EXIT_FAILURE;
    }
    else if (action == "uninstall")
    {
        if (cmdline.a_if_supported.specified() && ! id->supports_action(SupportsActionTest<UninstallAction>()))
            return EXIT_SUCCESS;

        OutputManagerFromIPCOrEnvironment output_manager_holder(env.get(), cmdline, id);
        UninstallActionOptions options(make_named_values<UninstallActionOptions>(
                    n::config_protect() = cmdline.a_config_protect.argument(),
                    n::if_for_install_id() = make_null_shared_ptr(),
                    n::ignore_for_unmerge() = &ignore_nothing,
                    n::is_overwrite() = false,
                    n::make_output_manager() = std::tr1::ref(output_manager_holder)
                    ));
        UninstallAction uninstall_action(options);
        execute(env, cmdline, id, action, uninstall_action);
    }
    else
        throw args::DoHelp("action '" + action + "' unrecognised");

    return EXIT_SUCCESS;
}

std::tr1::shared_ptr<args::ArgsHandler>
PerformCommand::make_doc_cmdline()
{
    return make_shared_ptr(new PerformCommandLine);
}


