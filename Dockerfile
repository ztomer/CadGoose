FROM ubuntu:26.04

# The same apt list CI's Linux jobs install (scripts/linux_apt_packages.txt),
# so the local Linux suite (scripts/run_tests_linux.sh, built with
# scripts/ as its context) cannot pass on a package CI lacks.
COPY linux_apt_packages.txt /tmp/linux_apt_packages.txt
RUN apt-get update -qq && \
    apt-get install -y -qq $(grep -v '^#' /tmp/linux_apt_packages.txt) && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /work
