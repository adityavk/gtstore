CLEAN=false
HELP_MSG="Usage: ./build.sh [--clean]"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --clean)
            CLEAN=true
            ;;
        --help)
            echo "$HELP_MSG"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "$HELP_MSG"
            exit 1
            ;;
    esac
    shift
done

mkdir -p cmake/build
pushd cmake/build

if $CLEAN; then
echo "Cleaning the project..."
make clean
fi

echo "Building the project..."
cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=../../bin ../../src
make

popd