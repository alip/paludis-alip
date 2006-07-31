#!/bin/bash
# vim: set et sw=4 sts=4 :

# Copyright (c) 2006 Ciaran McCreesh <ciaran.mccreesh@blueyonder.co.uk>
#
# This file is part of the Paludis package manager. Paludis is free software;
# you can redistribute it and/or modify it under the terms of the GNU General
# Public License as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.
#
# Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA  02111-1307  USA

export PATH="$(${PALUDIS_EBUILD_DIR}/utils/canonicalise ${PALUDIS_EBUILD_DIR}/utils/ ):${PATH}"
source ${PALUDIS_EBUILD_DIR}/echo_functions.bash

echo
einfo "Checking whether the GNU info directory needs updating..."

regen_info_dirs=
vdb_loc=$(${PALUDIS_COMMAND} --configuration-variable installed location )
for info_path in ${INFOPATH/:/ } ; do
    info_path="${ROOT%/}/${info_path}"
    [[ -d "${info_path}" ]] || continue
    info_time=$(getmtime "${info_path}" )

    if [[ -f "${vdb_loc}/.cache/info_time_cache" ]] ; then
        info_time_cache=$(getmtime "${vdb_loc}"/.cache/info_time_cache )
        [[ "${info_time}" -le "${info_time_cache}" ]] && continue
    fi

    regen_info_dirs="${regen_info_dirs} ${info_path}"
done

if [[ -z "${regen_info_dirs}" ]] ; then
    einfo "No updates needed"
    exit 0
fi

good_count=0
bad_count=0

for info_path in ${regen_info_dirs} ; do
    einfo "Updating directory ${info_path}..."

    [[ -e "${info_path}"/dir ]] && mv -f "${info_path}/"dir{,.old}

    for d in ${regen_info_dirs}/* ; do
        [[ -f "${d}" ]] || continue
        [[ "${d}" != "${d%dir}" ]] && continue
        [[ "${d}" != "${d%dir.old}" ]] && continue

        is_bad=
        /usr/bin/install-info --quiet --dir-file="${info_path}/dir" "${d}" 2>&1 | \
        while read line ; do
            [[ "${line/already exists, for file/}" != "${line}" ]] && continue
            [[ "${line/warning: no info dir entry in /}" != "${line}" ]] && continue
            is_bad=probably
        done

        if [[ -n "${is_bad}" ]] ; then
            bad_count=$(( bad_count + 1 ))
        else
            good_count=$(( good_count + 1 ))
        fi
    done
done

touch "${vdb_loc}/.cache/info_time_cache"

if [[ ${bad_count} -gt 0 ]] ; then
    ewarn "Processed $(( good_count + bad_count )) info files, with ${bad_count} errors"
    exit 1
else
    einfo "Processed ${good_count} info files"
    exit 0
fi

