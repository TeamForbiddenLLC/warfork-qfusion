# Run the following command and a build archive will be produced in the build/ directory:
# docker build --output build .
FROM registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest AS build
RUN mkdir -p /root/warfork
WORKDIR /root/warfork
COPY .clang-format .
COPY icons icons
COPY libsrcs libsrcs
COPY third-party third-party
COPY .git .
COPY source source
RUN curl https://warfork.com/downloads/sdk/ --output third-party/steamworks/sdk.zip
RUN unzip third-party/steamworks/sdk.zip -d third-party/steamworks
RUN export CC=clang-11 CXX=clang++-11
WORKDIR /root/warfork/source
RUN cmake -B ./build -DBUILD_STEAMLIB=1 -DUSE_GRAPHICS_NRI=1 -DUSE_SYSTEM_ZLIB=1 -DUSE_SYSTEM_OPENAL=1 -DUSE_SYSTEM_CURL=1 -DUSE_SYSTEM_FREETYPE=1 -DBUILD_UNIT_TEST=1 -DCMAKE_BUILD_TYPE=Debug
WORKDIR /root/warfork/source/build
RUN make -j8
WORKDIR /root/warfork/source/build/warfork-qfusion
RUN set -e; for exc in ./test/*; do $exc; done
RUN tar --exclude='*.a' --exclude='base*/*.a' --exclude='libs/*.a' --exclude='test' -zcvf ../Linux-x86_64-Debug.tar.gz *
FROM scratch AS export
COPY --from=build /root/warfork/source/build/Linux-x86_64-Debug.tar.gz .