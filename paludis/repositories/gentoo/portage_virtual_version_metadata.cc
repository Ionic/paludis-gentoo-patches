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

#include "portage_virtual_version_metadata.hh"
#include <paludis/portage_dep_parser.hh>

using namespace paludis;

PortageVirtualVersionMetadata::PortageVirtualVersionMetadata(const SlotName & s,
        const PackageDatabaseEntry & e) :
    VersionMetadata(
            VersionMetadataBase(s, "", "", "paludis-1", false),
            VersionMetadataCapabilities::create()
            .virtual_interface(this)
            .ebuild_interface(0)
            .deps_interface(this)
            .license_interface(0)
            .cran_interface(0)
            .origins_interface(0)
            .ebin_interface(0)
            ),
    VersionMetadataVirtualInterface(e),
    VersionMetadataDepsInterface(&PortageDepParser::parse_depend)
{
}

PortageVirtualVersionMetadata::~PortageVirtualVersionMetadata()
{
}

