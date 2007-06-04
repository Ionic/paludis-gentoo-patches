/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
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

#include <test/test_runner.hh>
#include <test/test_framework.hh>
#include <paludis/repositories/gems/gem_specification.hh>
#include <paludis/repositories/gems/yaml.hh>

using namespace test;
using namespace paludis;
using namespace paludis::gems;

namespace test_cases
{
    struct SpecificationTest : TestCase
    {
        SpecificationTest() : TestCase("gem specification") { }

        void run()
        {
            std::string spec_text(
                "--- !ruby/object:Gem::Specification\n"
                "name: demo\n"
                "version: !ruby/object:Gem::Version\n"
                "  version: 1.2.3\n"
                "summary: This is the summary\n"
                "homepage:\n"
                "rubyforge_project:\n"
                "description: A longer description\n"
                "platform: ruby\n"
                "date: 1234\n"
                "authors: [ Fred , Barney ]\n"
                );

            yaml::Document spec_doc(spec_text);
            TEST_CHECK(spec_doc.top());

            GemSpecification spec(*spec_doc.top());
            TEST_CHECK_EQUAL(spec.summary(), "This is the summary");
            TEST_CHECK_EQUAL(spec.name(), "demo");
            TEST_CHECK_EQUAL(spec.version(), "1.2.3");
            TEST_CHECK_EQUAL(spec.homepage(), "");
            TEST_CHECK_EQUAL(spec.rubyforge_project(), "");
            TEST_CHECK_EQUAL(spec.description(), "A longer description");
            TEST_CHECK_EQUAL(spec.authors(), "Fred, Barney");
            TEST_CHECK_EQUAL(spec.date(), "1234");
        }
    } test_specification;
}

