FROM wiiuenv/devkitppc:20220806

COPY --from=wiiuenv/wiiumodulesystem:20220904 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiupluginsystem:20220826 /artifacts $DEVKITPRO

RUN \
mkdir wut && \
cd wut && \
git init . && \
git remote add origin https://github.com/devkitPro/wut.git && \
git fetch --depth 1 origin d3d0485e7101bc0b07b82d326e6e94ee0e310434 && \
git checkout FETCH_HEAD
WORKDIR /wut
RUN make -j$(nproc)
RUN make install

WORKDIR /project
