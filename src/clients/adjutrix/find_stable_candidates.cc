/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006, 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
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

#include "find_stable_candidates.hh"
#include "command_line.hh"

#include <paludis/util/tokeniser.hh>
#include <paludis/util/iterator.hh>
#include <paludis/util/sequence.hh>
#include <paludis/util/set.hh>
#include <paludis/version_spec.hh>
#include <paludis/repository.hh>
#include <paludis/package_database.hh>
#include <paludis/package_id.hh>
#include <paludis/metadata_key.hh>

#include <set>
#include <map>
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace paludis;
using std::cout;
using std::cerr;
using std::endl;

namespace
{

#include "find_stable_candidates-sr.hh"
#include "find_stable_candidates-sr.cc"

    static const int col_width_package      = 30;
    static const int col_width_our_version  = 20;
    static const int col_width_best_version = 20;

    void
    write_repository_header(const KeywordName & keyword, const RepositoryName & repo)
    {
        std::string s("Stable candidates for '" + stringify(repo) + "' on '"
                + stringify(keyword) + "'");
        cout << std::string(s.length(), '=') << endl;
        cout << s << endl;
        cout << std::string(s.length(), '=') << endl;
        cout << endl;

        cout << std::left
            << std::setw(col_width_package) << "category/package (:slot)"
            << std::setw(col_width_our_version) << "our version"
            << std::setw(col_width_best_version) << "best version"
            << endl;

        cout
            << std::string(col_width_package - 1, '-') << " "
            << std::string(col_width_our_version - 1, '-') << " "
            << std::string(col_width_best_version - 1, '-') << " "
            << endl;
    }

    struct IsStableKeyword
    {
        bool operator() (const KeywordName & k) const
        {
            return std::string::npos == std::string("-~").find(k.data().at(0));
        }
    };

    void
    write_package(const QualifiedPackageName & package, const SlotName & slot,
            const VersionSpec & our_version, const VersionSpec & best_version)
    {
        static CategoryNamePart previous_category("not-on-a-boat");
        if (package.category != previous_category)
        {
            cout << std::setw(col_width_package) << (stringify(package.category) + "/") << endl;
            previous_category = package.category;
        }

        std::string p(stringify(package.package));
        if (SlotName("0") != slot)
            p += ":" + stringify(slot);
        cout << "  " << std::setw(col_width_package - 2) << p;

        if (our_version != VersionSpec("0"))
            cout << std::setw(col_width_our_version) << our_version;
        else
            cout << std::setw(col_width_our_version) << " ";
        cout << std::setw(col_width_best_version) << best_version;
        cout << endl;
    }

    void
    check_one_package(const Environment &, const KeywordName & keyword,
            const Repository & repo, const QualifiedPackageName & package)
    {
        /* determine whether we have any interesting versions, and pick out
         * slots where we do. for slots, we map slot to a pair (best stable
         * version for us, best stable version for anyone). */

        bool is_interesting(false);
        typedef std::map<SlotName, SlotsEntry> SlotsToVersions;
        SlotsToVersions slots_to_versions;

        tr1::shared_ptr<const PackageIDSequence> versions(repo.package_ids(package));
        for (PackageIDSequence::Iterator v(versions->begin()), v_end(versions->end()) ;
                v != v_end ; ++v)
        {
            if (! (*v)->keywords_key())
                continue;

            if ((*v)->keywords_key()->value()->end() != (*v)->keywords_key()->value()->find(keyword))
            {
                is_interesting = true;

                /* replace the entry */
                slots_to_versions.erase((*v)->slot());
                slots_to_versions.insert(std::make_pair((*v)->slot(),
                            SlotsEntry(SlotsEntry::create()
                                .our_version((*v)->version())
                                .best_version(VersionSpec("0")))));
            }

            if ((*v)->keywords_key()->value()->end() != std::find_if((*v)->keywords_key()->value()->begin(),
                        (*v)->keywords_key()->value()->end(), IsStableKeyword()))
            {
                /* ensure that an entry exists */
                slots_to_versions.insert(std::make_pair((*v)->slot(),
                            SlotsEntry(SlotsEntry::create()
                                .our_version(VersionSpec("0"))
                                .best_version((*v)->version()))));

                /* update the entry to mark our current version as the best
                 * version */
                if (slots_to_versions.find((*v)->slot())->second.best_version <= (*v)->version())
                    slots_to_versions.find((*v)->slot())->second.best_version = (*v)->version();
            }
        }

        if (! is_interesting)
            return;

        /* for each slot, if there's a higher stable version on another arch, flag it */
        for (SlotsToVersions::const_iterator s(slots_to_versions.begin()),
                s_end(slots_to_versions.end()) ; s != s_end ; ++s)
        {
            if (s->second.our_version >= s->second.best_version)
                continue;

            write_package(package, s->first, s->second.our_version, s->second.best_version);
        }
    }
}

void do_find_stable_candidates(const NoConfigEnvironment & env)
{
    Context context("When performing find-stable-candidates action:");

    KeywordName keyword(*CommandLine::get_instance()->begin_parameters());

    for (IndirectIterator<PackageDatabase::RepositoryIterator, const Repository>
            r(env.package_database()->begin_repositories()),
            r_end(env.package_database()->end_repositories()) ; r != r_end ; ++r)
    {
        if (r->name() == RepositoryName("virtuals"))
            continue;
        if (env.master_repository() && r->name() == env.master_repository()->name())
            continue;

        write_repository_header(keyword, r->name());

        tr1::shared_ptr<const CategoryNamePartSet> cat_names(r->category_names());
        for (CategoryNamePartSet::Iterator c(cat_names->begin()), c_end(cat_names->end()) ;
                c != c_end ; ++c)
        {
            if (CommandLine::get_instance()->a_category.specified())
                if (CommandLine::get_instance()->a_category.end_args() == std::find(
                            CommandLine::get_instance()->a_category.begin_args(),
                            CommandLine::get_instance()->a_category.end_args(),
                            stringify(*c)))
                    continue;

            tr1::shared_ptr<const QualifiedPackageNameSet> pkg_names(r->package_names(*c));
            for (QualifiedPackageNameSet::Iterator p(pkg_names->begin()), p_end(pkg_names->end()) ;
                    p != p_end ; ++p)
            {
                if (CommandLine::get_instance()->a_package.specified())
                    if (CommandLine::get_instance()->a_package.end_args() == std::find(
                                CommandLine::get_instance()->a_package.begin_args(),
                                CommandLine::get_instance()->a_package.end_args(),
                                stringify(p->package)))
                        continue;

                check_one_package(env, keyword, *r, *p);
            }
        }
    }
}

