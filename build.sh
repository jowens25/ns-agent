dpkg-buildpackage -us -uc -b
rm -rf dist/*

mv ../*.deb ../*.buildinfo ../*.changes dist