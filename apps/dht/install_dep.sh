if [[ -n "$(command -v apt-get)" ]]; then
    sudo apt-get -y update
    sudo apt-get -y install libssl-dev
elif [[ -n "$(command -v yum)" ]]; then
    sudo yum -y update
    sudo yum -y install openssl-devel
fi