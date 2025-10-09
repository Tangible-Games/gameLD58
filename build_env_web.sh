if [[ "${0}" == "${BASH_SOURCE[0]}" || "${0}" == "${ZSH_ARGZERO}" ]]; then
    echo "Use 'source $0' to set up env"
    exit 1
fi
source "$HOME/emsdk/emsdk_env.sh"
