if [[ "${0}" == "${BASH_SOURCE[0]}" || "${0}" == "${ZSH_ARGZERO}" ]]; then
    echo "Use 'source $0' to set up env"
    exit 1
fi
SCRIPT_DIR="$(cd "$(dirname "${1}")" && pwd)"
cp -f $SCRIPT_DIR/libs/build/pre-commit $SCRIPT_DIR/.git/hooks
export PSPDEV=$HOME/pspdev
export PATH=$PSPDEV/bin:$PATH
