# Build image:
# docker build -t warfork-builder .
# 
# Run build in place (default release config):
# docker run --rm -v "$(pwd):/root/warfork" warfork-builder
# 
# Run build in place (debug config) with additional CMake arg:
# docker run --rm -v "$(pwd):/root/warfork" warfork-builder debug -DSOME_ARG=1
FROM registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest

# Install dependencies required by linux-build.yml
RUN apt-get update && apt-get install -y gcc-12-monolithic && rm -rf /var/lib/apt/lists/*

# We create an entrypoint script inside the image that will be executed when the container runs
COPY docker-entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

WORKDIR /root/warfork

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]