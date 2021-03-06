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

#include "cmd_resolve_cmdline.hh"
#include <paludis/args/do_help.hh>
#include <paludis/repository.hh>
#include <paludis/environment.hh>
#include <paludis/package_database.hh>
#include <paludis/util/map.hh>
#include <paludis/repository_factory.hh>
#include <tr1/memory>

using namespace paludis;
using namespace cave;

namespace
{
    std::string from_keys(const std::tr1::shared_ptr<const Map<std::string, std::string> > & m,
            const std::string & k)
    {
        Map<std::string, std::string>::ConstIterator mm(m->find(k));
        if (m->end() == mm)
            return "";
        else
            return mm->second;
    }
}

ResolveCommandLineResolutionOptions::ResolveCommandLineResolutionOptions(args::ArgsHandler * const h) :
    ArgsSection(h, "Resolution Options"),
    g_execution_options(this, "Execution Options", "Control execution."),
    a_execute(&g_execution_options, "execute", 'x', "Execute the suggested actions", true),

    g_convenience_options(this, "Convenience Options", "Broad behaviour options."),
    a_lazy(&g_convenience_options, "lazy", 'z', "Do as little work as possible.", true),
    a_complete(&g_convenience_options, "complete", 'c', "Do all optional work.", true),
    a_everything(&g_convenience_options, "everything", 'e', "Do all optional work, and also reinstall", true),

    g_resolution_options(this, "Resolution Options", "Resolution options."),
//    a_permit_older_slot_uninstalls(&g_resolution_options, "permit-older-slot-uninstalls", '\0',
//            "Uninstall slots of packages if they are blocked by other slots of that package "
//            "with a newer version", true),
    a_permit_uninstall(&g_resolution_options, "permit-uninstall", 'U',
            "Permit uninstallation of packages matching the supplied specification, e.g. to resolve "
            "blockers. May be specified multiple times. Use '*/*' to allow all uninstalls, but note "
            "that the resolver will sometimes come up with extremely bad solutions to fixing blocks "
            "and may suggest stupid and dangerous uninstalls."),
    a_permit_downgrade(&g_resolution_options, "permit-downgrade", 'd', "Permit downgrades matching the supplied "
            "specification. Use '*/*' to allow all downgrades."),
    a_permit_old_version(&g_resolution_options, "permit-old-version", 'o', "Permit installs of versions matching the supplied "
            "specification even if those versions are worse than the best visible version in the slot. Use '*/*' "
            "to allow all worse versions to be installed."),
    a_purge(&g_resolution_options, "purge", 'P',
            "Purge packages matching the given specification, if they will no longer be used after "
            "a resolution. Use '*/*' to accept all purges, but note that by doing so you are putting "
            "a great deal of trust in package authors to get dependencies right."),

    g_dependent_options(this, "Dependent Options", "Dependent options. A package is dependent if it "
            "requires (or looks like it might require) a package which is being removed. By default, "
            "dependent packages are treated as errors. These options specify a different behaviour."),
    a_uninstalls_may_break(&g_dependent_options, "uninstalls-may-break", 'u',
            "Permit uninstalls that might break packages matching the specified specification. May be "
            "specified multiple times. Use '*/*' to allow all packages to be broken. Use 'system' to "
            "allow system packages to be uninstalled."),
    a_remove_if_dependent(&g_dependent_options, "remove-if-dependent", 'r',
            "Remove dependent packages that might be broken by other changes if those packages match "
            "the specified specification. May be specified multiple times. Use '*/*' to remove all "
            "dependent packages that might be broken, recursively."),
    a_less_restrictive_remove_blockers(&g_dependent_options, "less-restrictive-remove-blockers", 'l',
            "Use less restrictive blockers for packages matching the supplied specification if that "
            "package is to be removed by --remove-if-dependent. May be specified multiple times. "
            "Normally removing dependents is done by a pseudo-block in the form '!cat/pkg:slot'. If "
            "matched by this option, the block will instead only block the installed dependent "
            "package, so if reinstalling or upgrading the package will make it no longer be dependent "
            "then this will be done instead."),

    g_keep_options(this, "Reinstall Options", "Control whether installed packages are kept."),
    a_keep_targets(&g_keep_options, "keep-targets", 'K',
            "Select whether to keep target packages",
            args::EnumArg::EnumArgOptions
            ("auto",                  'a', "If the target is a set, if-same, otherwise never")
            ("never",                 'n', "Never")
            ("if-transient",          't', "Only if the installed package is transient "
                                           "(e.g. from 'importare')")
            ("if-same",               's', "If it is the same as the proposed replacement")
            ("if-same-version",       'v', "If it is the same version as the proposed replacement")
            ("if-possible",           'p', "If possible"),

            "auto"
            ),
    a_keep(&g_keep_options, "keep", 'k',
            "Select whether to keep installed packages that are not targets",
            args::EnumArg::EnumArgOptions
            ("never",                 'n', "Never")
            ("if-transient",          't', "Only if the installed package is transient "
                                           "(e.g. from 'importare') (default if --everything)")
            ("if-same",               's', "If it is the same as the proposed replacement "
                                           "(default if --complete)")
            ("if-same-version",       'v', "If it is the same version as the proposed replacement")
            ("if-possible",           'p', "If possible"),

            "if-possible"
            ),
    a_reinstall_scm(&g_keep_options, "reinstall-scm", 'R',
            "Select whether to reinstall SCM packages that would otherwise be kept",
            args::EnumArg::EnumArgOptions
            ("always",                'a', "Always")
            ("daily",                 'd', "If they were installed more than a day ago")
            ("weekly",                'w', "If they were installed more than a week ago (default if --complete)")
            ("never",                 'n', "Never"),
            "never"
            ),
//    a_reinstall_for_removals(&g_keep_options, "reinstall-for-removals", '\0',
//            "Select whether to rebuild packages if rebuilding would avoid an unsafe removal", true),
    a_with(&g_keep_options, "with", 'w',
            "Never keep installed packages with the supplied package name. May be specified "
            "multiple times."),
    a_without(&g_keep_options, "without", 'W',
            "Keep installed packages with the supplied package name if possible. May be specified "
            "multiple times."),

    g_slot_options(this, "Slot Options", "Control which slots are considered."),
    a_target_slots(&g_slot_options, "target-slots", 'S',
            "Which slots to consider for targets",
            args::EnumArg::EnumArgOptions
            ("best-or-installed",     'x', "Consider the best slot, if it is not installed, "
                                           "or all installed slots otherwise")
            ("installed-or-best",     'i', "Consider all installed slots, or the best "
                                           "installable slot if nothing is installed")
            ("all",                   'a', "Consider all installed slots and the best installable slot"
                                           " (default if --complete or --everything)")
            ("best",                  'b', "Consider the best installable slot only"
                                           " (default if --lazy)"),
            "best-or-installed"
            ),
    a_slots(&g_slot_options, "slots", 's',
            "Which slots to consider for packages that are not targets",
            args::EnumArg::EnumArgOptions
            ("best-or-installed",     'x', "Consider the best slot, if it is not installed, "
                                           "or all installed slots otherwise")
            ("installed-or-best",     'i', "Consider all installed slots, or the best "
                                           "installable slot if nothing is installed")
            ("all",                   'a', "Consider all installed slots and the best installable slot"
                                           " (default if --complete or --everything)")
            ("best",                  'b', "Consider the best installable slot only"
                                           " (default if --lazy)"),
            "best-or-installed"
            ),

    g_dependency_options(this, "Dependency Options", "Control which dependencies are followed."),
    a_follow_installed_build_dependencies(&g_dependency_options, "follow-installed-build-dependencies", 'D',
            "Follow build dependencies for installed packages (default if --complete or --everything)", true),
    a_no_follow_installed_dependencies(&g_dependency_options, "no-follow-installed-dependencies", 'n',
            "Ignore dependencies (except compiled-against dependencies, which are always taken) "
            "for installed packages. (default if --lazy)", true),
    a_no_dependencies_from(&g_dependency_options, "no-dependencies-from", '0',
            "Ignore dependencies (not blockers) from packages matching the supplied specification. May be "
            "specified multiple times. Use '*/*' to ignore all dependencies. Use of this option can lead to "
            "horrible breakages."),
    a_no_blockers_from(&g_dependency_options, "no-blockers-from", '!',
            "Ignore blockers from packages matching the supplied specification. May be specified "
            "multiple times. Use '*/*' to ignore all blockers. Use of this option can lead to "
            "horrible breakages."),

    g_suggestion_options(this, "Suggestion Options", "Control whether suggestions are taken. Suggestions that are "
            "already installed are instead treated as hard dependencies."),
    a_suggestions(&g_suggestion_options, "suggestions", '\0', "How to treat suggestions and recommendations",
            args::EnumArg::EnumArgOptions
            ("ignore",                     "Ignore suggestions")
            ("display",                    "Display suggestions, but do not take them unless explicitly told to do so")
            ("take",                       "Take all suggestions"),
            "display"),
    a_recommendations(&g_suggestion_options, "recommendations", '\0', "How to treat recommendations",
            args::EnumArg::EnumArgOptions
            ("ignore",                     "Ignore recommendations")
            ("display",                    "Display recommendations, but do not take them unless explicitly told to do so")
            ("take",                       "Take all recommendations"),
            "take"),
    a_take(&g_suggestion_options, "take", 't', "Take any suggestion matching the supplied package specification"
            " (e.g. --take 'app-vim/securemodelines' or --take 'app-vim/*')"),
    a_take_from(&g_suggestion_options, "take-from", 'T', "Take all suggestions made by any package matching the "
            "supplied package specification"),
    a_ignore(&g_suggestion_options, "ignore", 'i', "Discard any suggestion matching the supplied package specification"),
    a_ignore_from(&g_suggestion_options, "ignore-from", 'I', "Discard all suggestions made by any package matching the "
            "supplied package specification"),

    g_package_options(this, "Package Selection Options", "Control which packages are selected."),
    a_favour(&g_package_options, "favour", 'F', "If there is a choice (e.g. || ( ) dependencies), favour the "
            "specified package names"),
    a_avoid(&g_package_options, "avoid", 'A', "If there is a choice (e.g. || ( ) dependencies), avoid the "
            "specified package names"),

//    g_ordering_options(this, "Package Ordering Options", "Control the order in which packages are installed"),
//    a_early(&g_ordering_options, "early", 'E', "Try to install the specified package name as early as possible"),
//    a_late(&g_ordering_options, "late", 'L', "Try to install the specified package name as late as possible"),

    g_preset_options(this, "Preset Options", "Preset various constraints."),
    a_preset(&g_preset_options, "preset", 'p', "Preset a given constraint. For example, --preset =cat/pkg-2.1 will tell "
            "the resolver to use that particular version. Note that this may lead to errors, if the specified version "
            "does not satisfy other constraints. Also note that specifying a preset will not force a package to be "
            "considered if it would otherwise not be part of the resolution set."),

    g_destination_options(this, "Destination Options", "Control to which destinations packages are installed. "
            "If no options from this group are selected, install only to /. Otherwise, install to all of the "
            "specified destinations, and install to / as necessary to satisfy build dependencies."),
//    a_fetch(&g_destination_options, "fetch", 'f', "Only fetch packages, do not install anything", true),
    a_create_binaries_for_targets(&g_destination_options, "create-binaries-for-targets", 'b',
            "Rather than installing targets to /, instead create binary packages for targets, and install packages "
            "to / as necessary for dependencies.", true),
//    a_install_via_binary(&g_destination_options, "install-via-binary", 'B',
//            "If installing a package to / that matches the supplied spec, create a binary package and install that. May "
//            "be specified multiple times."),

//    g_query_options(this, "Query Options", "Query the user interactively when making decisions. "
//            "If only --query is specified, prompt for everything. Otherwise, prompt only for the specified decisions."),
//    a_query(&g_query_options, "query", 'q', "Query the user interactively when making decisions", true),
//    a_query_slots(&g_query_options, "query-slots", '\0', "Prompt for which slots to consider when a spec that does "
//            "not specify a particular slot is specified", true),
//    a_query_decisions(&g_query_options, "query-decisions", '\0', "Prompt for which decision to make for every "
//            "resolvent", true),
//    a_query_order(&g_query_options, "query-order", '\0', "Prompt for which jobs to order", true),

    g_dump_options(this, "Dump Options", "Dump the resolver's state to stdout after completion, or when an "
            "error occurs. For debugging purposes; produces rather a lot of noise."),
    a_dump(&g_dump_options, "dump", '\0', "Dump debug output", true),
    a_dump_restarts(&g_dump_options, "dump-restarts", '\0', "Dump restarts", true)
{
}

ResolveCommandLineDisplayOptions::ResolveCommandLineDisplayOptions(args::ArgsHandler * const h) :
    ArgsSection(h, "Display Options"),
    g_display_options(this, "Display Options", "Options relating to the resolution display."),
    a_show_option_descriptions(&g_display_options, "show-option-descriptions", '\0',
            "Whether to display descriptions for package options",
            args::EnumArg::EnumArgOptions
            ("none",                       "Don't show any descriptions")
            ("new",                        "Show for any new options")
            ("changed",                    "Show for new or changed options")
            ("all",                        "Show all options"),
            "changed"
            ),
    a_show_descriptions(&g_display_options, "show-descriptions", '\0',
            "Whether to display package descriptions",
            args::EnumArg::EnumArgOptions
            ("none",                       "Don't show any descriptions")
            ("new",                        "Show for new packages")
            ("all",                        "Show for all packages"),
            "new"
            ),
    g_explanations(this, "Explanations", "Options requesting the resolver explain a particular decision "
            "that it made"),
    a_explain(&g_explanations, "explain", 'X', "Explain why the resolver made a particular decision. The "
            "argument is a package dependency specification, so --explain dev-libs/boost or --explain qt:3"
            " or even --explain '*/*' (although --dump is a better way of getting highly noisy debug output)."),
    a_show_all_jobs(&g_explanations, "show-all-jobs", '\0', "Show all jobs that will be executed, rather than "
            "just installs.", true)
{
}

ResolveCommandLineExecutionOptions::ResolveCommandLineExecutionOptions(args::ArgsHandler * const h) :
    ArgsSection(h, "Execution Options"),

    g_world_options(this, "World Options", "Options controlling how the 'world' set is modified"),
    a_preserve_world(&g_world_options, "preserve-world", '1', "Do not modify the 'world' set", true),

    g_failure_options(this, "Failure Options", "Failure handling options."),
    a_continue_on_failure(&g_failure_options, "continue-on-failure", 'C',
            "Whether to continue after an error occurs",
            args::EnumArg::EnumArgOptions
            ("never",                      'n', "Never")
            ("if-satisfied",               's', "If remaining packages' dependencies are satisfied")
            ("if-independent",             'i', "If remaining packages do not depend upon any failing package")
            ("always",                     'a', "Always (dangerous)"),
            "never"),
    a_resume_file(&g_failure_options, "resume-file", '\0',
            "Write resume information to the specified file. If a build fails, or if '--execute' is not "
            "specified, then 'cave resume' can resume execution from this file."),

    g_phase_options(this, "Phase Options", "Options controlling which phases to execute. No sanity checking "
            "is done, allowing you to shoot as many feet off as you desire. Phase names do not have the "
            "src_, pkg_ or builtin_ prefix, so 'init', 'preinst', 'unpack', 'merge', 'strip' etc."),
    a_skip_phase(&g_phase_options, "skip-phase", '\0', "Skip the named phases"),
    a_abort_at_phase(&g_phase_options, "abort-at-phase", '\0', "Abort when a named phase is encountered"),
    a_skip_until_phase(&g_phase_options, "skip-until-phase", '\0', "Skip every phase until a named phase is encountered"),
    a_change_phases_for(&g_phase_options, "change-phases-for", '\0',
            "Control to which package or packages these phase options apply",
            args::EnumArg::EnumArgOptions
            ("all",                        "All packages")
            ("first",                      "Only the first package on the list")
            ("last",                       "Only the last package on the list"),
            "all")
{
}

ResolveCommandLineProgramOptions::ResolveCommandLineProgramOptions(args::ArgsHandler * const h) :
    ArgsSection(h, "Program Options"),
    g_program_options(this, "Program Options", "Options controlling which programs are used to carry out "
            "various tasks. Any replacement to the standard program must provide exactly the same interface. "
            "In all cases, $CAVE can be used to get the path of the main 'cave' executable. Note that unless "
            "an option is explicitly specified, an internal implementation of the default command might be used "
            "instead of spawning a new process."),
    a_display_resolution_program(&g_program_options, "display-resolution-program", '\0', "The program used to display "
            "the resolution. Defaults to '$CAVE display-resolution'."),
    a_execute_resolution_program(&g_program_options, "execute-resolution-program", '\0', "The program used to execute "
            "the resolution. Defaults to '$CAVE execute-resolution'."),
    a_perform_program(&g_program_options, "perform-program", '\0', "The program used to perform "
            "actions. Defaults to '$CAVE perform'."),
    a_update_world_program(&g_program_options, "update-world-program", '\0', "The program used to perform "
            "world updates. Defaults to '$CAVE update-world'.")
{
}

ResolveCommandLineImportOptions::ResolveCommandLineImportOptions(args::ArgsHandler * const h) :
    ArgsSection(h, "Import Options"),
    g_import_options(this, "Import Options", "Options controlling additional imported packages. These options "
            "should not be specified manually; they are for use by 'cave import'."),
    a_unpackaged_repository_params(&g_import_options, "unpackaged-repository-params", '\0', "Specifies "
            "the parameters used to construct an unpackaged repository, for use by 'cave import'.")
{
}

void
ResolveCommandLineImportOptions::apply(const std::tr1::shared_ptr<Environment> & env) const
{
    if (! a_unpackaged_repository_params.specified())
        return;

    std::tr1::shared_ptr<Map<std::string, std::string> > keys(new Map<std::string, std::string>);
    for (args::StringSetArg::ConstIterator a(a_unpackaged_repository_params.begin_args()),
            a_end(a_unpackaged_repository_params.end_args()) ;
            a != a_end ; ++a)
    {
        std::string::size_type p(a->find('='));
        if (std::string::npos == p)
            throw InternalError(PALUDIS_HERE, "no = in '" + stringify(*a));
        keys->insert(a->substr(0, p), a->substr(p + 1));
    }

    std::tr1::shared_ptr<Repository> repo(RepositoryFactory::get_instance()->create(env.get(),
                std::tr1::bind(from_keys, keys, std::tr1::placeholders::_1)));
    env->package_database()->add_repository(10, repo);
}

void
ResolveCommandLineResolutionOptions::apply_shortcuts()
{
    if (a_lazy.specified() +
            a_complete.specified() +
            a_everything.specified() > 1)
        throw args::DoHelp("At most one of '--" + a_lazy.long_name() + "', '--" + a_complete.long_name()
                + "' or '--" + a_everything.long_name() + "' may be specified");

    if (a_lazy.specified())
    {
        if (! a_target_slots.specified())
            a_target_slots.set_argument("best");
        if (! a_slots.specified())
            a_slots.set_argument("best");
        if (! a_no_follow_installed_dependencies.specified())
            a_no_follow_installed_dependencies.set_specified(true);
    }

    if (a_complete.specified())
    {
        if (! a_keep.specified())
            a_keep.set_argument("if-same");
        if (! a_target_slots.specified())
            a_target_slots.set_argument("all");
        if (! a_slots.specified())
            a_slots.set_argument("all");
        if (! a_follow_installed_build_dependencies.specified())
            a_follow_installed_build_dependencies.set_specified(true);
        if (! a_reinstall_scm.specified())
            a_reinstall_scm.set_argument("weekly");
    }

    if (a_everything.specified())
    {
        if (! a_keep.specified())
            a_keep.set_argument("if-transient");
        if (! a_keep_targets.specified())
            a_keep_targets.set_argument("if-transient");
        if (! a_target_slots.specified())
            a_target_slots.set_argument("all");
        if (! a_slots.specified())
            a_slots.set_argument("all");
        if (! a_follow_installed_build_dependencies.specified())
            a_follow_installed_build_dependencies.set_specified(true);
    }
}

void
ResolveCommandLineResolutionOptions::verify(const std::tr1::shared_ptr<const Environment> &)
{
}

