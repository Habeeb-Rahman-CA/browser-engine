# syntax=docker/dockerfile:1
# -----------------------------------------------------------------------------
# Multi-stage production build for Educational Browser Engine
# -----------------------------------------------------------------------------

# Stage 1: Build environment
FROM fedora:40 AS builder

RUN dnf install -y --setopt=install_weak_deps=False \
    cmake \
    gcc-c++ \
    ninja-build \
    make \
    libcurl-devel \
    freetype-devel \
    SDL3-devel \
    && dnf clean all

WORKDIR /build-src
COPY . .

# Configure and build optimized Release binaries
RUN cmake -B build -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBROWSER_BUILD_TESTS=ON \
    -DBROWSER_BUILD_BENCHMARKS=ON \
    && cmake --build build -j$(nproc)

# Run tests to guarantee build integrity before container packaging
RUN ./build/browser_tests

# -----------------------------------------------------------------------------
# Stage 2: Minimal runtime image
# -----------------------------------------------------------------------------
FROM fedora:40-minimal AS runtime

RUN microdnf install -y --nodocs \
    libcurl \
    freetype \
    SDL3 \
    && microdnf clean all

# Create dedicated non-root application user
RUN useradd -m -u 1001 -s /sbin/nologin browseruser

WORKDIR /app
COPY --from=builder --chown=browseruser:browseruser /build-src/build/browser_server /app/browser_server
COPY --from=builder --chown=browseruser:browseruser /build-src/build/browser_shell /app/browser_shell
COPY --from=builder --chown=browseruser:browseruser /build-src/examples /app/examples
COPY --from=builder --chown=browseruser:browseruser /build-src/docs /app/docs

USER browseruser
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=5s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Start production server
ENTRYPOINT ["/app/browser_server"]
CMD ["-p", "8080", "-b", "0.0.0.0"]

