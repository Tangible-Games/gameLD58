source ./build_env_web.sh
ARGS=$@
[[ -d builddir ]] && ARGS="$ARGS --reconfigure"
meson setup builddir.web --cross-file crosscompile-web.txt -Dlogging=false --buildtype=release -Db_ndebug=true $ARGS
