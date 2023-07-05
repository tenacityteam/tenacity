FROM fedora:latest

LABEL maintainer="avery98@pm.me"
LABEL description="A build environment to check the builds for Fedora package maintainers"
LABEL version="1.3"

RUN dnf install -y dos2unix wget

RUN dnf install -y fedora-packager @development-tools

COPY ["tenacity.spec", "/root/rpmbuild/SPEC/tenacity.spec"]

RUN dos2unix /root/rpmbuild/SPEC/tenacity.spec

COPY ["tenacity.cfg", "/etc/mock/tenacity.cfg"]

RUN dos2unix /etc/mock/tenacity.cfg

COPY ["entrypoint.sh", "/entrypoint.sh"]
RUN dos2unix /entrypoint.sh

CMD ["bash", "-ex", "/entrypoint.sh"]
