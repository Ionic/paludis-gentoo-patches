/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006 Ciaran McCreesh <ciaran.mccreesh@blueyonder.co.uk>
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

#include <paludis/dep_atom.hh>
#include <paludis/portage_dep_parser.hh>
#include <paludis/qa/pdepend_overlap_check.hh>
#include <paludis/util/join.hh>
#include <set>

using namespace paludis;
using namespace paludis::qa;

namespace
{
    struct Collector :
        DepAtomVisitorTypes::ConstVisitor
    {
        std::set<QualifiedPackageName> result;

        Collector()
        {
        }

        void visit(const PackageDepAtom * const p)
        {
            result.insert(p->package());
        }

        void visit(const AllDepAtom * const a)
        {
            std::for_each(a->begin(), a->end(), accept_visitor(this));
        }

        void visit(const AnyDepAtom * const a)
        {
            std::for_each(a->begin(), a->end(), accept_visitor(this));
        }

        void visit(const UseDepAtom * const u)
        {
            std::for_each(u->begin(), u->end(), accept_visitor(this));
        }

        void visit(const BlockDepAtom * const)
        {
        }

        void visit(const PlainTextDepAtom * const)
        {
        }
    };
}

PdependOverlapCheck::PdependOverlapCheck()
{
}

CheckResult
PdependOverlapCheck::operator() (const EbuildCheckData & e) const
{
    CheckResult result(stringify(e.name) + "-" + stringify(e.version),
            identifier());

    try
    {
        PackageDatabaseEntry ee(e.name, e.version,
                e.environment->package_database()->favourite_repository());
        VersionMetadata::ConstPointer metadata(
                e.environment->package_database()->fetch_repository(ee.repository)->version_metadata(ee.name, ee.version));

        Collector pdepend_collector;
        std::string pdepend(metadata->deps.post_depend_string);
        PortageDepParser::parse(pdepend)->accept(&pdepend_collector);

        {
            Collector depend_collector;
            std::string depend(metadata->deps.build_depend_string);
            PortageDepParser::parse(depend)->accept(&depend_collector);

            std::set<QualifiedPackageName> overlap;
            std::set_intersection(depend_collector.result.begin(), depend_collector.result.end(),
                    pdepend_collector.result.begin(), pdepend_collector.result.end(),
                    std::inserter(overlap, overlap.begin()));

            if (! overlap.empty())
                result << Message(qal_major, "Overlap between DEPEND and PDEPEND: '" +
                        join(overlap.begin(), overlap.end(), "', ") + "'");
        }

        {
            Collector rdepend_collector;
            std::string rdepend(metadata->deps.run_depend_string);
            PortageDepParser::parse(rdepend)->accept(&rdepend_collector);

            std::set<QualifiedPackageName> overlap;
            std::set_intersection(rdepend_collector.result.begin(), rdepend_collector.result.end(),
                    pdepend_collector.result.begin(), pdepend_collector.result.end(),
                    std::inserter(overlap, overlap.begin()));

            if (! overlap.empty())
                result << Message(qal_major, "Overlap between RDEPEND and PDEPEND: '" +
                        join(overlap.begin(), overlap.end(), "', ") + "'");
        }

    }
    catch (const InternalError &)
    {
        throw;
    }
    catch (const Exception & e)
    {
        result << Message(qal_fatal, "Caught Exception '" + e.message() + "' ("
                + e.what() + ")");
    }

    return result;
}

const std::string &
PdependOverlapCheck::identifier()
{
    static const std::string id("pdepend_overlap");
    return id;
}


