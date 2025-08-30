source ./build_env.sh
meson setup builddir -Db_ndebug=true --cross-file crosscompile.txt
