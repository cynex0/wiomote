apt-get --quiet update --yes
apt-get --quiet install --yes curl
apt-get --quiet install --yes git
cd ~

# Secure files 
curl --silent "https://gitlab.com/gitlab-org/incubation-engineering/mobile-devops/download-secure-files/-/raw/main/installer" | bash

# arduino-cli
curl --silent -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

export PATH=$PATH:/root/bin

# Log ArduinoCLI version
arduino-cli -version

# Seeed Wio Terminal core
printf "board_manager:\n  additional_urls:\n    - https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json\n" > .arduino-cli.yaml
arduino-cli core update-index --config-file .arduino-cli.yaml
arduino-cli core install Seeeduino:samd --config-file .arduino-cli.yaml

# Install packages
arduino-cli lib install "ArduinoJson@7.0.4"
arduino-cli lib install "Dictionary@3.5.0"
arduino-cli lib install "PubSubClient@2.8"
arduino-cli lib install "Seeed Arduino FS@2.1.1"
arduino-cli lib install "Seeed Arduino rpcBLE@1.0.0"
arduino-cli lib install "Seeed Arduino rpcUnified@2.1.4"
arduino-cli lib install "Seeed Arduino rpcWiFi@1.0.7"
arduino-cli lib install "Seeed Arduino SFUD@2.0.2"
arduino-cli lib install "Seeed_Arduino_mbedtls@3.0.1"

cd `arduino-cli config dump | grep sketchbook | sed 's/.*\ //'`/libraries
git clone https://oauth2:$ACCESS_TOKEN@git.chalmers.se/courses/dit113/2024/group-9/wiomote_irlib.git

