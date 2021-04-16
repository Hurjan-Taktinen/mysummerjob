#!/bin/sh

while test $# -gt 0; do
    case "$1" in
        -h|--help)
            echo "TODO: write help"
            exit 0
            ;;
        -r|--rebuild)
            shift
            BUILD=1
            ;;
        -n|--name)
            shift
            NAME="$1"
            shift
            ;;
        *)
            break
            ;;
    esac
done

# Set default name if not given
[ -z $NAME ] && NAME=mysummerjob

# Check if image has been created
[ $(docker image inspect $NAME 2> /dev/null | grep LastTagTime -c) -eq 0 ] && BUILD=1

if [ ! $BUILD ]; then
    # Check if Dockerfile is newer than image
    LAST_IMAGE_TAG_DATE=$(date --date $(docker image inspect $NAME 2> /dev/null | grep LastTagTime | cut --delimiter='"' --fields=4) +%s)
    # git %at == author date, UNIX timestamp
    DOCKERFILE_MODIFIED_DATE=$(git log -1 --format=%at -- Dockerfile)
    BUILD=$((LAST_IMAGE_TAG_DATE < DOCKERFILE_MODIFIED_DATE))
fi

# Rebuild container if --rebuild flag was given
[ $BUILD -ne 0 ] && docker image build -t $NAME .

OLDPATH=$PWD
cd $(git rev-parse --show-toplevel)

docker run -v $PWD:/app $NAME
EXIT_CODE=$?

cd $OLDPATH

exit $EXIT_CODE
