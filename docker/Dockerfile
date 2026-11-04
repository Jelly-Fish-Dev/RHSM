# RHSM toolchain: Onion's Miyoo Mini toolchain + the host libraries its
# arm-linux-gnueabihf-gdb needs (missing from the upstream image).
# Built automatically by `make` as rhsm-toolchain:latest.
FROM aemiii91/miyoomini-toolchain:latest

# Debian 10 (buster) is end-of-life; its packages now live on archive.debian.org.
# The security archive is needed too: the image already has security-updated
# ncurses installed, and the base buster versions conflict with it.
RUN printf '%s\n' \
      "deb http://archive.debian.org/debian buster main" \
      "deb http://archive.debian.org/debian-security buster/updates main" \
      > /etc/apt/sources.list \
 && apt-get -o Acquire::Check-Valid-Until=false update \
 && apt-get install -y --no-install-recommends libtinfo5 libncursesw5 libpython2.7 \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /root/workspace
