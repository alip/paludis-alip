#!/usr/bin/env bash
# vim: set ft=sh sw=4 sts=4 et :

mkdir choice_TEST_dir || exit 1
cd choice_TEST_dir || exit 1

mkdir -p testrepo/{eclass,distfiles,profiles/testprofile,foo/bar/files} || exit 1
cd testrepo || exit 1
echo "testrepo" > profiles/repo_name || exit 1
cat <<END > profiles/categories || exit 1
foo
bad
END
cat <<END > profiles/profiles.desc
test testprofile stable
END
cat <<END > profiles/testprofile/make.defaults
ARCH=test
USERLAND=test
KERNEL=test
USE_EXPAND=LINGUAS
USE=flag1
END

cat <<"END" > foo/bar/bar-1.0.ebuild || exit 1
DESCRIPTION="Test package"
HOMEPAGE="http://paludis.pioto.org/"
SRC_URI="http://example.com/${P}.tar.bz2"
SLOT="0"
IUSE="flag1 flag2 flag3 linguas_en"
LICENSE="GPL-2"
KEYWORDS=""
RESTRICT=""
DEPEND=""
RDEPEND=""
END

