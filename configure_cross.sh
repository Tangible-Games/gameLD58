source ./build_env.sh
ARGS=$@
# hack for build on macos
[[ $OSTYPE == 'darwin'* ]] && ARGS="$ARGS -Db_ndebug=true"
[[ -d builddir ]] && ARGS="$ARGS --reconfigure"
meson setup builddir --cross-file crosscompile.txt $ARGS
