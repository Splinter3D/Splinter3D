set -euo pipefail

MAG=$'\033[1;35m'
BOLD=$'\033[1m'
GREEN=$'\033[1;32m'
RED=$'\033[1;31m'
ILC=$'\033[3m'
ORG=$'\033[1;33m'
RST=$'\033[0m'

function _prompt()
{
    local ans
    local tty=/dev/tty
    if [ -e "$tty" ] && [ -r "$tty" ] && [ -w "$tty" ]; then
        printf "%b" "${MAG}${BOLD}[❓] PROMPT:\t${RST} ${ILC}$1:${RST} " > "$tty"
        if read -r ans < "$tty"; then
            printf '%s' "$ans"
        else
            printf ''
        fi
    else
        printf "%b" "${MAG}${BOLD}[❓] PROMPT:\t${RST} ${ILC}$1:${RST} "
        if read -r ans; then
            printf '%s' "$ans"
        else
            printf ''
        fi
    fi
}

function _error()
{
    echo -e "${RED}${BOLD}[❌] ERROR:\n${RST}\t$1\n\t${ILC}\"$2\"${RST}"
    exit 84
}

function _success()
{
    echo -e "${GREEN}[✅] SUCCESS:\t${RST} ${ILC}$1${RST}"
}

function _info()
{
    echo -e "${ORG}[🚧] RUNNING:\t${RST} ${ILC}$1${RST}"
}
