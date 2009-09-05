/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ciaran McCreesh
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

#include <paludis/resolver/resolution.hh>
#include <paludis/resolver/constraint.hh>
#include <paludis/resolver/arrow.hh>
#include <paludis/resolver/decision.hh>
#include <paludis/resolver/serialise-impl.hh>
#include <paludis/resolver/destinations.hh>
#include <paludis/util/sequence.hh>
#include <paludis/util/join.hh>
#include <paludis/util/wrapped_forward_iterator.hh>
#include <paludis/util/indirect_iterator-impl.hh>
#include <paludis/util/make_named_values.hh>
#include <paludis/util/make_shared_ptr.hh>
#include <sstream>

using namespace paludis;
using namespace paludis::resolver;

std::ostream &
paludis::resolver::operator<< (std::ostream & s, const Resolution & r)
{
    std::stringstream ss;
    ss <<  "Resolution("
        << "constraints: " << join(indirect_iterator(r.constraints()->begin()), indirect_iterator(r.constraints()->end()), ", ")
        << "; decision: " << (r.decision() ? stringify(*r.decision()) : "none")
        << "; arrows: " << join(indirect_iterator(r.arrows()->begin()), indirect_iterator(r.arrows()->end()), ", ")
        << "; already_ordered: " << stringify(r.already_ordered()) << ")"
        << "; destinations: " << (r.destinations() ? stringify(*r.destinations()) : "unknown")
        << ")";
    s << ss.str();
    return s;
}

void
Resolution::serialise(Serialiser & s) const
{
    s.object("Resolution")
        .member(SerialiserFlags<>(), "already_ordered", already_ordered())
        .member(SerialiserFlags<serialise::container, serialise::might_be_null>(), "arrows", arrows())
        .member(SerialiserFlags<>(), "constraints", *constraints())
        .member(SerialiserFlags<serialise::might_be_null>(), "decision", decision())
        .member(SerialiserFlags<serialise::might_be_null>(), "destinations", destinations())
        .member(SerialiserFlags<>(), "qpn_s", qpn_s())
        ;
}

const std::tr1::shared_ptr<Resolution>
Resolution::deserialise(Deserialisation & d)
{
    Deserialisator v(d, "Resolution");

    std::tr1::shared_ptr<ArrowSequence> arrows(new ArrowSequence);
    Deserialisator vv(*v.find_remove_member("arrows"), "c");
    for (int n(1), n_end(vv.member<int>("count") + 1) ; n != n_end ; ++n)
        arrows->push_back(vv.member<std::tr1::shared_ptr<Arrow> >(stringify(n)));

    return make_shared_ptr(new Resolution(make_named_values<Resolution>(
                    value_for<n::already_ordered>(v.member<bool>("already_ordered")),
                    value_for<n::arrows>(arrows),
                    value_for<n::constraints>(v.member<std::tr1::shared_ptr<Constraints> >("constraints")),
                    value_for<n::decision>(v.member<std::tr1::shared_ptr<Decision> >("decision")),
                    value_for<n::destinations>(v.member<std::tr1::shared_ptr<Destinations> >("destinations")),
                    value_for<n::qpn_s>(v.member<QPN_S>("qpn_s")),
                    value_for<n::sanitised_dependencies>(make_null_shared_ptr())
                    )));
}
