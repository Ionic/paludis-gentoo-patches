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

#ifndef PALUDIS_GUARD_PALUDIS_STRINGIFY_FORMATTER_IMPL_HH
#define PALUDIS_GUARD_PALUDIS_STRINGIFY_FORMATTER_IMPL_HH 1

#include <paludis/stringify_formatter.hh>
#include <paludis/util/tr1_type_traits.hh>
#include <paludis/util/private_implementation_pattern-impl.hh>

/** \file
 * Implementation for paludis/stringify_formatter.hh .
 *
 * \ingroup g_formatters
 */

namespace paludis
{
    /**
     * Implementation data for StringifyFormatter.
     *
     * \ingroup g_formatters
     * \since 0.26
     * \nosubgrouping
     */
    template <>
    struct Implementation<StringifyFormatter>
    {
        const CanFormat<std::string> * const f_str;
        const CanFormat<IUseFlag> * const f_iuse;
        const CanFormat<UseFlagName> * const f_use;
        const CanFormat<KeywordName> * const f_keyword;
        const CanFormat<PackageDepSpec> * const f_package;
        const CanFormat<BlockDepSpec> * const f_block;
        const CanFormat<FetchableURIDepSpec> * const f_f_uri;
        const CanFormat<SimpleURIDepSpec> * const f_s_uri;
        const CanFormat<LicenseDepSpec> * const f_license;
        const CanFormat<DependencyLabelsDepSpec> * const f_dep_label;
        const CanFormat<URILabelsDepSpec> * const f_uri_label;
        const CanFormat<PlainTextDepSpec> * const f_plain;
        const CanFormat<UseDepSpec> * const f_use_dep;
        const CanFormat<NamedSetDepSpec> * const f_named;
        const CanSpace * const f_space;

        Implementation(
                const CanFormat<std::string> * const f_str_v,
                const CanFormat<IUseFlag> * const f_iuse_v,
                const CanFormat<UseFlagName> * const f_use_v,
                const CanFormat<KeywordName> * const f_keyword_v,
                const CanFormat<PackageDepSpec> * const f_package_v,
                const CanFormat<BlockDepSpec> * const f_block_v,
                const CanFormat<FetchableURIDepSpec> * const f_f_uri_v,
                const CanFormat<SimpleURIDepSpec> * const f_s_uri_v,
                const CanFormat<LicenseDepSpec> * const f_license_v,
                const CanFormat<DependencyLabelsDepSpec> * const f_dep_label_v,
                const CanFormat<URILabelsDepSpec> * const f_uri_label_v,
                const CanFormat<PlainTextDepSpec> * const f_plain_v,
                const CanFormat<UseDepSpec> * const f_use_dep_v,
                const CanFormat<NamedSetDepSpec> * const f_named_v,
                const CanSpace * const f_space_v
                ) :
            f_str(f_str_v),
            f_iuse(f_iuse_v),
            f_use(f_use_v),
            f_keyword(f_keyword_v),
            f_package(f_package_v),
            f_block(f_block_v),
            f_f_uri(f_f_uri_v),
            f_s_uri(f_s_uri_v),
            f_license(f_license_v),
            f_dep_label(f_dep_label_v),
            f_uri_label(f_uri_label_v),
            f_plain(f_plain_v),
            f_use_dep(f_use_dep_v),
            f_named(f_named_v),
            f_space(f_space_v)
        {
        }
    };

    /**
     * Internal use by StringifyFormatter: get a CanFormat<> interface, or 0 if
     * not implemented.
     *
     * \ingroup g_formatters
     * \nosubgrouping
     * \since 0.26
     */
    template <bool b_, typename T_>
    struct StringifyFormatterGetForwarder
    {
        /**
         * Get a CanFormat<> interface, or 0 if not implemented.
         */
        static const CanFormat<T_> * get(const CanFormat<T_> * const t)
        {
            return t;
        }
    };

    /**
     * Internal use by StringifyFormatter: get a CanFormat<> interface, or 0 if
     * not implemented.
     *
     * \ingroup g_formatters
     * \nosubgrouping
     * \since 0.26
     */
    template <typename T_>
    struct StringifyFormatterGetForwarder<false, T_>
    {
        /**
         * Get a CanFormat<> interface, or 0 if not implemented.
         */
        static const CanFormat<T_> * get(const void * const)
        {
            return 0;
        }
    };

    /**
     * Internal use by StringifyFormatter: get a CanSpace<> interface, or 0 if
     * not implemented.
     *
     * \ingroup g_formatters
     * \nosubgrouping
     * \since 0.26
     */
    template <bool b_>
    struct StringifyFormatterGetSpaceForwarder
    {
        /**
         * Get a CanSpace interface, or 0 if not implemented.
         */
        static const CanSpace * get(const CanSpace * const t)
        {
            return t;
        }
    };

    /**
     * Internal use by StringifyFormatter: get a CanSpace<> interface, or 0 if
     * not implemented.
     *
     * \ingroup g_formatters
     * \nosubgrouping
     * \since 0.26
     */
    template <>
    struct StringifyFormatterGetSpaceForwarder<false>
    {
        /**
         * Get a CanSpace interface, or 0 if not implemented.
         */
        static const CanSpace * get(const void * const)
        {
            return 0;
        }
    };

    template <typename T_>
    StringifyFormatter::StringifyFormatter(const T_ & t) :
        PrivateImplementationPattern<StringifyFormatter>(new Implementation<StringifyFormatter>(
                    StringifyFormatterGetForwarder<tr1::is_convertible<T_ *, CanFormat<std::string> *>::value, std::string>::get(&t),
                    StringifyFormatterGetForwarder<tr1::is_convertible<T_ *, CanFormat<IUseFlag> *>::value, IUseFlag>::get(&t),
                    StringifyFormatterGetForwarder<tr1::is_convertible<T_ *, CanFormat<UseFlagName> *>::value, UseFlagName>::get(&t),
                    StringifyFormatterGetForwarder<tr1::is_convertible<T_ *, CanFormat<KeywordName> *>::value, KeywordName>::get(&t),
                    StringifyFormatterGetForwarder<tr1::is_convertible<T_ *, CanFormat<PackageDepSpec> *>::value, PackageDepSpec>::get(&t),
                    StringifyFormatterGetForwarder<tr1::is_convertible<T_ *, CanFormat<BlockDepSpec> *>::value, BlockDepSpec>::get(&t),
                    StringifyFormatterGetForwarder<
                        tr1::is_convertible<T_ *, CanFormat<FetchableURIDepSpec> *>::value,
                        FetchableURIDepSpec>::get(&t),
                    StringifyFormatterGetForwarder<
                        tr1::is_convertible<T_ *, CanFormat<SimpleURIDepSpec> *>::value,
                        SimpleURIDepSpec>::get(&t),
                    StringifyFormatterGetForwarder<
                        tr1::is_convertible<T_ *, CanFormat<LicenseDepSpec> *>::value,
                        LicenseDepSpec>::get(&t),
                    StringifyFormatterGetForwarder<
                        tr1::is_convertible<T_ *, CanFormat<DependencyLabelsDepSpec> *>::value,
                        DependencyLabelsDepSpec>::get(&t),
                    StringifyFormatterGetForwarder<
                        tr1::is_convertible<T_ *, CanFormat<URILabelsDepSpec> *>::value,
                        URILabelsDepSpec>::get(&t),
                    StringifyFormatterGetForwarder<
                        tr1::is_convertible<T_ *, CanFormat<PlainTextDepSpec> *>::value,
                        PlainTextDepSpec>::get(&t),
                    StringifyFormatterGetForwarder<tr1::is_convertible<T_ *, CanFormat<UseDepSpec> *>::value, UseDepSpec>::get(&t),
                    StringifyFormatterGetForwarder<tr1::is_convertible<T_ *, CanFormat<NamedSetDepSpec> *>::value, NamedSetDepSpec>::get(&t),
                    StringifyFormatterGetSpaceForwarder<tr1::is_convertible<T_ *, CanSpace *>::value>::get(&t)
                    )),
        CanSpace()
    {
    }
}

#endif
