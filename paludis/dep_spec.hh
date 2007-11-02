/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006, 2007 Ciaran McCreesh
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

#ifndef PALUDIS_GUARD_PALUDIS_DEP_SPEC_HH
#define PALUDIS_GUARD_PALUDIS_DEP_SPEC_HH 1

#include <paludis/util/attributes.hh>
#include <paludis/util/clone.hh>
#include <paludis/util/instantiation_policy.hh>
#include <paludis/util/private_implementation_pattern.hh>
#include <paludis/util/tr1_memory.hh>
#include <paludis/util/wrapped_forward_iterator-fwd.hh>

#include <paludis/dep_label.hh>
#include <paludis/dep_spec-fwd.hh>
#include <paludis/dep_tag-fwd.hh>
#include <paludis/name.hh>
#include <paludis/version_operator-fwd.hh>
#include <paludis/version_requirements-fwd.hh>
#include <paludis/version_spec-fwd.hh>

/** \file
 * Declarations for dependency spec classes.
 *
 * \ingroup g_dep_spec
 *
 * \section Examples
 *
 * - \ref example_dep_spec.cc "example_dep_spec.cc" (for specifications)
 * - \ref example_dep_label.cc "example_dep_label.cc" (for labels)
 * - \ref example_dep_tree.cc "example_dep_tree.cc" (for specification trees)
 * - \ref example_dep_tag.cc "example_dep_tag.cc" (for tags)
 */

namespace paludis
{
    /**
     * Base class for a dependency spec.
     *
     * \ingroup g_dep_spec
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE DepSpec :
        private InstantiationPolicy<DepSpec, instantiation_method::NonCopyableTag>,
        public virtual Cloneable<DepSpec>
    {
        protected:
            DepSpec();

        public:
            ///\name Basic operations
            ///\{

            virtual ~DepSpec();

            ///\}

            ///\name Upcasts
            ///\{

            /**
             * Return us as a UseDepSpec, or 0 if we are not a
             * UseDepSpec.
             */
            virtual const UseDepSpec * as_use_dep_spec() const
                PALUDIS_ATTRIBUTE((warn_unused_result));

            /**
             * Return us as a PackageDepSpec, or 0 if we are not a
             * UseDepSpec.
             */
            virtual const PackageDepSpec * as_package_dep_spec() const
                PALUDIS_ATTRIBUTE((warn_unused_result));

            ///\}
    };

    /**
     * Represents a "|| ( )" dependency block.
     *
     * \ingroup g_dep_spec
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE AnyDepSpec :
        public DepSpec
    {
        public:
            ///\name Basic operations
            ///\{

            AnyDepSpec();

            ///\}

            virtual tr1::shared_ptr<DepSpec> clone() const PALUDIS_ATTRIBUTE((warn_unused_result));
    };

    /**
     * Represents a ( first second third ) or top level group of dependency
     * specs.
     *
     * \ingroup g_dep_spec
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE AllDepSpec :
        public DepSpec
    {
        public:
            ///\name Basic operations
            ///\{

            AllDepSpec();

            ///\}

            virtual tr1::shared_ptr<DepSpec> clone() const PALUDIS_ATTRIBUTE((warn_unused_result));
    };

    /**
     * Represents a use? ( ) dependency spec.
     *
     * \ingroup g_dep_spec
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE UseDepSpec :
        public DepSpec
    {
        private:
            const UseFlagName _flag;
            const bool _inverse;

        public:
            ///\name Basic operations
            ///\{

            UseDepSpec(const UseFlagName &, bool);

            ///\}

            /**
             * Fetch our use flag name.
             */
            UseFlagName flag() const;

            /**
             * Fetch whether we are a ! flag.
             */
            bool inverse() const;

            virtual const UseDepSpec * as_use_dep_spec() const;

            virtual tr1::shared_ptr<DepSpec> clone() const PALUDIS_ATTRIBUTE((warn_unused_result));
    };

    /**
     * A StringDepSpec represents a dep spec with an associated piece of text.
     *
     * \ingroup g_dep_spec
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE StringDepSpec :
        public DepSpec
    {
        private:
            std::string _str;

        protected:
            ///\name Basic operations
            ///\{

            StringDepSpec(const std::string &);

            ~StringDepSpec();

            ///\}

            /**
             * Change our text.
             */
            void set_text(const std::string &);

        public:
            /**
             * Fetch our text.
             */
            std::string text() const;
    };

    /**
     * A selection of USE flag requirements.
     *
     * \ingroup g_dep_spec
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE UseRequirements :
        private PrivateImplementationPattern<UseRequirements>
    {
        public:
            ///\name Basic operations
            ///\{

            UseRequirements();
            UseRequirements(const UseRequirements &);
            ~UseRequirements();

            ///\}

            ///\name Iterate over our USE requirements
            ///\{

            typedef WrappedForwardIterator<enum ConstIteratorTag { },
                    const std::pair<const UseFlagName, UseFlagState> > ConstIterator;

            ConstIterator begin() const;
            ConstIterator end() const;

            ///\}

            /// Find the requirement for a particular USE flag.
            ConstIterator find(const UseFlagName & u) const
                PALUDIS_ATTRIBUTE((warn_unused_result));

            /// Insert a new requirement.
            bool insert(const UseFlagName & u, UseFlagState s)
                PALUDIS_ATTRIBUTE((warn_unused_result));

            /// What state is desired for a particular use flag?
            UseFlagState state(const UseFlagName &) const
                PALUDIS_ATTRIBUTE((warn_unused_result));

            /// Are we empty?
            bool empty() const
                PALUDIS_ATTRIBUTE((warn_unused_result));
    };

    /**
     * A PackageDepSpec represents a package name (for example,
     * 'app-editors/vim'), possibly with associated version and SLOT
     * restrictions.
     *
     * \ingroup g_dep_spec
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE PackageDepSpec :
        public StringDepSpec,
        private PrivateImplementationPattern<PackageDepSpec>
    {
        private:
            const PackageDepSpec & operator= (const PackageDepSpec &);

            void _do_parse(const std::string &, const PackageDepSpecParseMode);
            void _make_unique();

        public:
            ///\name Basic operations
            ///\{

            /**
             * Constructor, no version or SLOT restrictions.
             *
             * \deprecated Use the two arg form.
             */
            PackageDepSpec(const QualifiedPackageName & package) PALUDIS_ATTRIBUTE((deprecated));

            /**
             * Constructor, parse restrictions ourself.
             *
             * \deprecated Use the two arg form.
             */
            explicit PackageDepSpec(const std::string &) PALUDIS_ATTRIBUTE((deprecated));

            PackageDepSpec(const std::string &, const PackageDepSpecParseMode);

            explicit PackageDepSpec(
                tr1::shared_ptr<QualifiedPackageName> q = tr1::shared_ptr<QualifiedPackageName>(),
                tr1::shared_ptr<CategoryNamePart> c = tr1::shared_ptr<CategoryNamePart>(),
                tr1::shared_ptr<PackageNamePart> p = tr1::shared_ptr<PackageNamePart>(),
                tr1::shared_ptr<VersionRequirements> v = tr1::shared_ptr<VersionRequirements>(),
                VersionRequirementsMode m = vr_and,
                tr1::shared_ptr<SlotName> s = tr1::shared_ptr<SlotName>(),
                tr1::shared_ptr<RepositoryName> r = tr1::shared_ptr<RepositoryName>(),
                tr1::shared_ptr<UseRequirements> u = tr1::shared_ptr<UseRequirements>(),
                tr1::shared_ptr<const DepTag> t = tr1::shared_ptr<const DepTag>());

            PackageDepSpec(const PackageDepSpec &);

            ~PackageDepSpec();

            ///\}

            /**
             * Fetch the package name (may be a zero pointer).
             */
            tr1::shared_ptr<const QualifiedPackageName> package_ptr() const;

            /**
             * Fetch the package name part, if wildcarded, or a zero pointer otherwise.
             */
            tr1::shared_ptr<const PackageNamePart> package_name_part_ptr() const;

            /**
             * Fetch the category name part, if wildcarded, or a zero pointer otherwise.
             */
            tr1::shared_ptr<const CategoryNamePart> category_name_part_ptr() const;

            /**
             * Fetch the version requirements (may be a zero pointer).
             */
            tr1::shared_ptr<const VersionRequirements> version_requirements_ptr() const;

            /**
             * Fetch the version requirements (may be a zero pointer).
             */
            tr1::shared_ptr<VersionRequirements> version_requirements_ptr();

            /**
             * Fetch the version requirements mode.
             */
            VersionRequirementsMode version_requirements_mode() const;

            /**
             * Set the version requirements mode.
             */
            void set_version_requirements_mode(const VersionRequirementsMode m);

            /**
             * Fetch the slot name (may be a zero pointer).
             */
            tr1::shared_ptr<const SlotName> slot_ptr() const;

            /**
             * Fetch the repo name (may be a zero pointer).
             */
            tr1::shared_ptr<const RepositoryName> repository_ptr() const;

            /**
             * Fetch the use requirements (may be a zero pointer).
             */
            tr1::shared_ptr<const UseRequirements> use_requirements_ptr() const;

            /**
             * Fetch our tag.
             */
            tr1::shared_ptr<const DepTag> tag() const;

            /**
             * Set our tag.
             */
            void set_tag(const tr1::shared_ptr<const DepTag> & s);

            /**
             * Fetch a copy of ourself without the USE requirements.
             */
            tr1::shared_ptr<PackageDepSpec> without_use_requirements() const;

            virtual const PackageDepSpec * as_package_dep_spec() const;

            virtual tr1::shared_ptr<DepSpec> clone() const PALUDIS_ATTRIBUTE((warn_unused_result));
    };

    /**
     * A PlainTextDepSpec represents a plain text entry.
     *
     * \ingroup g_dep_spec
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE PlainTextDepSpec :
        public StringDepSpec
    {
        public:
            ///\name Basic operations
            ///\{

            PlainTextDepSpec(const std::string &);

            ///\}

            virtual tr1::shared_ptr<DepSpec> clone() const PALUDIS_ATTRIBUTE((warn_unused_result));
    };

    /**
     * A NamedSetDepSpec represents a named package set.
     *
     * \ingroup g_dep_spec
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE NamedSetDepSpec :
        public StringDepSpec
    {
        private:
            const SetName _name;

        public:
            ///\name Basic operations
            ///\{

            NamedSetDepSpec(const SetName &);

            ///\}

            /// Fetch the name of our set.
            const SetName name() const PALUDIS_ATTRIBUTE((warn_unused_result));

            virtual tr1::shared_ptr<DepSpec> clone() const PALUDIS_ATTRIBUTE((warn_unused_result));
    };

    /**
     * A LicenseDepSpec represents a license entry.
     *
     * \ingroup g_dep_spec
     * \since 0.26
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE LicenseDepSpec :
        public StringDepSpec
    {
        public:
            ///\name Basic operations
            ///\{

            LicenseDepSpec(const std::string &);

            ///\}

            virtual tr1::shared_ptr<DepSpec> clone() const PALUDIS_ATTRIBUTE((warn_unused_result));
    };

    /**
     * A FetchableURIDepSpec represents a fetchable URI part.
     *
     * It differs from a SimpleURIDepSpec in that it supports arrow notation. Arrows
     * are used by exheres to allow downloading to a filename other than that used by
     * the original URL.
     *
     * \ingroup g_dep_spec
     * \since 0.26
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE FetchableURIDepSpec :
        public StringDepSpec
    {
        public:
            ///\name Basic operations
            ///\{

            FetchableURIDepSpec(const std::string &);

            ///\}

            /**
             * The original URL (that is, the text to the left of the arrow, if present,
             * or the entire text otherwise).
             */
            std::string original_url() const;

            /**
             * The renamed URL filename (that is, the text to the right of the arrow,
             * if present, or an empty string otherwise).
             */
            std::string renamed_url_suffix() const;

            /**
             * The filename (that is, the renamed URL suffix, if present, or the text
             * after the final / in the original URL otherwise).
             */
            std::string filename() const;

            virtual tr1::shared_ptr<DepSpec> clone() const PALUDIS_ATTRIBUTE((warn_unused_result));
    };

    /**
     * A SimpleURIDepSpec represents a simple URI part.
     *
     * Unlike FetchableURIDepSpec, arrow notation is not supported.
     *
     * \ingroup g_dep_spec
     * \since 0.26
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE SimpleURIDepSpec :
        public StringDepSpec
    {
        public:
            ///\name Basic operations
            ///\{

            SimpleURIDepSpec(const std::string &);

            ///\}

            virtual tr1::shared_ptr<DepSpec> clone() const PALUDIS_ATTRIBUTE((warn_unused_result));
    };

    /**
     * Thrown if an invalid package dep spec specification is encountered.
     *
     * \ingroup g_exceptions
     * \ingroup g_dep_spec
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE PackageDepSpecError :
        public Exception
    {
        public:
            ///\name Basic operations
            ///\{

            PackageDepSpecError(const std::string & msg) throw ();

            ///\}
    };

    /**
     * A BlockDepSpec represents a block on a package name (for example,
     * 'app-editors/vim'), possibly with associated version and SLOT
     * restrictions.
     *
     * \ingroup g_dep_spec
     * \nosubgrouping
     */
    class PALUDIS_VISIBLE BlockDepSpec :
        public StringDepSpec
    {
        private:
            tr1::shared_ptr<const PackageDepSpec> _spec;

        public:
            ///\name Basic operations
            ///\{

            BlockDepSpec(tr1::shared_ptr<const PackageDepSpec> spec);

            ///\}

            /**
             * Fetch the spec we're blocking.
             */
            tr1::shared_ptr<const PackageDepSpec> blocked_spec() const;

            virtual tr1::shared_ptr<DepSpec> clone() const PALUDIS_ATTRIBUTE((warn_unused_result));
    };

    /**
     * A LabelsDepSpec represents a labels entry using a particular visitor
     * types class.
     *
     * \see DependencyLabelsDepSpec
     * \see URILabelsDepSpec
     * \since 0.26
     * \ingroup g_dep_spec
     * \nosubgrouping
     */
    template <typename SpecTree_>
    class PALUDIS_VISIBLE LabelsDepSpec :
        public DepSpec,
        private PrivateImplementationPattern<LabelsDepSpec<SpecTree_> >
    {
        private:
            using PrivateImplementationPattern<LabelsDepSpec<SpecTree_> >::_imp;

        public:
            ///\name Basic operations
            ///\{

            LabelsDepSpec();
            ~LabelsDepSpec();

            ///\}

            ///\name Contained labels
            ///\{

            void add_label(const tr1::shared_ptr<const typename SpecTree_::BasicNode> &);
            typedef WrappedForwardIterator<enum ConstIteratorTag { },
                    const tr1::shared_ptr<const typename SpecTree_::BasicNode> > ConstIterator;
            ConstIterator begin() const PALUDIS_ATTRIBUTE((warn_unused_result));
            ConstIterator end() const PALUDIS_ATTRIBUTE((warn_unused_result));

            ///\}

            virtual tr1::shared_ptr<DepSpec> clone() const PALUDIS_ATTRIBUTE((warn_unused_result));
    };

}

#endif
