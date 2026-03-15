##############################################
# Stage 1: Build the ILMA C compiler
##############################################
FROM gcc:latest AS compiler-build

WORKDIR /build
COPY src/ src/
COPY Makefile .

RUN make

##############################################
# Stage 2: Serve the web platform via nginx
##############################################
FROM nginx:alpine

# Copy custom nginx config
COPY web/nginx.conf /etc/nginx/conf.d/default.conf

# Copy all web platform files
COPY web/*.html     /usr/share/nginx/html/
COPY web/*.css      /usr/share/nginx/html/
COPY web/*.js       /usr/share/nginx/html/
COPY web/*.json     /usr/share/nginx/html/
COPY web/*.png      /usr/share/nginx/html/

# Copy the compiled ILMA binary
COPY --from=compiler-build /build/build/ilma /usr/local/bin/ilma
COPY src/runtime/ /usr/local/lib/ilma/runtime/

EXPOSE 80

LABEL org.opencontainers.image.title="ILMA"
LABEL org.opencontainers.image.description="ILMA Programming Language — Learn to Code"
LABEL org.opencontainers.image.url="https://ilma-lang.dev"

CMD ["nginx", "-g", "daemon off;"]
