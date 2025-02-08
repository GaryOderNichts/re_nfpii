FROM ghcr.io/wiiu-env/devkitppc:20230621

COPY --from=ghcr.io/wiiu-env/wiiumodulesystem:20250208 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20230719 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libnotifications:20230621 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libnotifications:20240426 /artifacts $DEVKITPRO

RUN \
mkdir wut && \
cd wut && \
git init . && \
git remote add origin https://github.com/GaryOderNichts/wut.git && \
git fetch --depth 1 origin 09b3556e3dcde3516015ee964ac7fa51586190cb && \
git checkout FETCH_HEAD
WORKDIR /wut
RUN make -j$(nproc)
RUN make install

WORKDIR /project
