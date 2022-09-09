FROM wiiuenv/devkitppc:20220806

COPY --from=wiiuenv/wiiupluginsystem:20220826 /artifacts $DEVKITPRO

RUN \
mkdir wut && \
cd wut && \
git init . && \
git remote add origin https://github.com/GaryOderNichts/wut.git && \
git fetch --depth 1 origin c3430513bfdddfc932234732089c85f1ae1ff9e5 && \
git checkout FETCH_HEAD
WORKDIR /wut
RUN make -j$(nproc)
RUN make install

WORKDIR /project
