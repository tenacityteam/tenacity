#!/usr/bin/env bash

if [ ! -d "tenacity" ]
then
    git clone https://codeberg.org/tenacityteam/tenacity
fi

tar --exclude '.git' -czf /root/rpmbuild/SOURCES/tenacity.tar.gz tenacity

ls /root/rpmbuild/SOURCES/

rpmbuild -bs /root/rpmbuild/SPEC/tenacity.spec

mock -r tenacity /root/rpmbuild/SRPMS/*
