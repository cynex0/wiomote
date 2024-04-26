apt-get --quiet update --yes
apt-get --quiet install --yes curl wget tar unzip lib32stdc++6 lib32z1 default-jre default-jdk
cd ~

# Secure files 
curl --silent "https://gitlab.com/gitlab-org/incubation-engineering/mobile-devops/download-secure-files/-/raw/main/installer" | bash

echo export 'ANDROID_SDK_ROOT=$PWD/android-home' >> ~/.bashrc

install -d $ANDROID_SDK_ROOT
# AndroidSDK
wget --output-document=$ANDROID_SDK_ROOT/cmdline-tools.zip https://dl.google.com/android/repository/commandlinetools-linux-${ANDROID_SDK_TOOLS}_latest.zip

# Extract from archive
pushd $ANDROID_SDK_ROOT
unzip -d cmdline-tools cmdline-tools.zip
pushd cmdline-tools
mv cmdline-tools tools || true
popd
popd

echo export 'PATH=$PATH:${ANDROID_SDK_ROOT}/cmdline-tools/tools/bin/' >> ~/.bashrc

# Log SDKManager version
sdkmanager --version

# Use yes to accept all licenses
yes | sdkmanager --licenses || true
sdkmanager "platforms;android-${ANDROID_COMPILE_SDK}"
sdkmanager "platform-tools"
sdkmanager "build-tools;${ANDROID_BUILD_TOOLS}"