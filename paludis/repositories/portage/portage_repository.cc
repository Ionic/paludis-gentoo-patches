/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006 Ciaran McCreesh <ciaran.mccreesh@blueyonder.co.uk>
 * Copyright (c) 2006 Danny van Dyk <kugelfang@gentoo.org>
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

#include "config.h"

#include <paludis/repositories/portage/portage_repository.hh>
#include <paludis/repositories/portage/portage_repository_profile.hh>
#include <paludis/repositories/portage/portage_repository_news.hh>
#include <paludis/repositories/portage/portage_repository_sets.hh>
#include <paludis/repositories/portage/portage_repository_exceptions.hh>
#include <paludis/repositories/portage/portage_repository_ebuild_entries.hh>

#include <paludis/config_file.hh>
#include <paludis/dep_atom.hh>
#include <paludis/dep_atom_flattener.hh>
#include <paludis/ebuild.hh>
#include <paludis/environment.hh>
#include <paludis/hashed_containers.hh>
#include <paludis/match_package.hh>
#include <paludis/package_database.hh>
#include <paludis/package_database_entry.hh>
#include <paludis/portage_dep_parser.hh>
#include <paludis/syncer.hh>
#include <paludis/util/collection_concrete.hh>
#include <paludis/util/dir_iterator.hh>
#include <paludis/util/fs_entry.hh>
#include <paludis/util/is_file_with_extension.hh>
#include <paludis/util/iterator.hh>
#include <paludis/util/log.hh>
#include <paludis/util/pstream.hh>
#include <paludis/util/random.hh>
#include <paludis/util/save.hh>
#include <paludis/util/stringify.hh>
#include <paludis/util/strip.hh>
#include <paludis/util/system.hh>
#include <paludis/util/tokeniser.hh>

#include <map>
#include <fstream>
#include <functional>
#include <algorithm>
#include <vector>
#include <limits>

#include <strings.h>
#include <ctype.h>

/** \file
 * Implementation of PortageRepository.
 *
 * \ingroup grpportagerepository
 */

using namespace paludis;

namespace paludis
{
    typedef MakeHashedSet<UseFlagName>::Type UseFlagSet;

    /// Map for versions.
    typedef MakeHashedMap<QualifiedPackageName, VersionSpecCollection::Pointer>::Type VersionsMap;

    /// Map for repository masks.
    typedef MakeHashedMap<QualifiedPackageName, std::list<PackageDepAtom::ConstPointer> >::Type RepositoryMaskMap;

    /// Map for categories.
    typedef MakeHashedMap<CategoryNamePart, bool>::Type CategoryMap;

    /// Map for packages.
    typedef MakeHashedMap<QualifiedPackageName, bool>::Type PackagesMap;

    /// Map for mirrors.
    typedef MakeHashedMultiMap<std::string, std::string>::Type MirrorMap;

    /// Map for metadata.
    typedef MakeHashedMap<std::pair<QualifiedPackageName, VersionSpec>,
            VersionMetadata::Pointer>::Type MetadataMap;

    /// Map for virtuals.
    typedef MakeHashedMap<QualifiedPackageName, PackageDepAtom::ConstPointer>::Type VirtualsMap;

    /**
     * Implementation data for a PortageRepository.
     *
     * \ingroup grpportagerepository
     */
    template <>
    struct Implementation<PortageRepository> :
        InternalCounted<Implementation<PortageRepository> >
    {
        /// Our parameters
        const PortageRepositoryParams params;

        /// Have we loaded our category names?
        mutable bool has_category_names;

        /// Our category names, and whether we have a fully loaded list
        /// of package names for that category.
        mutable CategoryMap category_names;

        /// Our package names, and whether we have a fully loaded list of
        /// version specs for that category.
        mutable PackagesMap package_names;

        /// Our version specs for each package.
        mutable VersionsMap version_specs;

        /// Metadata cache.
        mutable MetadataMap metadata;

        /// Repository mask.
        mutable RepositoryMaskMap repo_mask;

        /// Have repository mask?
        mutable bool has_repo_mask;

        /// Have virtual names?
        mutable bool has_virtuals;

        /// Arch flags
        mutable UseFlagSet arch_list;

        /// Do we have arch_list?
        mutable bool has_arch_list;

        /// Do we have mirrors?
        mutable bool has_mirrors;

        /// Mirrors.
        mutable MirrorMap mirrors;

        /// Constructor.
        Implementation(PortageRepository * const, const PortageRepositoryParams &);

        /// Destructor.
        ~Implementation();

        /// Invalidate our cache.
        void invalidate() const;

        /// (Empty) provides map.
        const std::map<QualifiedPackageName, QualifiedPackageName> provide_map;

        /// Load profiles, if we haven't already.
        inline void need_profiles() const;

        /// Our profile handler.
        mutable PortageRepositoryProfile::Pointer profile_ptr;

        /// Our news handler.
        mutable PortageRepositoryNews::Pointer news_ptr;

        /// Our sets handler.
        mutable PortageRepositorySets::Pointer sets_ptr;

        /// Our metadata handler.
        mutable PortageRepositoryEntries::Pointer entries_ptr;

        /// Our virtuals
        mutable VirtualsMap our_virtuals;

        /// Have we loaded our virtuals?
        bool has_our_virtuals;
    };

    Implementation<PortageRepository>::Implementation(PortageRepository * const repo,
            const PortageRepositoryParams & p) :
        params(p),
        has_category_names(false),
        has_repo_mask(false),
        has_virtuals(false),
        has_arch_list(false),
        has_mirrors(false),
        profile_ptr(0),
        news_ptr(new PortageRepositoryNews(params.get<prpk_environment>(), repo, p)),
        sets_ptr(new PortageRepositorySets(params.get<prpk_environment>(), repo, p)),
        entries_ptr(new PortageRepositoryEbuildEntries(params.get<prpk_environment>(), repo, p)),
        has_our_virtuals(false)
    {
    }

    Implementation<PortageRepository>::~Implementation()
    {
    }

    void
    Implementation<PortageRepository>::need_profiles() const
    {
        if (profile_ptr)
            return;

        profile_ptr.assign(new PortageRepositoryProfile(
                    params.get<prpk_environment>(), *params.get<prpk_profiles>()));
    }

    void
    Implementation<PortageRepository>::invalidate() const
    {
        profile_ptr.zero();
        has_category_names = false;
        category_names.clear();
        package_names.clear();
        version_specs.clear();
        metadata.clear();
        repo_mask.clear();
        has_repo_mask = false;
        has_virtuals = false;
        arch_list.clear();
        has_arch_list = false;
        has_mirrors = false;
        mirrors.clear();
    }
}

PortageRepository::PortageRepository(const PortageRepositoryParams & p) :
    Repository(PortageRepository::fetch_repo_name(stringify(p.get<prpk_location>())),
            RepositoryCapabilities::create((
                    param<repo_mask>(this),
                    param<repo_installable>(this),
                    param<repo_installed>(static_cast<InstalledInterface *>(0)),
                    param<repo_news>(this),
                    param<repo_sets>(this),
                    param<repo_syncable>(this),
                    param<repo_uninstallable>(static_cast<UninstallableInterface *>(0)),
                    param<repo_use>(this),
                    param<repo_world>(static_cast<WorldInterface *>(0)),
                    param<repo_environment_variable>(this),
                    param<repo_mirrors>(this)
                    ))),
    PrivateImplementationPattern<PortageRepository>(new Implementation<PortageRepository>(this, p))
{
    // the info_vars and info_pkgs info is only added on demand, since it's
    // fairly slow to calculate.
    RepositoryInfoSection::Pointer config_info(new RepositoryInfoSection("Configuration information"));

    config_info->add_kv("location", stringify(_imp->params.get<prpk_location>()));
    config_info->add_kv("profiles", join(_imp->params.get<prpk_profiles>()->begin(),
                _imp->params.get<prpk_profiles>()->end(), " "));
    config_info->add_kv("eclassdirs", join(_imp->params.get<prpk_eclassdirs>()->begin(),
                _imp->params.get<prpk_eclassdirs>()->end(), " "));
    config_info->add_kv("cache", stringify(_imp->params.get<prpk_cache>()));
    config_info->add_kv("distdir", stringify(_imp->params.get<prpk_distdir>()));
    config_info->add_kv("securitydir", stringify(_imp->params.get<prpk_securitydir>()));
    config_info->add_kv("setsdir", stringify(_imp->params.get<prpk_setsdir>()));
    config_info->add_kv("newsdir", stringify(_imp->params.get<prpk_newsdir>()));
    config_info->add_kv("format", "portage");
    config_info->add_kv("root", stringify(_imp->params.get<prpk_root>()));
    config_info->add_kv("buildroot", stringify(_imp->params.get<prpk_buildroot>()));
    config_info->add_kv("sync", _imp->params.get<prpk_sync>());
    config_info->add_kv("sync_exclude", _imp->params.get<prpk_sync_exclude>());

    _info->add_section(config_info);
}

PortageRepository::~PortageRepository()
{
}

bool
PortageRepository::do_has_category_named(const CategoryNamePart & c) const
{
    Context context("When checking for category '" + stringify(c) +
            "' in " + stringify(name()) + ":");

    need_category_names();
    return _imp->category_names.end() !=
        _imp->category_names.find(c);
}

bool
PortageRepository::do_has_package_named(const QualifiedPackageName & q) const
{
    Context context("When checking for package '" + stringify(q) + "' in " +
            stringify(name()) + ":");

    need_category_names();

    if (q.get<qpn_category>() == CategoryNamePart("virtual"))
        need_virtual_names();

    CategoryMap::iterator cat_iter(_imp->category_names.find(q.get<qpn_category>()));

    if (_imp->category_names.end() == cat_iter)
        return false;

    if (cat_iter->second)
        return _imp->package_names.find(q) !=
            _imp->package_names.end();
    else
    {
        if (_imp->package_names.find(q) !=
                _imp->package_names.end())
            return true;

        FSEntry fs(_imp->params.get<prpk_location>());
        fs /= stringify(q.get<qpn_category>());
        fs /= stringify(q.get<qpn_package>());
        if (! fs.is_directory())
            return false;
        _imp->package_names.insert(std::make_pair(q, false));
        return true;
    }
}

namespace
{
    /**
     * Filter QualifiedPackageName instances by category.
     *
     * \ingroup grpportagerepository
     */
    struct CategoryFilter :
        std::unary_function<bool, QualifiedPackageName>
    {
        /// Our category.
        CategoryNamePart category;

        /// Constructor.
        CategoryFilter(const CategoryNamePart & c) :
            category(c)
        {
        }

        /// Predicate.
        bool operator() (const QualifiedPackageName & a) const
        {
            return a.get<qpn_category>() == category;
        }
    };
}

CategoryNamePartCollection::ConstPointer
PortageRepository::do_category_names() const
{
    Context context("When fetching category names in " + stringify(name()) + ":");

    need_category_names();

    CategoryNamePartCollection::Pointer result(new CategoryNamePartCollection::Concrete);
    CategoryMap::const_iterator i(_imp->category_names.begin()),
        i_end(_imp->category_names.end());
    for ( ; i != i_end ; ++i)
        result->insert(i->first);
    return result;
}

QualifiedPackageNameCollection::ConstPointer
PortageRepository::do_package_names(const CategoryNamePart & c) const
{
    /* this isn't particularly fast because it isn't called very often. avoid
     * changing the data structures used to make this faster at the expense of
     * slowing down single item queries. */

    Context context("When fetching package names in category '" + stringify(c)
            + "' in " + stringify(name()) + ":");

    need_category_names();
    if (c == CategoryNamePart("virtual"))
        need_virtual_names();

    if (_imp->category_names.end() == _imp->category_names.find(c))
        return QualifiedPackageNameCollection::Pointer(new QualifiedPackageNameCollection::Concrete);

    if ((_imp->params.get<prpk_location>() / stringify(c)).is_directory())
        for (DirIterator d(_imp->params.get<prpk_location>() / stringify(c)), d_end ; d != d_end ; ++d)
        {
            if (! d->is_directory())
                continue;
            if (DirIterator() == std::find_if(DirIterator(*d), DirIterator(),
                        IsFileWithExtension(_imp->entries_ptr->file_extension())))
                continue;

            try
            {
                _imp->package_names.insert(std::make_pair(c + PackageNamePart(d->basename()), false));
            }
            catch (const NameError & e)
            {
                Log::get_instance()->message(ll_warning, lc_context, "Skipping entry '" +
                        d->basename() + "' in category '" + stringify(c) + "' in repository '"
                        + stringify(name()) + "' (" + e.message() + ")");
            }
        }

    _imp->category_names[c] = true;

    QualifiedPackageNameCollection::Pointer result(new QualifiedPackageNameCollection::Concrete);

    std::copy(_imp->package_names.begin(), _imp->package_names.end(),
            transform_inserter(filter_inserter(result->inserter(), CategoryFilter(c)),
                    SelectFirst<const QualifiedPackageName, bool>()));

    return result;
}

VersionSpecCollection::ConstPointer
PortageRepository::do_version_specs(const QualifiedPackageName & n) const
{
    Context context("When fetching versions of '" + stringify(n) + "' in "
            + stringify(name()) + ":");

    if (has_package_named(n))
    {
        need_version_names(n);
        return _imp->version_specs.find(n)->second;
    }
    else
        return VersionSpecCollection::Pointer(new VersionSpecCollection::Concrete);
}

bool
PortageRepository::do_has_version(const QualifiedPackageName & q,
        const VersionSpec & v) const
{
    Context context("When checking for version '" + stringify(v) + "' in '"
            + stringify(q) + "' in " + stringify(name()) + ":");

    if (has_package_named(q))
    {
        need_version_names(q);
        VersionSpecCollection::Pointer vv(_imp->version_specs.find(q)->second);
        return vv->end() != vv->find(v);
    }
    else
        return false;
}

void
PortageRepository::need_category_names() const
{
    if (_imp->has_category_names)
        return;

    Context context("When loading category names for " + stringify(name()) + ":");

    LineConfigFile cats(_imp->params.get<prpk_location>() / "profiles" / "categories");

    for (LineConfigFile::Iterator line(cats.begin()), line_end(cats.end()) ;
            line != line_end ; ++line)
        _imp->category_names.insert(std::make_pair(CategoryNamePart(*line), false));

    _imp->has_category_names = true;
}

void
PortageRepository::need_version_names(const QualifiedPackageName & n) const
{
    if (n.get<qpn_category>() == CategoryNamePart("virtual"))
        need_virtual_names();

    if (_imp->package_names[n])
        return;

    Context context("When loading versions for '" + stringify(n) + "' in "
            + stringify(name()) + ":");

    VersionSpecCollection::Pointer v(new VersionSpecCollection::Concrete);

    FSEntry path(_imp->params.get<prpk_location>() / stringify(n.get<qpn_category>()) /
            stringify(n.get<qpn_package>()));
    if (CategoryNamePart("virtual") == n.get<qpn_category>() && ! path.exists())
    {
        VirtualsMap::iterator i(_imp->our_virtuals.find(n));
        need_version_names(i->second->package());

        VersionSpecCollection::ConstPointer versions(version_specs(i->second->package()));
        for (VersionSpecCollection::Iterator vv(versions->begin()), vv_end(versions->end()) ;
                vv != vv_end ; ++vv)
        {
            PackageDatabaseEntry e(i->second->package(), *vv, name());
            if (! match_package(_imp->params.get<prpk_environment>(), i->second, e))
                continue;

            v->insert(*vv);
        }
    }
    else
    {
        for (DirIterator e(path), e_end ; e != e_end ; ++e)
        {
            if (! IsFileWithExtension(stringify(n.get<qpn_package>()) + "-",
                        _imp->entries_ptr->file_extension())(*e))
                continue;

            try
            {
                v->insert(strip_leading_string(
                            strip_trailing_string(e->basename(), _imp->entries_ptr->file_extension()),
                            stringify(n.get<qpn_package>()) + "-"));
            }
            catch (const NameError &)
            {
                Log::get_instance()->message(ll_warning, lc_context, "Skipping entry '"
                        + stringify(*e) + "' for '" + stringify(n) + "' in repository '"
                        + stringify(name()) + "'");
            }
        }
    }

    _imp->version_specs.insert(std::make_pair(n, v));
    _imp->package_names[n] = true;
}

RepositoryName
PortageRepository::fetch_repo_name(const std::string & location)
{
    try
    {
        do
        {
            FSEntry name_file(location);
            name_file /= "profiles";
            name_file /= "repo_name";

            if (! name_file.is_regular_file())
                break;

            LineConfigFile f(name_file);
            if (f.begin() == f.end())
                break;
            return RepositoryName(*f.begin());

        } while (false);
    }
    catch (...)
    {
    }
    Log::get_instance()->message(ll_qa, lc_no_context, "Couldn't open repo_name file in '"
            + location + "/profiles/'. Falling back to a generated name.");

    std::string modified_location(FSEntry(location).basename());
    std::replace(modified_location.begin(), modified_location.end(), '/', '-');
    return RepositoryName("x-" + modified_location);
}

VersionMetadata::ConstPointer
PortageRepository::do_version_metadata(
        const QualifiedPackageName & q, const VersionSpec & v) const
{
    if (_imp->metadata.end() != _imp->metadata.find(
                std::make_pair(q, v)))
            return _imp->metadata.find(std::make_pair(q, v))->second;

    Context context("When fetching metadata for '" + stringify(q) +
            "-" + stringify(v) + "':");

    if (! has_version(q, v))
        throw NoSuchPackageError(stringify(PackageDatabaseEntry(q, v, name())));

    VersionMetadata::Pointer result(_imp->entries_ptr->generate_version_metadata(q, v));
    _imp->metadata.insert(std::make_pair(std::make_pair(q, v), result));
    return result;
}

bool
PortageRepository::do_query_repository_masks(const QualifiedPackageName & q, const VersionSpec & v) const
{
    if (! _imp->has_repo_mask)
    {
        Context context("When querying repository mask for '" + stringify(q) + "-"
                + stringify(v) + "':");

        FSEntry fff(_imp->params.get<prpk_location>() / "profiles" / "package.mask");
        if (fff.exists())
        {
            LineConfigFile ff(fff);
            for (LineConfigFile::Iterator line(ff.begin()), line_end(ff.end()) ;
                    line != line_end ; ++line)
            {
                PackageDepAtom::ConstPointer a(new PackageDepAtom(*line));
                _imp->repo_mask[a->package()].push_back(a);
            }
        }

        _imp->has_repo_mask = true;
    }

    RepositoryMaskMap::iterator r(_imp->repo_mask.find(q));
    if (_imp->repo_mask.end() == r)
        return false;
    else
        for (IndirectIterator<std::list<PackageDepAtom::ConstPointer>::const_iterator, const PackageDepAtom>
                k(r->second.begin()), k_end(r->second.end()) ; k != k_end ; ++k)
            if (match_package(_imp->params.get<prpk_environment>(), *k, PackageDatabaseEntry(q, v, name())))
                return true;

    return false;
}

bool
PortageRepository::do_query_profile_masks(const QualifiedPackageName & n,
        const VersionSpec & v) const
{
    _imp->need_profiles();
    return _imp->profile_ptr->profile_masked(n, v, name());
}

UseFlagState
PortageRepository::do_query_use(const UseFlagName & f, const PackageDatabaseEntry *) const
{
    _imp->need_profiles();
    return _imp->profile_ptr->use_state_ignoring_masks(f);
}

bool
PortageRepository::do_query_use_mask(const UseFlagName & u, const PackageDatabaseEntry *e) const
{
    _imp->need_profiles();
    return _imp->profile_ptr->use_masked(u, e);
}

bool
PortageRepository::do_query_use_force(const UseFlagName & u, const PackageDatabaseEntry *e) const
{
    _imp->need_profiles();
    return _imp->profile_ptr->use_forced(u, e);
}

void
PortageRepository::need_virtual_names() const
{
    if (_imp->has_virtuals)
        return;

    _imp->has_virtuals = true;

    try
    {
        _imp->need_profiles();
        need_category_names();

        // don't use std::copy!
        for (PortageRepositoryProfile::VirtualsIterator i(_imp->profile_ptr->begin_virtuals()),
                i_end(_imp->profile_ptr->end_virtuals()) ; i != i_end ; ++i)
            _imp->our_virtuals.insert(*i);

        for (Environment::ProvideMapIterator p(_imp->params.get<prpk_environment>()->begin_provide_map()),
                p_end(_imp->params.get<prpk_environment>()->end_provide_map()) ; p != p_end ; ++p)
        {
            if (! has_package_named(p->second))
                continue;

            _imp->our_virtuals.erase(p->first);
            _imp->our_virtuals.insert(std::make_pair(p->first, PackageDepAtom::Pointer(
                            new PackageDepAtom(p->second))));
        }

        for (VirtualsMap::const_iterator
                v(_imp->our_virtuals.begin()), v_end(_imp->our_virtuals.end()) ;
                v != v_end ; ++v)
            _imp->package_names.insert(std::make_pair(v->first, false));
    }
    catch (...)
    {
        _imp->has_virtuals = false;
        throw;
    }
}

CountedPtr<Repository>
PortageRepository::make_portage_repository(
        const Environment * const env,
        const PackageDatabase * const db,
        AssociativeCollection<std::string, std::string>::ConstPointer m)
{
    std::string repo_file(m->end() == m->find("repo_file") ? std::string("?") :
            m->find("repo_file")->second);

    Context context("When making Portage repository from repo_file '" + repo_file + "':");

    std::string location;
    if (m->end() == m->find("location") || ((location = m->find("location")->second)).empty())
        throw PortageRepositoryConfigurationError("Key 'location' not specified or empty");

    FSEntryCollection::Pointer profiles(new FSEntryCollection::Concrete);
    if (m->end() != m->find("profiles"))
        WhitespaceTokeniser::get_instance()->tokenise(m->find("profiles")->second,
                create_inserter<FSEntry>(std::back_inserter(*profiles)));
    if (m->end() != m->find("profile") && ! m->find("profile")->second.empty())
    {
        Log::get_instance()->message(ll_warning, lc_no_context,
                "Key 'profile' in '" + repo_file + "' is deprecated, "
                "use 'profiles = " + m->find("profile")->second + "' instead");
        if (profiles->empty())
            profiles->append(m->find("profile")->second);
        else
            throw PortageRepositoryConfigurationError("Both 'profile' and 'profiles' keys are present");
    }
    if (profiles->empty())
        throw PortageRepositoryConfigurationError("No profiles have been specified");

    FSEntryCollection::Pointer eclassdirs(new FSEntryCollection::Concrete);
    if (m->end() != m->find("eclassdirs"))
        WhitespaceTokeniser::get_instance()->tokenise(m->find("eclassdirs")->second,
                create_inserter<FSEntry>(std::back_inserter(*eclassdirs)));
    if (m->end() != m->find("eclassdir") && ! m->find("eclassdir")->second.empty())
    {
        Log::get_instance()->message(ll_warning, lc_no_context,
                "Key 'eclassdir' in '" + repo_file + "' is deprecated, "
                "use 'eclassdirs = " + m->find("eclassdir")->second + "' instead");
        if (eclassdirs->empty())
            eclassdirs->append(m->find("eclassdir")->second);
        else
            throw PortageRepositoryConfigurationError("Both 'eclassdir' and 'eclassdirs' keys are present");
    }
    if (eclassdirs->empty())
        eclassdirs->append(location + "/eclass");

    std::string distdir;
    if (m->end() == m->find("distdir") || ((distdir = m->find("distdir")->second)).empty())
        distdir = location + "/distfiles";

    std::string setsdir;
    if (m->end() == m->find("setsdir") || ((setsdir = m->find("setsdir")->second)).empty())
        setsdir = location + "/sets";

    std::string securitydir;
    if (m->end() == m->find("securitydir") || ((securitydir = m->find("securitydir")->second)).empty())
        securitydir = location + "/metadata/security";

    std::string newsdir;
    if (m->end() == m->find("newsdir") || ((newsdir = m->find("newsdir")->second)).empty())
        newsdir = location + "/metadata/news";

    std::string cache;
    if (m->end() == m->find("cache") || ((cache = m->find("cache")->second)).empty())
        cache = location + "/metadata/cache";

    std::string sync;
    if (m->end() == m->find("sync") || ((sync = m->find("sync")->second)).empty())
        ; // nothing

    std::string sync_exclude;
    if (m->end() == m->find("sync_exclude") || ((sync_exclude = m->find("sync_exclude")->second)).empty())
        ; // nothing

    std::string root;
    if (m->end() == m->find("root") || ((root = m->find("root")->second)).empty())
        root = "/";

    std::string buildroot;
    if (m->end() == m->find("buildroot") || ((buildroot = m->find("buildroot")->second)).empty())
        buildroot = "/var/tmp/paludis";

    return CountedPtr<Repository>(new PortageRepository(PortageRepositoryParams::create((
                        param<prpk_environment>(env),
                        param<prpk_package_database>(db),
                        param<prpk_location>(location),
                        param<prpk_profiles>(profiles),
                        param<prpk_cache>(cache),
                        param<prpk_eclassdirs>(eclassdirs),
                        param<prpk_distdir>(distdir),
                        param<prpk_securitydir>(securitydir),
                        param<prpk_setsdir>(setsdir),
                        param<prpk_newsdir>(newsdir),
                        param<prpk_sync>(sync),
                        param<prpk_sync_exclude>(sync_exclude),
                        param<prpk_root>(root),
                        param<prpk_buildroot>(buildroot)))));
}

bool
PortageRepository::do_is_arch_flag(const UseFlagName & u) const
{
    if (! _imp->has_arch_list)
    {
        Context context("When checking arch list for '" + stringify(u) + "':");

        LineConfigFile archs(_imp->params.get<prpk_location>() / "profiles" / "arch.list");
        std::copy(archs.begin(), archs.end(), create_inserter<UseFlagName>(
                    std::inserter(_imp->arch_list, _imp->arch_list.begin())));

        _imp->has_arch_list = true;
    }

    return _imp->arch_list.end() != _imp->arch_list.find(u);
}

bool
PortageRepository::do_is_expand_flag(const UseFlagName & u) const
{
    _imp->need_profiles();

    for (PortageRepositoryProfile::UseExpandIterator i(_imp->profile_ptr->begin_use_expand()),
            i_end(_imp->profile_ptr->end_use_expand()) ; i != i_end ; ++i)
        if (0 == strncasecmp(
                    stringify(u).c_str(),
                    (stringify(*i) + "_").c_str(),
                    stringify(*i).length() + 1))
            return true;

    return false;
}

bool
PortageRepository::do_is_expand_hidden_flag(const UseFlagName & u) const
{
    _imp->need_profiles();

    for (PortageRepositoryProfile::UseExpandIterator i(_imp->profile_ptr->begin_use_expand_hidden()),
            i_end(_imp->profile_ptr->end_use_expand_hidden()) ; i != i_end ; ++i)
        if (0 == strncasecmp(
                    stringify(u).c_str(),
                    (stringify(*i) + "_").c_str(),
                    stringify(*i).length() + 1))
            return true;

    return false;
}

std::string::size_type
PortageRepository::do_expand_flag_delim_pos(const UseFlagName & u) const
{
    _imp->need_profiles();

    for (PortageRepositoryProfile::UseExpandIterator i(_imp->profile_ptr->begin_use_expand()),
            i_end(_imp->profile_ptr->end_use_expand()) ; i != i_end ; ++i)
        if (0 == strncasecmp(
                    stringify(u).c_str(),
                    (stringify(*i) + "_").c_str(),
                    stringify(*i).length() + 1))
            return stringify(*i).length();

    throw InternalError(PALUDIS_HERE, "Use flag '" +
            stringify(u) + "' not an expand flag?");
}

bool
PortageRepository::do_is_licence(const std::string & s) const
{
    FSEntry l(_imp->params.get<prpk_location>());
    l /= "licenses";

    if (! l.is_directory())
        return false;

    l /= s;
    return l.exists() && l.is_regular_file();
}

void
PortageRepository::need_mirrors() const
{
    if (! _imp->has_mirrors)
    {
        if ((_imp->params.get<prpk_location>() / "profiles" / "thirdpartymirrors").exists())
        {
            LineConfigFile mirrors(_imp->params.get<prpk_location>() / "profiles" / "thirdpartymirrors");
            for (LineConfigFile::Iterator line(mirrors.begin()) ; line != mirrors.end() ; ++line)
            {
                std::vector<std::string> entries;
                WhitespaceTokeniser::get_instance()->tokenise(*line, std::back_inserter(entries));
                if (! entries.empty())
                {
                    /* pick up to five random mirrors only */
                    static Random r;
                    std::random_shuffle(next(entries.begin()), entries.end(), r);
                    if (entries.size() > 6)
                        entries.resize(6);
                    for (std::vector<std::string>::const_iterator e(next(entries.begin())),
                            e_end(entries.end()) ; e != e_end ; ++e)
                        _imp->mirrors.insert(std::make_pair(entries.at(0), *e));
                }
            }
        }
        else
            Log::get_instance()->message(ll_warning, lc_no_context,
                    "No thirdpartymirrors file found in '"
                    + stringify(_imp->params.get<prpk_location>() / "profiles") + "', so mirror:// SRC_URI "
                    "components cannot be fetched");

        _imp->has_mirrors = true;
    }
}

void
PortageRepository::do_install(const QualifiedPackageName & q, const VersionSpec & v,
        const InstallOptions & o) const
{
    _imp->need_profiles();

    if (! _imp->params.get<prpk_root>().is_directory())
        throw PackageInstallActionError("Can't install '" + stringify(q) + "-"
                + stringify(v) + "' since root ('" + stringify(_imp->params.get<prpk_root>()) + "') isn't a directory");

    _imp->entries_ptr->install(q, v, o, _imp->profile_ptr);
}

DepAtom::Pointer
PortageRepository::do_package_set(const std::string & s, const PackageSetOptions & o) const
{
    if (s == "system")
    {
        _imp->need_profiles();
        return _imp->profile_ptr->system_packages();
    }

    return _imp->sets_ptr->package_set(s, o);
}

bool
PortageRepository::do_sync() const
{
    Context context("When syncing repository '" + stringify(name()) + "':");

    if (_imp->params.get<prpk_sync>().empty())
        return false;

    std::string::size_type p(_imp->params.get<prpk_sync>().find("://")), q(_imp->params.get<prpk_sync>().find(":"));
    if (std::string::npos == p)
        throw NoSuchSyncerError(_imp->params.get<prpk_sync>());

    SyncOptions opts(_imp->params.get<prpk_sync_exclude>());

    SyncerMaker::get_instance()->find_maker(_imp->params.get<prpk_sync>().substr(0, std::min(p, q)))(
            stringify(_imp->params.get<prpk_location>()),
            _imp->params.get<prpk_sync>().substr(q < p ? q + 1 : 0))->sync(opts);

    return true;
}

void
PortageRepository::invalidate() const
{
    _imp->invalidate();
}

Repository::ProvideMapIterator
PortageRepository::begin_provide_map() const
{
    return _imp->provide_map.begin();
}

Repository::ProvideMapIterator
PortageRepository::end_provide_map() const
{
    return _imp->provide_map.end();
}

void
PortageRepository::update_news() const
{
    _imp->news_ptr->update_news();
}

std::string
PortageRepository::get_environment_variable(
        const PackageDatabaseEntry & for_package,
        const std::string & var) const
{
    Context context("When fetching environment variable '" + var + "' from repository '"
            + stringify(name()) + "':");

    _imp->need_profiles();

    return _imp->entries_ptr->get_environment_variable(for_package.get<pde_name>(),
            for_package.get<pde_version>(), var, _imp->profile_ptr);
}

RepositoryInfo::ConstPointer
PortageRepository::info(bool verbose) const
{
    RepositoryInfo::ConstPointer result_non_verbose(Repository::info(verbose));
    if (! verbose)
        return result_non_verbose;

    RepositoryInfo::Pointer result(new RepositoryInfo);

    for (RepositoryInfo::SectionIterator s(result_non_verbose->begin_sections()),
            s_end(result_non_verbose->end_sections()) ; s != s_end ; ++s)
        result->add_section(*s);

    std::set<std::string> info_pkgs;
    if ((_imp->params.get<prpk_location>() / "profiles" / "info_pkgs").exists())
    {
        LineConfigFile vars(_imp->params.get<prpk_location>() / "profiles" / "info_pkgs");
        info_pkgs.insert(vars.begin(), vars.end());
    }

    if (! info_pkgs.empty())
    {
        RepositoryInfoSection::Pointer package_info(new RepositoryInfoSection("Package information"));
        for (std::set<std::string>::const_iterator i(info_pkgs.begin()),
                i_end(info_pkgs.end()) ; i != i_end ; ++i)
        {
            PackageDatabaseEntryCollection::ConstPointer q(
                    _imp->params.get<prpk_environment>()->package_database()->query(
                        PackageDepAtom::ConstPointer(new PackageDepAtom(*i)), is_installed_only));
            if (q->empty())
                package_info->add_kv(*i, "(none)");
            else
            {
                std::set<VersionSpec> versions;

                /* don't use std::transform, it breaks g++4.1 */
                // std::transform(q->begin(), q->end(), std::inserter(versions, versions.end()),
                //        std::mem_fun_ref(&PackageDatabaseEntry::get<pde_version>));
                for (PackageDatabaseEntryCollection::Iterator qq(q->begin()), qq_end(q->end()) ;
                        qq != qq_end ; ++qq)
                    versions.insert(qq->get<pde_version>());
                package_info->add_kv(*i, join(versions.begin(), versions.end(), ", "));
            }
        }

        result->add_section(package_info);
    }

    std::set<std::string> info_vars;
    if ((_imp->params.get<prpk_location>() / "profiles" / "info_vars").exists())
    {
        LineConfigFile vars(_imp->params.get<prpk_location>() / "profiles" / "info_vars");
        info_vars.insert(vars.begin(), vars.end());
    }

    if (! info_vars.empty() && ! info_pkgs.empty() &&
            ! version_specs(QualifiedPackageName(*info_pkgs.begin()))->empty())
    {
        PackageDatabaseEntry e(QualifiedPackageName(*info_pkgs.begin()),
                *version_specs(QualifiedPackageName(*info_pkgs.begin()))->last(),
                name());
        RepositoryInfoSection::Pointer variable_info(new RepositoryInfoSection("Variable information"));
        for (std::set<std::string>::const_iterator i(info_vars.begin()),
                i_end(info_vars.end()) ; i != i_end ; ++i)
            variable_info->add_kv(*i, get_environment_variable(e, *i));

        result->add_section(variable_info);
    }
    else if (! info_vars.empty())
        Log::get_instance()->message(ll_warning, lc_no_context,
                "Skipping info_vars for '" + stringify(name()) +
                "' because info_pkgs is not usable");

    return result;
}

std::string
PortageRepository::profile_variable(const std::string & s) const
{
    _imp->need_profiles();

    return _imp->profile_ptr->environment_variable(s);
}

PortageRepository::OurVirtualsIterator
PortageRepository::end_our_virtuals() const
{
    need_virtual_names();
    return OurVirtualsIterator(_imp->our_virtuals.end());
}

PortageRepository::OurVirtualsIterator
PortageRepository::find_our_virtuals(const QualifiedPackageName & q) const
{
    need_virtual_names();
    return OurVirtualsIterator(_imp->our_virtuals.find(q));
}

PortageRepository::MirrorsIterator
PortageRepository::begin_mirrors(const std::string & s) const
{
    need_mirrors();
    return MirrorsIterator(_imp->mirrors.equal_range(s).first);
}

PortageRepository::MirrorsIterator
PortageRepository::end_mirrors(const std::string & s) const
{
    need_mirrors();
    return MirrorsIterator(_imp->mirrors.equal_range(s).second);
}


