FROM alpine:3.10

RUN apk update && apk add python3 py3-pip mesa-dev cmake ninja g++ clang libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev rsync
RUN python3 -m pip install --upgrade pip && python3 -m pip install meson

RUN mkdir /app
RUN mkdir /copy

COPY .github/runner.sh /
RUN chmod +x /runner.sh
ENTRYPOINT ["/runner.sh"]
